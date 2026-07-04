// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file ConfigManager.hpp
 * @brief Persistent configuration via NVS (Preferences) — WiFi, MQTT, NTP, and device settings.
 */

#pragma once

#include <Arduino.h>

namespace PoolController {

/** @brief WiFi credentials stored in config.json. */
struct WiFiConfig {
  String ssid = "";
  String password = "";
};

/** @brief MQTT broker connection settings. */
struct MqttConfig {
  String host = "";
  uint16_t port = 1883;
  String username = "";
  String password = "";
};

/** @brief NTP server and timezone settings. */
struct NtpConfig {
  String server = "pool.ntp.org";
  long timezone = 0;
};

/** @brief Device operation parameters (temperatures, mode, timing). */
struct ControllerSettings {
  long loopInterval = 10;
  double tempMaxPool = 28.5;
  double tempMinSolar = 55.0;
  double tempHysteresis = 1.0;
  double tempCircThreshold = 24.0;    ///< Temperature-based circulation: threshold in °C
  uint16_t tempCircFactor = 30;       ///< Temperature-based circulation: extra minutes per °C
  uint16_t tempCircMaxRuntime = 720;  ///< Temperature-based circulation: max total runtime in minutes
  String opMode = "auto";
  long timeLossGreenHours = 1;
  long timeLossRedHours = 24;
  int timezoneIndex = 0;  ///< Index into TimeClientHelper timezone table
};

/**
 * @brief Manages persistent configuration via NVS (Preferences).
 *
 * Provides access to WiFi, MQTT, NTP, and device settings. Config is
 * loaded on boot from NVS and saved on changes. Also manages the
 * admin password hash and boot-version tracking for OTA transitions.
 */
class ConfigManager {
public:
  ConfigManager() = default;

  /** @brief Initialize ConfigManager and load config from NVS. @return true if config loaded. */
  static bool begin();
  /** @brief (Re)load config from NVS. @return true on success. */
  static bool load();
  /** @brief Save config to NVS. @return true on success. */
  static bool save();
  /** @brief Reset config to factory defaults and save. */
  static void reset();

  static WiFiConfig &getWiFi() { return wifi_; }
  static MqttConfig &getMqtt() { return mqtt_; }
  static NtpConfig &getNtp() { return ntp_; }
  static ControllerSettings &getSettings() { return settings_; }

  static String getAdminPasswordHash() { return adminPasswordHash_; }
  static void setAdminPassword(const String &newPassword);
  static bool verifyAdminPassword(const String &password);

  static bool isConfigured() { return configured_; }
  static void setConfigured(bool configured) { configured_ = configured; }

  // ── Sensor Address Mapping ──
  /** @brief Save DS18B20 sensor address mapping to NVS. */
  static void saveSensorMapping(const uint8_t solarAddr[8], const uint8_t poolAddr[8]);
  /** @brief Load DS18B20 sensor address mapping from NVS. @return true if both addresses loaded. */
  static bool loadSensorMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]);

  // ── Boot Version Tracking ──
  /// Call once after loading config to log OTA transition status.
  static void logOtaTransition();

private:
  static WiFiConfig wifi_;
  static MqttConfig mqtt_;
  static NtpConfig ntp_;
  static ControllerSettings settings_;
  static String adminPasswordHash_;
  static bool configured_;
};

}  // namespace PoolController
