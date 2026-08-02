// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file ConfigManager.cpp
 * @brief Persistent configuration — NVS (Preferences) read/write and factory reset.
 */

#include "ConfigManager.hpp"
#include "Version.h"
#include "LogCapture.hpp"
#include <Preferences.h>
#include <mbedtls/md.h>

namespace PoolController {

WiFiConfig ConfigManager::wifi_;
MqttConfig ConfigManager::mqtt_;
NtpConfig ConfigManager::ntp_;
ControllerSettings ConfigManager::settings_;
String ConfigManager::adminPasswordHash_ = "";
bool ConfigManager::configured_ = false;

// Default password is "admin" (SHA-256 hash, not a real secret)
// This hash is intentionally hardcoded for first-time setup
// gitleaks:allow - known default password hash
static constexpr const char *kDefaultPasswordHash =  // NOLINT(whitespace/line_length)
  "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";

static void hashSha256(const String &input, char (&output)[65]) {
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const uint8_t *)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  for (int i = 0; i < 32; i++) {
    snprintf(output + (i * 2), sizeof(output) - (i * 2), "%02x", hash[i]);
  }
  output[64] = '\0';
}

// ── NVS Key Names ──

static constexpr const char *kNvsNamespace = "config";
static constexpr const char *kWifiSsid = "wifi_ssid";
static constexpr const char *kWifiPass = "wifi_pass";
static constexpr const char *kMqttHost = "mqtt_host";
static constexpr const char *kMqttPort = "mqtt_port";
static constexpr const char *kMqttUser = "mqtt_user";
static constexpr const char *kMqttPass = "mqtt_pass";
static constexpr const char *kNtpServer = "ntp_server";
static constexpr const char *kNtpTz = "ntp_tz";
static constexpr const char *kSetInterval = "set_interval";
static constexpr const char *kSetMaxPool = "set_maxpool";
static constexpr const char *kSetMinSolar = "set_minsolar";
static constexpr const char *kSetHyst = "set_hyst";
static constexpr const char *kSetOpMode = "set_opmode";
static constexpr const char *kSetTzIdx = "set_tzidx";
static constexpr const char *kSetGreen = "set_green";
static constexpr const char *kSetRed = "set_red";
static constexpr const char *kSetCircThresh = "set_circth";
static constexpr const char *kSetCircFactor = "set_circfa";
static constexpr const char *kSetCircMax = "set_circmx";
static constexpr const char *kAdmPass = "adm_pass";
static constexpr const char *kCfgConfigured = "cfg_configured";

// ── Lifecycle ──

bool ConfigManager::begin() {
  LOG_INFO("✓ NVS config namespace opened\n");
  return load();
}

bool ConfigManager::load() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true)) {  // read-only mode
    LOG_ERROR("✖ Failed to open NVS config namespace\n");
    reset();
    return false;
  }

  wifi_.ssid = prefs.getString(kWifiSsid, "");
  wifi_.password = prefs.getString(kWifiPass, "");

  mqtt_.host = prefs.getString(kMqttHost, "");
  mqtt_.port = prefs.getUInt(kMqttPort, 1883);
  mqtt_.username = prefs.getString(kMqttUser, "");
  mqtt_.password = prefs.getString(kMqttPass, "");

  ntp_.server = prefs.getString(kNtpServer, "pool.ntp.org");
  ntp_.timezone = prefs.getLong(kNtpTz, 0);

  settings_.loopInterval = prefs.getLong(kSetInterval, 10);
  settings_.tempMaxPool = prefs.getDouble(kSetMaxPool, 28.5);
  settings_.tempMinSolar = prefs.getDouble(kSetMinSolar, 55.0);
  settings_.tempHysteresis = prefs.getDouble(kSetHyst, 1.0);
  settings_.opMode = prefs.getString(kSetOpMode, "auto");
  settings_.timezoneIndex = prefs.getInt(kSetTzIdx, 0);
  settings_.timeLossGreenHours = prefs.getUChar(kSetGreen, 1);
  settings_.timeLossRedHours = prefs.getUChar(kSetRed, 24);
  settings_.tempCircThreshold = prefs.getDouble(kSetCircThresh, 24.0);
  settings_.tempCircFactor = prefs.getUShort(kSetCircFactor, 30);
  settings_.tempCircMaxRuntime = prefs.getUShort(kSetCircMax, 720);

  adminPasswordHash_ = prefs.getString(kAdmPass, kDefaultPasswordHash);
  configured_ = prefs.getBool(kCfgConfigured, false);

  prefs.end();

  LOG_INFO("✓ Configuration loaded from NVS\n");
  return true;
}

