// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>

namespace PoolController {

struct WiFiConfig {
  String ssid = "";
  String password = "";
};

struct MqttConfig {
  String host = "";
  uint16_t port = 1883;
  String username = "";
  String password = "";
  bool useTls = false;
};

struct NtpConfig {
  String server = "pool.ntp.org";
  long timezone = 0;
};

struct ControllerSettings {
  long loopInterval = 10;
  double tempMaxPool = 28.5;
  double tempMinSolar = 55.0;
  double tempHysteresis = 1.0;
  String opMode = "auto";
  long timeLossGreenHours = 1;
  long timeLossRedHours = 24;
  int timezoneIndex = 0;  ///< Index into TimeClientHelper timezone table
};

class ConfigManager {
public:
  ConfigManager() = default;

  static bool begin();
  static bool load();
  static bool save();
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
