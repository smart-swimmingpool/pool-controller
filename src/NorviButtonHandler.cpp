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
#include "Config.hpp"
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

// ═══════════════════════════════════════════════════════════════════════════

void NorviButtonHandler::begin() {
  LOG_INFO("• NorviButtonHandler initializing on ADC GPIO%d...\n", PIN_BUTTON_ADC);

  pinMode(PIN_BUTTON_ADC, INPUT);

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

void NorviButtonHandler::loop() {
  const uint32_t now = millis();

  // Sample at fixed intervals to reduce noise
  if (now - lastSampleMs_ < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleMs_ = now;

  // Read ADC
  uint16_t rawAdc = analogRead(PIN_BUTTON_ADC);

  // Enhanced ADC stability check: only accept values that are stable for 150ms
  static uint16_t lastStableAdc_ = 0;
  static uint32_t lastStableTime_ = 0;

  // Fast-attack: a genuine press/release changes the button range — even
  // when the ADC delta is small (no-press 3800 → S3 3700 = 100). Base the
  // re-initialization on button-range transitions so every valid press is
  // caught within one sample instead of a full moving-average window. The
  // transition also restarts the stability window, so a single-sample glitch
  // crossing an adjacent range boundary (e.g. 3800 → 3700, delta 100) cannot
  // be accepted immediately — it must hold for the full stability period.
  if (detectButton(rawAdc) != detectButton(lastFilteredAdc_)) {
    for (uint8_t i = 0; i < ADC_FILTER_SIZE; i++) {
      adcSamples_[i] = rawAdc;
    }
    adcSampleIndex_ = 0;
    lastStableTime_ = now;
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
    // evaluate the debounce on this reading too.
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
  if (currentButton_ != Button::NONE && pressStartMs_ > 0 && (now - pressStartMs_ >= LONG_PRESS_MS)) {
    pressStartMs_ = 0;  // Prevent re-firing

    // Fire long-press callbacks; skip short-press if callback consumed it
    switch (currentButton_) {
    case Button::ONE:
      if (cbButton1Long_ && cbButton1Long_()) {
        return;
      }
      break;
    case Button::TWO:
      if (cbButton2Long_ && cbButton2Long_()) {
        return;
      }
      break;
    case Button::THREE:
      if (cbButton3Long_ && cbButton3Long_()) {
        return;
      }
      break;
    default:
      break;
    }
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

}  // namespace PoolController

#endif  // NORVI_AE01_R
