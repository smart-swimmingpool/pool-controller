// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file NorviButtonHandler.cpp
 * @brief NORVI front-panel button handler implementation.
 *
 * Samples the ADC on GPIO32 at 50 ms intervals, debounces via
 * a simple time-window filter, and fires registered callbacks on
 * short press events.
 *
 * @note This file is only compiled when `NORVI_AE01_R` is defined.
 */

#ifdef NORVI_AE01_R

#include "NorviButtonHandler.hpp"

#include <Arduino.h>
#include "CalibrationManager.hpp"
#include "Config.hpp"
#include "ConfigManager.hpp"
#include "LogCapture.hpp"

namespace PoolController {

// ── Static members ─────────────────────────────────────────────────────────

NorviButtonHandler::Button NorviButtonHandler::currentButton_ = Button::NONE;
NorviButtonHandler::Button NorviButtonHandler::stableButton_ = Button::NONE;
uint32_t NorviButtonHandler::lastChangeMs_ = 0;
uint32_t NorviButtonHandler::lastSampleMs_ = 0;
uint16_t NorviButtonHandler::lastRaw_ = 0;

// ADC Filtering: moving average over 5 samples (250ms window) with
// fast-attack re-initialization — a genuine press/release jumps the
// window immediately instead of waiting for the average to catch up.
static constexpr uint8_t ADC_FILTER_SIZE = 5;
static uint16_t adcSamples_[ADC_FILTER_SIZE] = {0};
static uint8_t adcSampleIndex_ = 0;
static uint32_t lastFilteredAdcTime_ = 0;
static uint16_t lastFilteredAdc_ = 0;

NorviButtonHandler::ButtonCallback NorviButtonHandler::cbButton1_ = nullptr;
NorviButtonHandler::ButtonCallback NorviButtonHandler::cbButton2_ = nullptr;
NorviButtonHandler::ButtonCallback NorviButtonHandler::cbButton3_ = nullptr;

NorviButtonHandler::ButtonLongPressCallback NorviButtonHandler::cbButton1Long_ = nullptr;
NorviButtonHandler::ButtonLongPressCallback NorviButtonHandler::cbButton2Long_ = nullptr;
NorviButtonHandler::ButtonLongPressCallback NorviButtonHandler::cbButton3Long_ = nullptr;

uint32_t NorviButtonHandler::pressStartMs_ = 0;
uint32_t NorviButtonHandler::releasePendingMs_ = 0;
bool NorviButtonHandler::calibWasActive_ = false;
bool NorviButtonHandler::suppressCallbacks_ = false;

// ADC thresholds — defaults are the 2026-08-16 calibrated values; begin()
// overwrites them from ConfigManager (NVS) so they are user-configurable.
uint16_t NorviButtonHandler::THRESH_BTN1_MIN{3100};
uint16_t NorviButtonHandler::THRESH_BTN1_MAX{3520};
uint16_t NorviButtonHandler::THRESH_BTN2_MIN{3520};
uint16_t NorviButtonHandler::THRESH_BTN2_MAX{3880};
uint16_t NorviButtonHandler::THRESH_BTN3_MIN{3880};
uint16_t NorviButtonHandler::THRESH_BTN3_MAX{4095};
uint16_t NorviButtonHandler::THRESH_NO_PRESS{4096};

// ═══════════════════════════════════════════════════════════════════════════

void NorviButtonHandler::begin() {
  LOG_INFO("• NorviButtonHandler initializing on ADC GPIO%d...\n", PIN_BUTTON_ADC);

  pinMode(PIN_BUTTON_ADC, INPUT);

  applySettings();

  calibWasActive_ = false;
  suppressCallbacks_ = false;

  // Take an initial sample to let the ADC stabilise
  analogRead(PIN_BUTTON_ADC);
  delay(10);
  lastRaw_ = analogRead(PIN_BUTTON_ADC);

  LOG_INFO("  ◦ ADC initial value: %u\n", lastRaw_);
  LOG_INFO("  ◦ Button 1 ADC range: %u–%u\n", THRESH_BTN1_MIN, THRESH_BTN1_MAX);
  LOG_INFO("  ◦ Button 2 ADC range: %u–%u\n", THRESH_BTN2_MIN, THRESH_BTN2_MAX);
  LOG_INFO("  ◦ Button 3 ADC range: %u–%u\n", THRESH_BTN3_MIN, THRESH_BTN3_MAX);
  LOG_INFO("✓ NorviButtonHandler initialized\n");
}

// ═══════════════════════════════════════════════════════════════════════════

void NorviButtonHandler::applySettings() {
  // Load configurable ADC thresholds from NVS (defaults = calibrated values)
  THRESH_BTN1_MIN = ConfigManager::getSettings().btn1Min;
  THRESH_BTN1_MAX = ConfigManager::getSettings().btn1Max;
  THRESH_BTN2_MIN = ConfigManager::getSettings().btn2Min;
  THRESH_BTN2_MAX = ConfigManager::getSettings().btn2Max;
  THRESH_BTN3_MIN = ConfigManager::getSettings().btn3Min;
  THRESH_BTN3_MAX = ConfigManager::getSettings().btn3Max;
  THRESH_NO_PRESS = ConfigManager::getSettings().btnNoPress;
}

// ═══════════════════════════════════════════════════════════════════════════

void NorviButtonHandler::loop() {
  const bool calibActive = CalibrationManager::isActive();

  // Enhanced ADC stability check: only accept values that are stable for 150ms
  static uint16_t lastStableAdc_ = 0;
  static uint32_t lastStableTime_ = 0;

  // Calibration just finished (DONE/ERROR): the user may still be holding
  // the button from the last wizard step. Reset press tracking and keep
  // callbacks suppressed until the ADC returns to the no-press range for
  // the debounce interval, so the held press cannot fire its callback.
  if (!calibActive && calibWasActive_) {
    suppressCallbacks_ = true;
    currentButton_ = Button::NONE;
    stableButton_ = Button::NONE;
    pressStartMs_ = 0;
    releasePendingMs_ = 0;
    lastChangeMs_ = millis();
    LOG_DEBUG("Calibration finished — callbacks suppressed until all buttons released\n");
  }
  calibWasActive_ = calibActive;

  // Suppress button handling while calibration is running — the wizard
  // owns the ADC input and button presses must not trigger actions.
  if (calibActive) {
    return;
  }

  // While suppression is armed, keep sampling so the release can be
  // detected, but never fire callbacks.
  if (suppressCallbacks_) {
    const uint32_t now = millis();
    if (now - lastSampleMs_ < SAMPLE_INTERVAL_MS) {
      return;
    }
    lastSampleMs_ = now;
    const uint16_t rawAdc = analogRead(PIN_BUTTON_ADC);
    if (detectButton(rawAdc) == Button::NONE) {
      if (releasePendingMs_ == 0) {
        releasePendingMs_ = now;
      } else if (now - releasePendingMs_ >= DEBOUNCE_MS) {
        // Confirmed all-buttons-released — re-enable callbacks and
        // re-baseline the filter so stale button-level samples cannot
        // produce a spurious press.
        suppressCallbacks_ = false;
        releasePendingMs_ = 0;
        for (uint8_t i = 0; i < ADC_FILTER_SIZE; i++) {
          adcSamples_[i] = rawAdc;
        }
        adcSampleIndex_ = 0;
        lastFilteredAdc_ = rawAdc;
        lastStableAdc_ = rawAdc;
        lastStableTime_ = now;
        lastRaw_ = rawAdc;
        LOG_DEBUG("All buttons released — callbacks re-enabled\n");
      }
    } else {
      releasePendingMs_ = 0;
    }
    return;
  }

  const uint32_t now = millis();

  // Sample at fixed intervals to reduce noise
  if (now - lastSampleMs_ < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleMs_ = now;

  // Read ADC
  uint16_t rawAdc = analogRead(PIN_BUTTON_ADC);

  // Fast-attack: a genuine press/release changes the button range — even
  // when the ADC delta is small (no-press ~2700 → S1 ~3400 = 700). Base the
  // re-initialization on button-range transitions so every valid press is
  // caught within one sample instead of a full moving-average window. The
  // transition also restarts the stability window, so a single-sample glitch
  // crossing an adjacent range boundary (e.g. 3800 → 3700, delta 100) cannot
  // be accepted immediately — it must hold for the full stability period.
  Button rawButton = detectButton(rawAdc);
  if (rawButton != detectButton(lastFilteredAdc_)) {
    for (uint8_t i = 0; i < ADC_FILTER_SIZE; i++) {
      adcSamples_[i] = rawAdc;
    }
    adcSampleIndex_ = 0;
    lastStableTime_ = now;

    if (releasePendingMs_ != 0) {
      // A press transition interrupts a pending release. If the release
      // already persisted for the debounce interval, commit it so the new
      // press is tracked as a fresh tap; otherwise treat it as bounce.
      if (rawButton != Button::NONE && (now - releasePendingMs_ >= DEBOUNCE_MS)) {
        currentButton_ = Button::NONE;
        lastChangeMs_ = now;
        pressStartMs_ = 0;
        LOG_DEBUG("Button released: %d\n", static_cast<int>(currentButton_));
      }
      releasePendingMs_ = 0;
    }

    // Release observation: a confirmed press released into the no-press
    // range is committed after DEBOUNCE_MS instead of the full stability
    // window, so rapid consecutive taps are not merged into one press.
    if (currentButton_ != Button::NONE && rawButton == Button::NONE) {
      releasePendingMs_ = now;
    }
  }

  // Commit a confirmed release once it has persisted for the debounce
  // interval. Runs before the stability gates so it is not delayed by the
  // press-stability window.
  if (releasePendingMs_ != 0 && (now - releasePendingMs_ >= DEBOUNCE_MS)) {
    currentButton_ = Button::NONE;
    lastChangeMs_ = now;
    pressStartMs_ = 0;
    releasePendingMs_ = 0;
    LOG_DEBUG("Button released: %d\n", static_cast<int>(currentButton_));
  }

  // Store sample for filtering
  adcSamples_[adcSampleIndex_] = rawAdc;
  adcSampleIndex_ = (adcSampleIndex_ + 1) % ADC_FILTER_SIZE;

  // Calculate moving average
  uint32_t sum = 0;
  for (uint8_t i = 0; i < ADC_FILTER_SIZE; i++) {
    sum += adcSamples_[i];
  }
  uint16_t filteredAdc = static_cast<uint16_t>(sum / ADC_FILTER_SIZE);
  lastFilteredAdc_ = filteredAdc;
  lastFilteredAdcTime_ = now;

  if (abs(static_cast<int>(filteredAdc) - static_cast<int>(lastStableAdc_)) > 100) {
    // Significant change detected - reset stability timer
    lastStableAdc_ = filteredAdc;
    lastStableTime_ = now;
    lastRaw_ = filteredAdc;
    Button detected = detectButton(filteredAdc);

    // Debug logging for ADC changes
    static uint16_t lastDebugAdc_ = 0xFFFF;
    if (abs(static_cast<int>(filteredAdc) - static_cast<int>(lastDebugAdc_)) > 100) {
      LOG_DEBUG("ADC: raw=%u filtered=%u → %d | THRESH: BTN1=%u-%u BTN2=%u-%u BTN3=%u-%u NO_PRESS=%u\n", rawAdc, filteredAdc,
        static_cast<int>(detected), THRESH_BTN1_MIN, THRESH_BTN1_MAX, THRESH_BTN2_MIN, THRESH_BTN2_MAX, THRESH_BTN3_MIN,
        THRESH_BTN3_MAX, THRESH_NO_PRESS);
      lastDebugAdc_ = filteredAdc;
    }
    // A pending short press may complete even while the signal changes —
    // evaluate the debounce on this reading too. A long-press callback may
    // also be due, so check it first (it consumes the press on success).
    if (evaluateLongPress(now)) {
      return;
    }
    evaluateShortPress(now);
    return;
  }

  if (now - lastStableTime_ < 150) {
    // Not stable yet - ignore this reading
    lastRaw_ = filteredAdc;
    Button detected = Button::NONE;

    // Debug logging for unstable ADC
    static uint16_t lastDebugAdc_ = 0xFFFF;
    if (abs(static_cast<int>(filteredAdc) - static_cast<int>(lastDebugAdc_)) > 100) {
      LOG_DEBUG("ADC UNSTABLE: filtered=%u (waiting for stability)\n", filteredAdc);
      lastDebugAdc_ = filteredAdc;
    }
    // Let a pending debounce complete while the ADC re-stabilizes (e.g.
    // after a release) — otherwise a short press never fires its callback.
    // Evaluate long presses here too: a held button spends most samples in
    // this wait, and a release at the LONG_PRESS_MS mark must not swallow
    // the callback (e.g. S3 save-and-reboot after a full 2s hold).
    if (evaluateLongPress(now)) {
      return;
    }
    evaluateShortPress(now);
    return;
  }

  // ADC is stable - use filtered value
  lastStableAdc_ = filteredAdc;
  lastStableTime_ = now;
  lastRaw_ = filteredAdc;
  Button detected = detectButton(filteredAdc);

  // Enhanced debug logging for ADC changes
  static uint16_t lastDebugAdc_ = 0xFFFF;
  if (abs(static_cast<int>(filteredAdc) - static_cast<int>(lastDebugAdc_)) > 100) {
    LOG_DEBUG("ADC: raw=%u filtered=%u → %d | THRESH: BTN1=%u-%u BTN2=%u-%u BTN3=%u-%u NO_PRESS=%u\n", rawAdc, filteredAdc,
      static_cast<int>(detected), THRESH_BTN1_MIN, THRESH_BTN1_MAX, THRESH_BTN2_MIN, THRESH_BTN2_MAX, THRESH_BTN3_MIN,
      THRESH_BTN3_MAX, THRESH_NO_PRESS);
    lastDebugAdc_ = filteredAdc;
  }

  // ── Press start tracking ─────────────────────────────────────────────
  if (detected != currentButton_) {
    currentButton_ = detected;
    lastChangeMs_ = now;

    if (detected != Button::NONE) {
      pressStartMs_ = now;  // Button was just pressed
      LOG_DEBUG("Button pressed: %d\n", static_cast<int>(detected));
    } else {
      pressStartMs_ = 0;  // Button released
      LOG_DEBUG("Button released: %d\n", static_cast<int>(currentButton_));
    }
    return;
  }

  // ── Long-press detection (fire once after LONG_PRESS_MS) ─────────────
  if (evaluateLongPress(now)) {
    return;  // Long-press callback consumed the press
  }

  // ── Short-press detection ────────────────────────────────────────────
  // Debounce: only register a change if it persists for DEBOUNCE_MS.
  // Also evaluated on unstable/significant-change readings (see above) so
  // a release during re-stabilization cannot swallow the callback.
  evaluateShortPress(now);
}

// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════

float NorviButtonHandler::getLongPressProgress() {
  if (currentButton_ == Button::NONE || pressStartMs_ == 0) {
    return 0.0f;
  }
  uint32_t elapsed = millis() - pressStartMs_;
  if (elapsed >= LONG_PRESS_MS) {
    return 1.0f;
  }
  return static_cast<float>(elapsed) / static_cast<float>(LONG_PRESS_MS);
}

// ═══════════════════════════════════════════════════════════════════════════

NorviButtonHandler::Button NorviButtonHandler::detectButton(uint16_t raw) {
  // Use the supplied ADC value for detection.
  uint16_t adcValue = raw;

  if (adcValue >= THRESH_NO_PRESS) {
    return Button::NONE;
  }

  if (adcValue >= THRESH_BTN1_MIN && adcValue <= THRESH_BTN1_MAX) {
    return Button::ONE;
  }

  if (adcValue >= THRESH_BTN2_MIN && adcValue <= THRESH_BTN2_MAX) {
    return Button::TWO;
  }

  if (adcValue >= THRESH_BTN3_MIN && adcValue <= THRESH_BTN3_MAX) {
    return Button::THREE;
  }

  return Button::NONE;
}

void NorviButtonHandler::evaluateShortPress(uint32_t now) {
  // Debounce: only register a change if it persists for DEBOUNCE_MS.
  if (stableButton_ != currentButton_ && (now - lastChangeMs_ >= DEBOUNCE_MS)) {
    stableButton_ = currentButton_;

    // Fire callback on press (not release)
    switch (stableButton_) {
    case Button::ONE:
      if (cbButton1_)
        cbButton1_();
      break;
    case Button::TWO:
      if (cbButton2_)
        cbButton2_();
      break;
    case Button::THREE:
      if (cbButton3_)
        cbButton3_();
      break;
    default:
      break;
    }
  }
}

bool NorviButtonHandler::evaluateLongPress(uint32_t now) {
  // Fire the long-press callback once after LONG_PRESS_MS.
  if (currentButton_ != Button::NONE && pressStartMs_ > 0 && (now - pressStartMs_ >= LONG_PRESS_MS)) {
    pressStartMs_ = 0;  // Prevent re-firing

    // Fire long-press callbacks; report whether the callback consumed the
    // press (short press must then be skipped).
    switch (currentButton_) {
    case Button::ONE:
      if (cbButton1Long_ && cbButton1Long_()) {
        return true;
      }
      break;
    case Button::TWO:
      if (cbButton2Long_ && cbButton2Long_()) {
        return true;
      }
      break;
    case Button::THREE:
      if (cbButton3Long_ && cbButton3Long_()) {
        return true;
      }
      break;
    default:
      break;
    }
  }
  return false;
}

}  // namespace PoolController

#endif  // NORVI_AE01_R
