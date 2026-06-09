// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file NorviOledDisplay.hpp
 * @brief OLED display driver for the NORVI IIOT-AE01-R built-in 0.96" SSD1306.
 *
 * Shows system status on a 128×64 I2C OLED:
 *   Page 0 — Main: pool temp, solar temp, operation mode, status
 *   Page 1 — Network: WiFi SSID, IP, MQTT connection state
 *   Page 2 — System: uptime, free heap, firmware version
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
 * and updated periodically in loop(). Supports multiple display pages that
 * can be cycled via NorviButtonHandler.
 */
class NorviOledDisplay {
public:
  /** @brief Display page identifiers. */
  enum class Page : std::uint8_t {
    MAIN = 0,    ///< Pool/Solar temps, operation mode, status LED
    NETWORK,     ///< WiFi SSID, IP address, MQTT state
    SYSTEM,      ///< Uptime, free heap, firmware version
    QRCODE,      ///< QR code link to web interface
    SENSOR_SETUP,///< Address mapping setup wizard
    COUNT        ///< Number of pages (sentinel)
  };

  // ── Sensor setup wizard ──────────────────────────────────────────────

  /** @brief Check if the sensor setup wizard is active. */
  static bool isSetupModeActive() { return setupActive_; }
  /** @brief Enter the sensor setup wizard (switches to SENSOR_SETUP page). */
  static void enterSetupMode();
  /** @brief Exit the sensor setup wizard (returns to MAIN page). */
  static void exitSetupMode();
  /** @brief Move selection to the next detected device. */
  static void setupSelectNext();
  /**
   * @brief Assign the currently selected device as Solar.
   * @return true if assignment was accepted.
   */
  static bool setupAssignAsSolar();
  /**
   * @brief Assign the currently selected device as Pool.
   * @return true if assignment was accepted.
   */
  static bool setupAssignAsPool();
  /** @brief Check if both sensors have been assigned. */
  static bool isMappingComplete();
  /** @brief Copy the current mapping into the provided 8-byte buffers. */
  static void getMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]);

  /** @brief Get the number of devices detected on the shared bus (0, 1, or 2). */
  static uint8_t getDetectedDeviceCount();

  /**
   * @brief Initialize the OLED display.
   * Configures I2C (SDA=16, SCL=17), initializes the SSD1306 driver,
   * and shows a brief splash screen.
   */
  static void begin();

  /**
   * @brief Update the display periodically.
   * Refreshes the active page content at most every 2 seconds.
   * Must be called from PoolController::loop().
   */
  static void loop();

  /** @brief Switch to the next display page (wraps around). */
  static void nextPage();

  /** @brief Switch to the previous display page (wraps around). */
  static void prevPage();

  /** @brief Get the currently active page. */
  static Page getCurrentPage() { return currentPage_; }

  /**
   * @brief Request an immediate redraw on next loop() cycle.
   * Useful after button presses or mode changes.
   */
  static void requestRedraw() { forceRedraw_ = true; }

private:
  /** @brief Draw the current page content to the display buffer. */
  static void drawPage();

  /** @brief Draw the MAIN page — temperatures, mode, status. */
  static void drawMainPage();

  /** @brief Draw the NETWORK page — WiFi, IP, MQTT. */
  static void drawNetworkPage();

  /** @brief Draw the SYSTEM page — uptime, heap, firmware. */
  static void drawSystemPage();

  /** @brief Format uptime milliseconds into a "Xd Yh Zm" string. */
  static void formatUptime(uint32_t ms, char *buffer, size_t size);

  /// Currently active display page.
  static Page currentPage_;

  /// Timestamp of the last display refresh (ms).
  static uint32_t lastUpdateMs_;

  /// Minimum interval between display refreshes (ms).
  static constexpr uint32_t UPDATE_INTERVAL_MS{2000};

  /// Force an immediate redraw on the next loop() cycle.
  static bool forceRedraw_;

  // ── Sensor setup wizard state ─────────────────────────────────────────
  static bool setupActive_;          ///< Wizard is active
  static uint8_t setupSelectedDev_;  ///< Currently selected device index
  static bool setupSolarDone_;        ///< Solar has been assigned
  static bool setupPoolDone_;         ///< Pool has been assigned
  static uint8_t setupSolarAddr_[8];  ///< Solar device address (8-byte ROM)
  static uint8_t setupPoolAddr_[8];   ///< Pool device address (8-byte ROM)

  /** @brief Draw the SENSOR_SETUP page. */
  static void drawSensorSetupPage();

  /** @brief Draw the QRCODE page — QR code linking to web interface. */
  static void drawQrCodePage();

  /**
   * @brief Draw the shared footer bar on all informational pages.
   * Shows separator line + time + firmware version + page number.
   * Not called on SENSOR_SETUP (has its own footer).
   */
  static void drawFooter();

  /** @brief Maximum page index used in normal navigation (excludes SENSOR_SETUP). */
  static constexpr uint8_t MAX_NAV_PAGE{static_cast<uint8_t>(Page::QRCODE)};

  /** @brief Check if a DeviceAddress is all zeros. */
  static bool isAddressZero(const uint8_t addr[8]);
};

}  // namespace PoolController
