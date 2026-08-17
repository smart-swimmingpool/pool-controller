// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file CalibrationManager.hpp
 * @brief Guided ADC calibration for the NORVI AE01-R front-panel buttons.
 *
 * Measures the resting level and each button level via the shared ADC input
 * (GPIO32), computes the button thresholds at the midpoints between adjacent
 * levels, and persists them through ConfigManager.
 *
 * @note Only available when the NORVI_AE01_R preprocessor macro is defined.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief State machine that drives the button calibration wizard.
 *
 * The wizard is driven from PoolController::loop() via loop(). The web UI
 * polls getStatus() to render instructions and the live ADC value, and calls
 * start()/cancel() to control the flow.
 */
class CalibrationManager {
public:
  /** @brief User-facing calibration steps. COMPUTE/SAVE are internal. */
  enum class Step : std::uint8_t {
    IDLE = 0,  ///< Not calibrating
    RESTING,   ///< Measure resting level (no button pressed)
    BTN1,      ///< Measure Button 1 level
    BTN2,      ///< Measure Button 2 level
    BTN3,      ///< Measure Button 3 level
    DONE,      ///< Calibration finished, thresholds saved
    ERROR      ///< Calibration failed (message in status)
  };

  /** @brief Snapshot of the calibration state for the web UI. */
  struct CalibrationStatus {
    Step step = Step::IDLE;
    uint16_t liveAdc = 0;       ///< Current filtered ADC reading
    uint16_t restingLevel = 0;  ///< Measured resting level (0 until measured)
    uint16_t s1 = 0;            ///< Measured Button 1 level (0 until measured)
    uint16_t s2 = 0;            ///< Measured Button 2 level (0 until measured)
    uint16_t s3 = 0;            ///< Measured Button 3 level (0 until measured)
    const char *message = "";   ///< Instruction or error text
  };

  /** @brief Initialize the calibration manager. */
  static void begin();

  /** @brief Drive the calibration state machine. Call from PoolController::loop(). */
  static void loop();

  /** @brief Start calibration. @return false if already active. */
  static bool start();

  /** @brief Cancel calibration; old thresholds stay in NVS. */
  static void cancel();

  /** @brief Current calibration status snapshot. */
  static CalibrationStatus getStatus();

  /** @brief True while a calibration is running (used to suppress button callbacks). */
  static bool isActive();

  // ── Test hooks (native tests only) ────────────────────────────────────
  /** @brief Override the ADC read function (default: analogRead on PIN_BUTTON_ADC). */
  static void setAdcReadForTest(uint16_t (*fn)());
  /** @brief Override the time function (default: millis). */
  static void setTimeForTest(uint32_t (*fn)());

private:
  enum class State : std::uint8_t { IDLE, RESTING, BTN1, BTN2, BTN3, COMPUTE, SAVE, DONE, ERROR };

  static void enterState(State s);
  static void handleMeasurementStep(State step, uint16_t previousLevel, uint16_t &outLevel);
  static void computeThresholds();
  static void saveThresholds();
  static uint16_t readAdc();
  static uint32_t now();

  static State state_;
  static CalibrationStatus status_;
  static uint16_t (*adcRead_)();
  static uint32_t (*timeFn_)();

  // Measurement phase state
  static uint32_t stepStartMs_;
  static uint32_t sampleCount_;
  static uint32_t sampleSum_;
  static uint32_t lastSampleMs_;
  static uint16_t lastReading_;
  static uint16_t stableCount_;
  static bool sampling_;
};

}  // namespace PoolController
