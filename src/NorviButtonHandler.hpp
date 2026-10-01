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
 * Calibrated thresholds (12-bit ADC, 0–4095) from live measurements:
 *   No press:    < 3100   (resting level oscillates ~2610–2990)
 *   Button 1:    3100 – 3520
 *   Button 2:    3520 – 3880
 *   Button 3:    3880 – 4095
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
   * @brief Reload ADC thresholds from ConfigManager (NVS).
   * Called after settings changes so new thresholds apply to the running
   * handler without a reboot.
   */
  static void applySettings();

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

  /// Fires the long-press callback once after LONG_PRESS_MS.
  /// @return true if the callback consumed the press (skip short press).
  static bool evaluateLongPress(uint32_t now);

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

  /// True while calibration was active on the previous loop() call.
  /// Used to detect the calibration-finish transition.
  static bool calibWasActive_;

  /// While set, callbacks stay suppressed until a confirmed
  /// all-buttons-released state is observed (armed when calibration
  /// finishes while a button is still held).
  static bool suppressCallbacks_;

  // ── ADC thresholds (12-bit, 0–4095) ──────────────────────────────────
  // Calibrated from live measurements on the NORVI AE01-R (2026-08-16):
  //   No press: ~2610–2990 (oscillates; must map to NONE)
  //   Button 1: ~3275–3456
  //   Button 2: ~3579–3765
  //   Button 3: ~4095 (full scale)
  // The resistor ladder pulls the ADC pin UP toward VCC on press.
  // Boundaries sit at the midpoints between adjacent levels.
  // THRESH_NO_PRESS is a no-op (4096 > ADC max) — S3 reads full scale.
  // Values are loaded from ConfigManager (NVS) in begin(); the defaults
  // below are the calibrated values and can be overridden via the web UI.

  static uint16_t THRESH_BTN1_MIN;
  static uint16_t THRESH_BTN1_MAX;
  static uint16_t THRESH_BTN2_MIN;
  static uint16_t THRESH_BTN2_MAX;
  static uint16_t THRESH_BTN3_MIN;
  static uint16_t THRESH_BTN3_MAX;
  static uint16_t THRESH_NO_PRESS;
};

}  // namespace PoolController
