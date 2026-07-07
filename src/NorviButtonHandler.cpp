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

namespace PoolController {

// ── Static members ─────────────────────────────────────────────────────────

NorviButtonHandler::Button NorviButtonHandler::currentButton_ = Button::NONE;
NorviButtonHandler::Button NorviButtonHandler::stableButton_ = Button::NONE;
uint32_t NorviButtonHandler::lastChangeMs_ = 0;
uint32_t NorviButtonHandler::lastSampleMs_ = 0;
uint16_t NorviButtonHandler::lastRaw_ = 0;

NorviButtonHandler::ButtonCallback NorviButtonHandler::cbButton1_ = nullptr;
NorviButtonHandler::ButtonCallback NorviButtonHandler::cbButton2_ = nullptr;
NorviButtonHandler::ButtonCallback NorviButtonHandler::cbButton3_ = nullptr;

NorviButtonHandler::ButtonLongPressCallback NorviButtonHandler::cbButton1Long_ = nullptr;
NorviButtonHandler::ButtonLongPressCallback NorviButtonHandler::cbButton2Long_ = nullptr;
NorviButtonHandler::ButtonLongPressCallback NorviButtonHandler::cbButton3Long_ = nullptr;

uint32_t NorviButtonHandler::pressStartMs_ = 0;

// ═══════════════════════════════════════════════════════════════════════════

void NorviButtonHandler::begin() {
  Serial.printf("• NorviButtonHandler initializing on ADC GPIO%d...\n", PIN_BUTTON_ADC);

  pinMode(PIN_BUTTON_ADC, INPUT);

  // Take an initial sample to let the ADC stabilise
  analogRead(PIN_BUTTON_ADC);
  delay(10);
  lastRaw_ = analogRead(PIN_BUTTON_ADC);

  Serial.printf("  ◦ ADC initial value: %u\n", lastRaw_);
  Serial.printf("  ◦ Button 1 ADC range: %u–%u\n", THRESH_BTN1_MIN, THRESH_BTN1_MAX);
  Serial.printf("  ◦ Button 2 ADC range: %u–%u\n", THRESH_BTN2_MIN, THRESH_BTN2_MAX);
  Serial.printf("  ◦ Button 3 ADC range: %u–%u\n", THRESH_BTN3_MIN, THRESH_BTN3_MAX);
  Serial.println("✓ NorviButtonHandler initialized");
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
  lastRaw_ = analogRead(PIN_BUTTON_ADC);
  Button detected = detectButton(lastRaw_);

  // Add debug logging for ADC changes
  static uint16_t lastDebugAdc_ = 0xFFFF;
  if (abs(static_cast<int>(lastRaw_) - static_cast<int>(lastDebugAdc_)) > 50) {
    Serial.printf("ADC: %u → %d\n", lastDebugAdc_, static_cast<int>(detected));
    lastDebugAdc_ = lastRaw_;
  }

  // ── Press start tracking ─────────────────────────────────────────────
  if (detected != currentButton_) {
    currentButton_ = detected;
    lastChangeMs_ = now;

    if (detected != Button::NONE) {
      pressStartMs_ = now;  // Button was just pressed
    } else {
      pressStartMs_ = 0;  // Button released
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
  // Debounce: only register a change if it persists for DEBOUNCE_MS
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
  if (raw >= THRESH_NO_PRESS) {
    return Button::NONE;
  }

  if (raw >= THRESH_BTN1_MIN && raw <= THRESH_BTN1_MAX) {
    return Button::ONE;
  }

  if (raw >= THRESH_BTN2_MIN && raw <= THRESH_BTN2_MAX) {
    return Button::TWO;
  }

  if (raw >= THRESH_BTN3_MIN && raw <= THRESH_BTN3_MAX) {
    return Button::THREE;
  }

  return Button::NONE;
}

}  // namespace PoolController

#endif  // NORVI_AE01_R
