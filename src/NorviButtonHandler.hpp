// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file NorviButtonHandler.hpp
 * @brief Front-panel button handler for the NORVI IIOT-AE01-R.
 *
 * The NORVI has three push buttons on the front panel that share a single
 * analog input (GPIO32). Each button connects through a different resistor
 * to create a distinct voltage level, allowing detection via ADC.
 *
 * Button actions (configurable):
 *   Button 1 — Cycle OLED display page
 *   Button 2 — Toggle pool pump ON/OFF (manual override)
 *   Button 3 — Cycle operation mode (auto → manu → boost → timer → auto)
 *
 * @note Only available when the NORVI_AE01_R preprocessor macro is defined.
 *
 * @see https://norvi.io/docs/norvi-iiot-ae01-r-datasheet/
 * @see https://norvi.io/docs/norvi-iiot-ae01-r-user-guide/
 */

#pragma once

#include <cstdint>
#include <functional>

namespace PoolController {

/**
 * @brief Reads and debounces the three front-panel buttons of the NORVI AE01-R.
 *
 * The buttons use a resistor ladder on GPIO32 (ADC1_CHANNEL_4). Each button
 * produces a distinct analog voltage range. The handler samples the ADC at
 * ~50 ms intervals, applies hysteresis for noise rejection, and fires
 * callbacks on press events.
 *
 * Default thresholds (12-bit ADC, 0–4095):
 *   Button 1:     200 – 1200
 *   Button 2:    1400 – 2500
 *   Button 3:    2700 – 3700
 *   No press:    > 3800
 */
class NorviButtonHandler {
public:
  /** @brief Identifiers for the three front-panel buttons. */
  enum class Button : std::uint8_t {
    NONE = 0,
    ONE,   ///< Left / top button
    TWO,   ///< Middle button
    THREE  ///< Right / bottom button
  };

  using ButtonCallback = std::function<void()>;
  /// Long-press callback: return true to consume (suppress short-press).
  using ButtonLongPressCallback = std::function<bool()>;

  /**
   * @brief Initialize the button ADC input.
   * Configures GPIO32 as analog input and samples once to stabilize the ADC.
   */
  static void begin();

  /**
   * @brief Sample and debounce buttons.
   * Must be called regularly from PoolController::loop().
   */
  static void loop();

  /** @brief Register a callback for Button 1 short press. */
  static void onButton1Press(ButtonCallback cb) { cbButton1_ = cb; }

  /** @brief Register a callback for Button 2 short press. */
  static void onButton2Press(ButtonCallback cb) { cbButton2_ = cb; }

  /** @brief Register a callback for Button 3 short press. */
  static void onButton3Press(ButtonCallback cb) { cbButton3_ = cb; }

  /**
   * @brief Register a callback for Button 1 long press (> 2 s).
   * @return true to consume (suppress short-press), false to allow short-press after release.
   */
  static void onButton1LongPress(ButtonLongPressCallback cb) { cbButton1Long_ = cb; }

  /**
   * @brief Register a callback for Button 2 long press (> 2 s).
   * @return true to consume (suppress short-press), false to allow short-press after release.
   */
  static void onButton2LongPress(ButtonLongPressCallback cb) { cbButton2Long_ = cb; }

  /**
   * @brief Register a callback for Button 3 long press (> 2 s).
   * @return true to consume (suppress short-press), false to allow short-press after release.
   */
  static void onButton3LongPress(ButtonLongPressCallback cb) { cbButton3Long_ = cb; }

  /** @brief Get the currently pressed button. */
  static Button getCurrentButton() { return currentButton_; }

  /** @brief Get the last raw ADC reading (for debugging). */
  static uint16_t getLastRawValue() { return lastRaw_; }

  /**
   * @brief Long-press progress 0.0–1.0 during a held press.
   * Returns 0.0f if no button is pressed or long-press already fired.
   * Returns 1.0f once LONG_PRESS_MS has elapsed.
   * Used by NorviOledDisplay to draw a progress bar.
   */
  static float getLongPressProgress();

private:
  /**
   * @brief Map an ADC reading to a Button ID.
   * @param raw  12-bit ADC value (0–4095).
   * @return Detected button, or NONE if no button is pressed.
   */
  static Button detectButton(uint16_t raw);

  /**
   * @brief Fire the short-press callback once the debounce interval elapsed.
   * Runs on every sample once a press is pending — including while the ADC
   * re-stabilizes after a release — so a short press is not swallowed by
   * the release stabilization path.
   */
  static void evaluateShortPress(uint32_t now);

  /// Debounce interval (ms) — ignores samples within this window.
  static constexpr uint32_t DEBOUNCE_MS{80};

  /// Sample interval (ms) between ADC reads.
  static constexpr uint32_t SAMPLE_INTERVAL_MS{50};

  /// Last detected button.
  static Button currentButton_;

  /// Last stable button (after debounce).
  static Button stableButton_;

  /// Timestamp of the last button state change (ms).
  static uint32_t lastChangeMs_;

  /// Timestamp of the last ADC sample (ms).
  static uint32_t lastSampleMs_;

  /// Last raw ADC reading.
  static uint16_t lastRaw_;

  /// Callback storage (short press).
  static ButtonCallback cbButton1_;
  static ButtonCallback cbButton2_;
  static ButtonCallback cbButton3_;

  /// Callback storage (long press).
  static ButtonLongPressCallback cbButton1Long_;
  static ButtonLongPressCallback cbButton2Long_;
  static ButtonLongPressCallback cbButton3Long_;

  /// Long-press duration threshold (ms).
  static constexpr uint32_t LONG_PRESS_MS{2000};

  /// Timestamp when the current button was first pressed.
  static uint32_t pressStartMs_;

  /// Timestamp when a confirmed press was first observed released (ms).
  /// Non-zero while the release is being debounced; the release is
  /// committed to `currentButton_` after DEBOUNCE_MS.
  static uint32_t releasePendingMs_;

  // ── ADC thresholds (12-bit, 0–4095) ──────────────────────────────────
  // These are typical ranges for the NORVI AE01-R resistor ladder.
  // Adjust if needed based on serial debug output.

  static constexpr uint16_t THRESH_BTN1_MIN{200};
  static constexpr uint16_t THRESH_BTN1_MAX{1200};
  static constexpr uint16_t THRESH_BTN2_MIN{1400};
  static constexpr uint16_t THRESH_BTN2_MAX{2500};
  static constexpr uint16_t THRESH_BTN3_MIN{2700};
  static constexpr uint16_t THRESH_BTN3_MAX{3700};
  static constexpr uint16_t THRESH_NO_PRESS{3800};
};

}  // namespace PoolController
