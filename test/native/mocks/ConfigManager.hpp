#pragma once
#include "Arduino.h"
#include <string>

namespace PoolController {

struct Settings {
  std::string opMode = "auto";
  int loopInterval = 5;
  float tempMaxPool = 28.0f;
  float tempMinSolar = 35.0f;
  float tempHysteresis = 1.0f;
  int timezoneIndex = 0;
  int timeLossGreenHours = 2;
  int timeLossRedHours = 24;
  uint8_t timerStartHour = 8;
  uint8_t timerStartMinutes = 0;
  uint8_t timerEndHour = 18;
  uint8_t timerEndMinutes = 0;
  float tempCircThreshold = 24.0f;
  int tempCircFactor = 30;
  int tempCircMaxRuntime = 720;
};

struct WiFiConfig {
  std::string ssid = "TestSSID";
  std::string password = "testpass";
};

struct MqttConfig {
  std::string host = "mqtt.local";
  uint16_t port = 1883;
  std::string username = "mqtt_user";
  std::string password = "mqtt_pass";
};

struct NtpConfig {
  std::string server = "pool.ntp.org";
};

class ConfigManager {
public:
  static bool isConfigured() { return true; }
  static void setConfigured(bool) {}
  static bool begin() { return true; }
  static bool load() { return true; }
  static bool save() { return true; }
  static void reset() {}
  static void logOtaTransition() {}

  static Settings &getSettings() { return _settings; }
  static WiFiConfig &getWiFi() { return _wifi; }
  static MqttConfig &getMqtt() { return _mqtt; }
  static NtpConfig &getNtp() { return _ntp; }

  static bool verifyAdminPassword(const String &) { return true; }
  static void setAdminPassword(const String &) {}

private:
  static Settings _settings;
  static WiFiConfig _wifi;
  static MqttConfig _mqtt;
  static NtpConfig _ntp;
};

} // namespace PoolController
