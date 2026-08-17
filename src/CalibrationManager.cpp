// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file CalibrationManager.cpp
 * @brief Guided ADC calibration for the NORVI AE01-R front-panel buttons.
 *
 * @note This file is only compiled when `NORVI_AE01_R` is defined.
 */

#ifdef NORVI_AE01_R

#include "CalibrationManager.hpp"

#include <Arduino.h>
#include "Config.hpp"
#include "ConfigManager.hpp"
#include "LogCapture.hpp"
#include "NorviButtonHandler.hpp"

namespace PoolController {

// ── Constants ──────────────────────────────────────────────────────────────

/// Per-step timeout: no stable level within this window → retry the step.
static constexpr uint32_t STEP_TIMEOUT_MS{10000};
/// Minimum gap between adjacent measured levels (ADC counts).
static constexpr uint16_t MIN_LEVEL_GAP{100};
/// Number of samples averaged per level (50 ms apart → 1 s window).
static constexpr uint32_t SAMPLE_COUNT{20};
/// Interval between individual samples during the sample phase.
static constexpr uint32_t SAMPLE_INTERVAL_MS{50};
/// Consecutive readings within this window count as "stable".
static constexpr uint16_t STABILITY_WINDOW{50};
/// Consecutive stable readings required before sampling starts.
static constexpr uint8_t STABLE_READINGS{3};

// ── Static members ─────────────────────────────────────────────────────────

CalibrationManager::State CalibrationManager::state_{State::IDLE};
CalibrationManager::CalibrationStatus CalibrationManager::status_{};
uint16_t (*CalibrationManager::adcRead_)() = nullptr;
uint32_t (*CalibrationManager::timeFn_)() = nullptr;
uint32_t CalibrationManager::stepStartMs_{0};
uint32_t CalibrationManager::sampleCount_{0};
uint32_t CalibrationManager::sampleSum_{0};
uint32_t CalibrationManager::lastSampleMs_{0};
uint16_t CalibrationManager::lastReading_{0};
uint16_t CalibrationManager::stableCount_{0};
bool CalibrationManager::sampling_{false};

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::begin() {
  if (adcRead_ == nullptr) {
    adcRead_ = []() { return static_cast<uint16_t>(analogRead(PIN_BUTTON_ADC)); };
  }
  if (timeFn_ == nullptr) {
    timeFn_ = []() { return static_cast<uint32_t>(millis()); };
  }
  state_ = State::IDLE;
  status_ = CalibrationStatus{};
  LOG_INFO("✓ CalibrationManager initialized\n");
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::loop() {
  if (state_ == State::IDLE || state_ == State::DONE || state_ == State::ERROR) {
    return;
  }

  const uint32_t t = now();
  status_.liveAdc = readAdc();

  switch (state_) {
  case State::RESTING:
    handleMeasurementStep(State::RESTING, 0, status_.restingLevel);
    break;
  case State::BTN1:
    handleMeasurementStep(State::BTN1, status_.restingLevel, status_.s1);
    break;
  case State::BTN2:
    handleMeasurementStep(State::BTN2, status_.s1, status_.s2);
    break;
  case State::BTN3:
    handleMeasurementStep(State::BTN3, status_.s2, status_.s3);
    break;
  case State::COMPUTE:
    computeThresholds();
    break;
  case State::SAVE:
    saveThresholds();
    break;
  default:
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::handleMeasurementStep(State step, uint16_t previousLevel, uint16_t &outLevel) {
  const uint32_t t = now();

  if (!sampling_) {
    // ── Wait phase: look for a stable reading ──────────────────────────
    if (t - stepStartMs_ >= STEP_TIMEOUT_MS) {
      // Timeout → retry the step (stay in the same state, reset timer)
      stepStartMs_ = t;
      stableCount_ = 0;
      status_.message = "No stable level detected — please try again";
      LOG_WARN("Calibration step timeout, retrying\n");
      return;
    }

    const uint16_t reading = readAdc();
    // Button levels are strictly ascending: accept only readings at least
    // MIN_LEVEL_GAP above the previous level (inclusive). This rejects
    // release levels (which drop back toward the resting level) and accepts
    // hardware whose adjacent levels sit exactly at the minimum gap.
    const bool differsFromPrevious = (previousLevel == 0) || (reading >= previousLevel + MIN_LEVEL_GAP);

    if (differsFromPrevious &&
      (stableCount_ == 0 || (reading > lastReading_ - STABILITY_WINDOW && reading < lastReading_ + STABILITY_WINDOW))) {
      stableCount_++;
      lastReading_ = reading;
    } else {
      stableCount_ = 0;
      lastReading_ = reading;
    }

    if (stableCount_ >= STABLE_READINGS) {
      // Stable → start sampling
      sampling_ = true;
      sampleCount_ = 0;
      sampleSum_ = 0;
      lastSampleMs_ = t;
      status_.message = "Level stable — sampling…";
    }
    return;
  }

  // ── Sample phase: collect SAMPLE_COUNT readings ──────────────────────
  // Gate on the sample interval so the readings span the intended window
  // instead of being collected within a few loop iterations.
  if (t - lastSampleMs_ >= SAMPLE_INTERVAL_MS) {
    lastSampleMs_ = t;
    const uint16_t reading = readAdc();
    // Reject outliers: if the reading leaves the stability window around
    // the level accepted in the wait phase (e.g. the button was released),
    // restart the measurement instead of averaging a mixed level.
    if (reading < lastReading_ - STABILITY_WINDOW || reading > lastReading_ + STABILITY_WINDOW) {
      sampling_ = false;
      stableCount_ = 0;
      status_.message = "Level changed — please hold the button steady";
      LOG_WARN("Calibration sample out of range, restarting step\n");
      return;
    }
    sampleSum_ += reading;
    sampleCount_++;
    if (sampleCount_ >= SAMPLE_COUNT) {
      const uint16_t average = static_cast<uint16_t>(sampleSum_ / SAMPLE_COUNT);
      sampling_ = false;
      stableCount_ = 0;
      stepStartMs_ = t;

      // Revalidate the averaged level against the previous measurement:
      // samples may drift within the stability window and produce an
      // average below the minimum gap, which would otherwise fail the
      // final sanity check in computeThresholds() and abort the wizard.
      if (previousLevel != 0 && average < previousLevel + MIN_LEVEL_GAP) {
        status_.message = "Level too close to the previous one — please try again";
        LOG_WARN("Calibration average %u below minimum gap, retrying step\n", average);
        return;  // stay in this state; the wait phase restarts
      }
      outLevel = average;

      switch (step) {
      case State::RESTING:
        status_.message = "Please press and hold Button 1";
        enterState(State::BTN1);
        break;
      case State::BTN1:
        status_.message = "Please press and hold Button 2";
        enterState(State::BTN2);
        break;
      case State::BTN2:
        status_.message = "Please press and hold Button 3";
        enterState(State::BTN3);
        break;
      case State::BTN3:
        status_.message = "Computing thresholds…";
        enterState(State::COMPUTE);
        break;
      default:
        break;
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::computeThresholds() {
  const uint16_t resting = status_.restingLevel;
  const uint16_t s1 = status_.s1;
  const uint16_t s2 = status_.s2;
  const uint16_t s3 = status_.s3;

  // Sanity checks: strictly ascending with minimum gaps, S3 within ADC range
  if (!(resting < s1 && s1 < s2 && s2 < s3) || (s1 - resting < MIN_LEVEL_GAP) || (s2 - s1 < MIN_LEVEL_GAP) ||
    (s3 - s2 < MIN_LEVEL_GAP) || s3 > 4095) {
    status_.step = Step::ERROR;
    status_.message = "Levels not ascending or too close — please re-run calibration";
    state_ = State::ERROR;
    LOG_ERROR("Calibration failed: levels resting=%u s1=%u s2=%u s3=%u\n", resting, s1, s2, s3);
    return;
  }

  state_ = State::SAVE;
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::saveThresholds() {
  auto &s = ConfigManager::getSettings();

  // Snapshot the previous thresholds so they can be restored if persistence
  // fails — the live Settings must not expose values that were never saved.
  const uint16_t prevBtn1Min = s.btn1Min;
  const uint16_t prevBtn1Max = s.btn1Max;
  const uint16_t prevBtn2Min = s.btn2Min;
  const uint16_t prevBtn2Max = s.btn2Max;
  const uint16_t prevBtn3Min = s.btn3Min;
  const uint16_t prevBtn3Max = s.btn3Max;
  const uint16_t prevBtnNoPress = s.btnNoPress;

  // Compute the new thresholds into temporaries first.
  const uint16_t resting = status_.restingLevel;
  const uint16_t s1 = status_.s1;
  const uint16_t s2 = status_.s2;
  const uint16_t s3 = status_.s3;
  const uint16_t btn1Min = (resting + s1) / 2;
  const uint16_t btn1Max = (s1 + s2) / 2;
  const uint16_t btn2Min = (s1 + s2) / 2;
  const uint16_t btn2Max = (s2 + s3) / 2;
  const uint16_t btn3Min = (s2 + s3) / 2;

  // Commit to the live Settings so save() persists them…
  s.btn1Min = btn1Min;
  s.btn1Max = btn1Max;
  s.btn2Min = btn2Min;
  s.btn2Max = btn2Max;
  s.btn3Min = btn3Min;
  s.btn3Max = 4095;     // full scale stays
  s.btnNoPress = 4096;  // sentinel stays

  if (!ConfigManager::save()) {
    // …and restore the previous values if persistence failed.
    s.btn1Min = prevBtn1Min;
    s.btn1Max = prevBtn1Max;
    s.btn2Min = prevBtn2Min;
    s.btn2Max = prevBtn2Max;
    s.btn3Min = prevBtn3Min;
    s.btn3Max = prevBtn3Max;
    s.btnNoPress = prevBtnNoPress;

    status_.step = Step::ERROR;
    status_.message = "Failed to save thresholds — please retry";
    state_ = State::ERROR;
    LOG_ERROR("Calibration failed: ConfigManager::save() returned false\n");
    return;
  }
  NorviButtonHandler::applySettings();
  status_.step = Step::DONE;
  status_.message = "Calibration complete — thresholds saved";
  state_ = State::DONE;
  LOG_INFO("✓ Calibration complete: btn1=%u-%u btn2=%u-%u btn3=%u-%u noPress=%u\n", s.btn1Min, s.btn1Max, s.btn2Min, s.btn2Max,
    s.btn3Min, s.btn3Max, s.btnNoPress);
}

// ═══════════════════════════════════════════════════════════════════════════

bool CalibrationManager::start() {
  if (state_ != State::IDLE && state_ != State::DONE && state_ != State::ERROR) {
    return false;  // already running
  }
  status_ = CalibrationStatus{};
  status_.step = Step::RESTING;
  status_.message = "Release all buttons — measuring resting level";
  enterState(State::RESTING);
  LOG_INFO("Calibration started\n");
  return true;
}

void CalibrationManager::cancel() {
  if (state_ == State::IDLE) {
    return;
  }
  status_ = CalibrationStatus{};
  status_.step = Step::IDLE;
  state_ = State::IDLE;
  sampling_ = false;
  stableCount_ = 0;
  LOG_INFO("Calibration cancelled\n");
}

CalibrationManager::CalibrationStatus CalibrationManager::getStatus() {
  return status_;
}

bool CalibrationManager::isActive() {
  return state_ != State::IDLE && state_ != State::DONE && state_ != State::ERROR;
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::enterState(State s) {
  state_ = s;
  stepStartMs_ = now();
  sampling_ = false;
  stableCount_ = 0;
  lastReading_ = 0;
  lastSampleMs_ = stepStartMs_;
  switch (s) {
  case State::RESTING:
    status_.step = Step::RESTING;
    break;
  case State::BTN1:
    status_.step = Step::BTN1;
    break;
  case State::BTN2:
    status_.step = Step::BTN2;
    break;
  case State::BTN3:
    status_.step = Step::BTN3;
    break;
  case State::DONE:
    status_.step = Step::DONE;
    break;
  case State::ERROR:
    status_.step = Step::ERROR;
    break;
  default:
    break;
  }
}

uint16_t CalibrationManager::readAdc() {
  return adcRead_ ? adcRead_() : 0;
}

uint32_t CalibrationManager::now() {
  return timeFn_ ? timeFn_() : 0;
}

void CalibrationManager::setAdcReadForTest(uint16_t (*fn)()) {
  adcRead_ = fn;
}

void CalibrationManager::setTimeForTest(uint32_t (*fn)()) {
  timeFn_ = fn;
}

}  // namespace PoolController

#endif  // NORVI_AE01_R
