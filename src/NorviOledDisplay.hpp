// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file NorviOledDisplay.hpp
 * @brief OLED display driver for the NORVI IIOT-AE01-R built-in 0.96" SSD1306.
 *
 * Shows system status on a 128×64 I2C OLED with multiple pages:
 *   MAIN     — Pool & solar temps, operation mode, pump status
 *   NETWORK  — WiFi SSID, IP, MQTT connection state
 *   SYSTEM   — Uptime, free heap, firmware version
 *   QRCODE   — QR code link to web interface
 *   WIFI_SETUP — Captive portal / QR code for first-time WiFi config
 *   SENSOR_SETUP — DS18B20 address mapping wizard (two-step: pick sensor → pick role)
 *
 * Navigation: S1 (UP) prev page, S2 (DOWN) next page, S3 (CONFIRM) action.
 * Auto-returns to MAIN after 60 s of inactivity.
 * Includes OLED burn-in mitigation (periodic 2 px shift).
 *
 * @note Only available when the NORVI_AE01_R preprocessor macro is defined.
 *       I2C pins: SDA = GPIO16, SCL = GPIO17 (fixed by NORVI hardware).
 *
 * @see https://norvi.io/docs/norvi-iiot-ae01-r-datasheet/
 * @see https://norvi.io/docs/norvi-iiot-ae01-r-user-guide/
 */

#pragma once

#include <cstdint>
#include <cstddef>

namespace PoolController {

/**
 * @brief Manages the NORVI's built-in 0.96" SSD1306 OLED display.
 *
 * Singleton with static methods. Initialized once in PoolController::setup()
 * and updated periodically in loop().
 */
class NorviOledDisplay {
public:
  /** @brief Display page identifiers. */
  enum class Page : std::uint8_t {
    MAIN = 0,      ///< Pool/Solar temps, operation mode, pump status
    NETWORK,       ///< WiFi SSID, IP, MQTT state
    SYSTEM,        ///< Uptime, free heap, firmware version
    QRCODE,        ///< QR code link to web interface
    WIFI_SETUP,    ///< Captive portal QR for first-time WiFi config
    SENSOR_SETUP,  ///< Address mapping wizard (two-step)
    COUNT          ///< Number of pages (sentinel)
  };

  // ── Sensor setup wizard states ───────────────────────────────────────

  /** @brief Sub-states for the sensor setup wizard. */
  enum class SetupStep : std::uint8_t {
    IDLE,           ///< On SENSOR_SETUP page but not in active assignment
    SELECT_SENSOR,  ///< S1/S2 picks a sensor, S3 confirms
    SELECT_ROLE,    ///< S1/S2 picks Solar or Pool, S3 assigns
  };

  // ── Action menu items (MAIN page via S3) ────────────────────────────

  /** @brief Items in the MAIN-page action menu. */
  enum class MenuItem : std::uint8_t {
    MODE = 0,  ///< Cycle operation mode
    PUMP,      ///< Toggle pump on/off
    EXIT       ///< Return to MAIN
  };

  // ── Public API ───────────────────────────────────────────────────────

  /** @brief Initialize the OLED display + splash screen. */
  static void begin();

  /**
   * @brief Update the display periodically.
   * Handles auto-return to MAIN, burn-in shift, and page redraw.
   * Must be called from PoolController::loop().
   */
  static void loop();

  /** @brief Previous page (S1 / UP). */
  static void previousPage();

  /** @brief Next page (S2 / DOWN). */
  static void nextPage();

  /** @brief Confirm / action button (S3). */
  static void confirmAction();

  // ── Action menu (MAIN page) ─────────────────────────────────────────

  /** @brief Check if the action menu is active. */
  static bool isMenuActive() { return menuActive_; }

  /** @brief Get the currently selected menu item. */
  static MenuItem getMenuSelection() { return menuSelection_; }

  /** @brief Enter the action menu (from MAIN page). */
  static void enterMenu();

  /** @brief Exit the action menu, return to MAIN. */
  static void exitMenu();

  /** @brief Move menu selection to next item (down). */
  static void menuNext();

  /** @brief Move menu selection to previous item (up). */
  static void menuPrevious();

  /** @brief Get the currently active page. */
  static Page getCurrentPage() { return currentPage_; }

  /** @brief Request an immediate redraw on next loop() cycle. */
  static void requestRedraw() { forceRedraw_ = true; }

  /** @brief Get current burn-in horizontal offset (px). */
  static int8_t getBurnInDx() { return burnInDx_; }
  /** @brief Get current burn-in vertical offset (px). */
  static int8_t getBurnInDy() { return burnInDy_; }

  // ── Sensor setup wizard ──────────────────────────────────────────────

  /** @brief Check if the sensor setup wizard is actively assigning. */
  static bool isSetupActive() { return setupStep_ != SetupStep::IDLE; }

