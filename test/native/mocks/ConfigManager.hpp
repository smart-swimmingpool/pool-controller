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
  static bool isConfigured() { return _configured; }
  static void setConfigured(bool configured) { _configured = configured; }
  static bool begin() { return true; }
  static bool load() { return true; }
  static bool save() { return true; }
  static void reset() {
    _adminPasswordHash =  // gitleaks:allow  SHA-256("admin")
        F("8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918");
    _configured = false;
  }
  static void logOtaTransition() {}

  // ── Sensor Address Mapping ──
  static void saveSensorMapping(const uint8_t solarAddr[8], const uint8_t poolAddr[8]);
  static bool loadSensorMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]);

  static Settings &getSettings() { return _settings; }
  static WiFiConfig &getWiFi() { return _wifi; }
  static MqttConfig &getMqtt() { return _mqtt; }
  static NtpConfig &getNtp() { return _ntp; }

  static bool verifyAdminPassword(const String &password) {
    // Simple mock: compare with stored hash
    // In real implementation, this would hash the password and compare
    // For testing, we'll use a simple comparison
    String inputHash = hashPassword(password);
    return inputHash == _adminPasswordHash;
  }

  static void setAdminPassword(const String &newPassword) {
    _adminPasswordHash = hashPassword(newPassword);
  }

  static String getAdminPasswordHash() { return _adminPasswordHash; }

private:
  static Settings _settings;
  static WiFiConfig _wifi;
  static MqttConfig _mqtt;
  static NtpConfig _ntp;
  static String _adminPasswordHash;
  static bool _configured;

  static uint8_t _sensorSolarAddr[8];
  static uint8_t _sensorPoolAddr[8];
  static bool _sensorMappingValid;

  // Simple hash function for mock (not cryptographic, just for testing)
  static String hashPassword(const String &password) {
    // For testing purposes, we'll use a simple hash
    // In production, this would be SHA-256
    if (password == "admin") {
      return "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";  // gitleaks:allow
    }
    // Simple hash for other passwords
    uint32_t hash = 0;
    for (size_t i = 0; i < password.length(); i++) {
      hash = hash * 31 + password.charAt(i);
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%016lx", (unsigned long)hash);
    return String(buf);
  }
};

} // namespace PoolController