bool ConfigManager::save() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {  // read-write mode
    LOG_ERROR("✖ Failed to open NVS config namespace for writing\n");
    return false;
  }

  prefs.putString(kWifiSsid, wifi_.ssid);
  prefs.putString(kWifiPass, wifi_.password);

  prefs.putString(kMqttHost, mqtt_.host);
  prefs.putUInt(kMqttPort, mqtt_.port);
  prefs.putString(kMqttUser, mqtt_.username);
  prefs.putString(kMqttPass, mqtt_.password);

  prefs.putString(kNtpServer, ntp_.server);
  prefs.putLong(kNtpTz, ntp_.timezone);

  prefs.putLong(kSetInterval, settings_.loopInterval);
  prefs.putDouble(kSetMaxPool, settings_.tempMaxPool);
  prefs.putDouble(kSetMinSolar, settings_.tempMinSolar);
  prefs.putDouble(kSetHyst, settings_.tempHysteresis);
  prefs.putString(kSetOpMode, settings_.opMode);
  prefs.putInt(kSetTzIdx, settings_.timezoneIndex);
  prefs.putUChar(kSetGreen, settings_.timeLossGreenHours);
  prefs.putUChar(kSetRed, settings_.timeLossRedHours);
  prefs.putDouble(kSetCircThresh, settings_.tempCircThreshold);
  prefs.putUShort(kSetCircFactor, settings_.tempCircFactor);
  prefs.putUShort(kSetCircMax, settings_.tempCircMaxRuntime);

  prefs.putString(kAdmPass, adminPasswordHash_);
  prefs.putBool(kCfgConfigured, configured_);

  prefs.end();

  LOG_INFO("✓ Configuration saved to NVS\n");
  return true;
}

void ConfigManager::reset() {
  // Clear all keys in the config namespace
  Preferences prefs;
  if (prefs.begin(kNvsNamespace, false)) {
    prefs.clear();
    prefs.end();
  }

  wifi_ = WiFiConfig{};
  mqtt_ = MqttConfig{};
  ntp_ = NtpConfig{};
  settings_ = ControllerSettings{};
  adminPasswordHash_ = kDefaultPasswordHash;  // Reset to default "admin" password
  configured_ = false;

  LOG_INFO("✓ Configuration reset to factory defaults\n");
}

// ── Boot Version Tracking ──

void ConfigManager::logOtaTransition() {
  Preferences prefs;
  prefs.begin("ota-version", false);  // namespace "ota-version" in NVS

  String previousVersion = prefs.getString("fw_version", "");
  String runningVersion = FW_VERSION;

  if (previousVersion.isEmpty()) {
    // First boot ever — nothing to compare
    LOG_INFO("ℹ First boot — firmware version %s\n", runningVersion.c_str());
  } else if (previousVersion != runningVersion) {
    // Version changed — OTA update just happened
    LOG_INFO("◉ OTA UPDATE DETECTED: %s → %s\n", previousVersion.c_str(), runningVersion.c_str());

    // Update stored version to match running version
    prefs.putString("fw_version", runningVersion);
  } else {
    // Normal boot — same version
    LOG_INFO("ℹ Normal boot — firmware %s (no OTA change)\n", runningVersion.c_str());
  }

  prefs.end();
}

void ConfigManager::setAdminPassword(const String &newPassword) {
  char hash[65];
  hashSha256(newPassword, hash);
  adminPasswordHash_ = hash;
}

bool ConfigManager::verifyAdminPassword(const String &password) {
  char hash[65];
  hashSha256(password, hash);
  return adminPasswordHash_ == hash;
}

void ConfigManager::saveSensorMapping(const uint8_t solarAddr[8], const uint8_t poolAddr[8]) {
  Preferences prefs;
  prefs.begin("ds18b20", false);  // read-write
  prefs.putBytes("solar_adr", solarAddr, 8);
  prefs.putBytes("pool_adr", poolAddr, 8);
  prefs.end();

  char buf[17];
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", solarAddr[0], solarAddr[1], solarAddr[2], solarAddr[3],
    solarAddr[4], solarAddr[5], solarAddr[6], solarAddr[7]);
  LOG_INFO("✓ Sensor mapping saved: Solar [%s]", buf);
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", poolAddr[0], poolAddr[1], poolAddr[2], poolAddr[3], poolAddr[4],
    poolAddr[5], poolAddr[6], poolAddr[7]);
  LOG_INFO(", Pool [%s]\n", buf);
}

bool ConfigManager::loadSensorMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  Preferences prefs;
  prefs.begin("ds18b20", true);  // read-only

  size_t slen = prefs.getBytes("solar_adr", solarAddr, 8);
  size_t plen = prefs.getBytes("pool_adr", poolAddr, 8);

  prefs.end();

  // Return true if both addresses were read successfully
  return (slen == 8 && plen == 8);
}

/** @brief Check if an 8-byte address is all zeros. */
static bool isSensorAddressZero(const uint8_t addr[8]) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] != 0)
      return false;
  }
  return true;
}

}  // namespace PoolController