  /** @brief Check if the mapping page should be forced on first boot. */
  static bool needsSensorMapping();

  /** @brief Check if both sensors have been assigned. */
  static bool isMappingComplete();

  /** @brief Copy the current mapping into the provided 8-byte buffers. */
  static void getMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]);

  /** @brief Get the number of devices detected on the shared bus. */
  static uint8_t getDetectedDeviceCount();

  // ── Setup-step queries (for button wiring) ───────────────────────────

  static bool isSelectSensorStep() { return setupStep_ == SetupStep::SELECT_SENSOR; }
  static bool isSelectRoleStep() { return setupStep_ == SetupStep::SELECT_ROLE; }

  /** @brief Move sensor selection up (previous sensor). */
  static void setupSelectPrevious();
  /** @brief Move sensor selection down (next sensor). */
  static void setupSelectNext();
  /** @brief Select Solar role for the chosen sensor (S1). */
  static void setupSelectSolar();
  /** @brief Select Pool role for the chosen sensor (S2). */
  static void setupSelectPool();

  /** @brief Assign the selected sensor with the chosen role. */
  static bool setupApplyAssignment();

  // ── WiFi first-boot query ────────────────────────────────────────────

  /** @brief Check if WiFi credentials are configured. */
  static bool needsWiFiSetup();

private:
  // ═════════════════════════════════════════════════════════════════════
  // Draw methods
  // ═════════════════════════════════════════════════════════════════════

  /** @brief Draw the current page content to the display buffer. */
  static void drawPage();

  /** @brief Draw MAIN — temperatures, mode, pump status. */
  static void drawMainPage();

  /** @brief Draw NETWORK — WiFi, IP, MQTT. */
  static void drawNetworkPage();

  /** @brief Draw SYSTEM — uptime, heap, firmware. */
  static void drawSystemPage();

  /** @brief Draw QRCODE — QR code for web UI. */
  static void drawQrCodePage();

  /** @brief Draw WIFI_SETUP — captive portal info + QR. */
  static void drawWiFiSetupPage();

  /** @brief Draw the action menu overlay (MAIN page). */
  static void drawMenuPage();

  /** @brief Draw SENSOR_SETUP — address mapping wizard. */
  static void drawSensorSetupPage();

  /** @brief Draw the shared footer (mode, time, version, page). */
  static void drawFooter();

  /** @brief Draw the SENSOR_SETUP footer (context-sensitive hints). */
  static void drawSensorSetupFooter();

  /** @brief Format uptime milliseconds into a "Xd Yh Zm" string. */
  static void formatUptime(uint32_t ms, char *buffer, size_t size);

  // ═════════════════════════════════════════════════════════════════════
  // Utility
  // ═════════════════════════════════════════════════════════════════════

  /** @brief Check if a DeviceAddress is all zeros. */
  static bool isAddressZero(const uint8_t addr[8]);

  /** @brief Get the maximum navigable page index (COUNT - 1). */
  static uint8_t maxNavPage() { return static_cast<uint8_t>(Page::COUNT) - 1; }

  /**
   * @brief Update the burn-in prevention offset.
   * Cycles through a pattern of (dx, dy) shifts every BURN_IN_CYCLE_MS.
   */
  static void updateBurnInOffset();

  // ═════════════════════════════════════════════════════════════════════
  // Static state
  // ═════════════════════════════════════════════════════════════════════

  static Page currentPage_;
  static uint32_t lastUpdateMs_;
  static constexpr uint32_t UPDATE_INTERVAL_MS{2000};
  static bool forceRedraw_;

  // ── Idle auto-return ─────────────────────────────────────────────────
  static uint32_t lastButtonPressMs_;
  static constexpr uint32_t AUTO_RETURN_MS{60000};

  // ── Burn-in mitigation ───────────────────────────────────────────────
  static int8_t burnInDx_;  ///< Current horizontal shift (px)
  static int8_t burnInDy_;  ///< Current vertical shift (px)
  static uint32_t lastBurnInShiftMs_;
  static constexpr uint32_t BURN_IN_CYCLE_MS{120000};  ///< Shift every 2 min

  // ── Sensor setup state ───────────────────────────────────────────────
  static SetupStep setupStep_;
  static uint8_t setupSelectedDev_;
  static bool setupSolarDone_;
  static bool setupPoolDone_;
  static uint8_t setupSolarAddr_[8];
  static uint8_t setupPoolAddr_[8];
  static bool setupRoleIsSolar_;  ///< true = Solar, false = Pool (in SELECT_ROLE)

  // ── Action menu state ──────────────────────────────────────────────
  static bool menuActive_;         ///< True while action menu is shown
  static MenuItem menuSelection_;  ///< Currently highlighted menu item

  // ── First-boot flow tracking ─────────────────────────────────────────
  static bool firstBootDone_;  ///< True after initial setup flow completed
};

}  // namespace PoolController
