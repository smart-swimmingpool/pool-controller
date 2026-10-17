// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file ConfigManager.cpp
 * @brief Persistent configuration — NVS (Preferences) read/write and factory reset.
 */

#include "ConfigManager.hpp"

#include <cstring>

#include <Preferences.h>
#include <mbedtls/md.h>

#include "LogCapture.hpp"
#include "Version.h"

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
static constexpr const char *kSetBtn1Min = "set_btn1min";
static constexpr const char *kSetBtn1Max = "set_btn1max";
static constexpr const char *kSetBtn2Min = "set_btn2min";
static constexpr const char *kSetBtn2Max = "set_btn2max";
static constexpr const char *kSetBtn3Min = "set_btn3min";
static constexpr const char *kSetBtn3Max = "set_btn3max";
static constexpr const char *kSetBtnNoPress = "set_btnnop";
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
  settings_.btn1Min = prefs.getUShort(kSetBtn1Min, 3100);
  settings_.btn1Max = prefs.getUShort(kSetBtn1Max, 3520);
  settings_.btn2Min = prefs.getUShort(kSetBtn2Min, 3520);
  settings_.btn2Max = prefs.getUShort(kSetBtn2Max, 3880);
  settings_.btn3Min = prefs.getUShort(kSetBtn3Min, 3880);
  settings_.btn3Max = prefs.getUShort(kSetBtn3Max, 4095);
  settings_.btnNoPress = prefs.getUShort(kSetBtnNoPress, 4096);

  adminPasswordHash_ = prefs.getString(kAdmPass, kDefaultPasswordHash);
  configured_ = prefs.getBool(kCfgConfigured, false);

  prefs.end();

  LOG_INFO("✓ Configuration loaded from NVS\n");
  return true;
}

// Preferences::putString() returns the stored length, so 0 is a failure only
// for a non-empty value; an empty string cannot be told apart from an error.
static bool putStringChecked(Preferences &prefs, const char *key, const String &value) {
  return prefs.putString(key, value) == value.length();
}

bool ConfigManager::save() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {  // read-write mode
    LOG_ERROR("✖ Failed to open NVS config namespace for writing\n");
    return false;
  }

  // Every key is written even after a failure, so a retry or rollback rewrites
  // the whole namespace; the result reports whether all writes succeeded.
  bool ok = true;
  ok = putStringChecked(prefs, kWifiSsid, wifi_.ssid) && ok;
  ok = putStringChecked(prefs, kWifiPass, wifi_.password) && ok;

  ok = putStringChecked(prefs, kMqttHost, mqtt_.host) && ok;
  ok = prefs.putUInt(kMqttPort, mqtt_.port) != 0 && ok;
  ok = putStringChecked(prefs, kMqttUser, mqtt_.username) && ok;
  ok = putStringChecked(prefs, kMqttPass, mqtt_.password) && ok;

  ok = putStringChecked(prefs, kNtpServer, ntp_.server) && ok;
  ok = prefs.putLong(kNtpTz, ntp_.timezone) != 0 && ok;

  ok = prefs.putLong(kSetInterval, settings_.loopInterval) != 0 && ok;
  ok = prefs.putDouble(kSetMaxPool, settings_.tempMaxPool) != 0 && ok;
  ok = prefs.putDouble(kSetMinSolar, settings_.tempMinSolar) != 0 && ok;
  ok = prefs.putDouble(kSetHyst, settings_.tempHysteresis) != 0 && ok;
  ok = putStringChecked(prefs, kSetOpMode, settings_.opMode) && ok;
  ok = prefs.putInt(kSetTzIdx, settings_.timezoneIndex) != 0 && ok;
  ok = prefs.putUChar(kSetGreen, settings_.timeLossGreenHours) != 0 && ok;
  ok = prefs.putUChar(kSetRed, settings_.timeLossRedHours) != 0 && ok;
  ok = prefs.putDouble(kSetCircThresh, settings_.tempCircThreshold) != 0 && ok;
  ok = prefs.putUShort(kSetCircFactor, settings_.tempCircFactor) != 0 && ok;
  ok = prefs.putUShort(kSetCircMax, settings_.tempCircMaxRuntime) != 0 && ok;
  ok = prefs.putUShort(kSetBtn1Min, settings_.btn1Min) != 0 && ok;
  ok = prefs.putUShort(kSetBtn1Max, settings_.btn1Max) != 0 && ok;
  ok = prefs.putUShort(kSetBtn2Min, settings_.btn2Min) != 0 && ok;
  ok = prefs.putUShort(kSetBtn2Max, settings_.btn2Max) != 0 && ok;
  ok = prefs.putUShort(kSetBtn3Min, settings_.btn3Min) != 0 && ok;
  ok = prefs.putUShort(kSetBtn3Max, settings_.btn3Max) != 0 && ok;
  ok = prefs.putUShort(kSetBtnNoPress, settings_.btnNoPress) != 0 && ok;

  ok = putStringChecked(prefs, kAdmPass, adminPasswordHash_) && ok;
  ok = prefs.putBool(kCfgConfigured, configured_) != 0 && ok;

  prefs.end();

  if (!ok) {
    LOG_ERROR("✖ Failed to write configuration to NVS\n");
    return false;
  }

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

bool ConfigManager::resetToDefaults() {
  const WiFiConfig previousWifi = wifi_;
  const MqttConfig previousMqtt = mqtt_;
  const NtpConfig previousNtp = ntp_;
  const ControllerSettings previousSettings = settings_;
  const String previousAdminHash = adminPasswordHash_;
  const bool previousConfigured = configured_;

  // save() rewrites every key, so the namespace holds the defaults afterwards
  // without the window an up-front clear() would open.
  wifi_ = WiFiConfig{};
  mqtt_ = MqttConfig{};
  ntp_ = NtpConfig{};
  settings_ = ControllerSettings{};
  adminPasswordHash_ = kDefaultPasswordHash;
  configured_ = false;
  if (!save()) {
    wifi_ = previousWifi;
    mqtt_ = previousMqtt;
    ntp_ = previousNtp;
    settings_ = previousSettings;
    adminPasswordHash_ = previousAdminHash;
    configured_ = previousConfigured;
    // A single failed put may have left some keys at their defaults.
    if (!save()) {
      LOG_ERROR("✖ Could not restore the previous configuration in NVS\n");
    }
    return false;
  }
  LOG_INFO("✓ Configuration reset to factory defaults\n");
  return true;
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

namespace {
// Process-wide sensor-mapping cache so hot loops avoid NVS traffic.
bool s_mappingCacheValid = false;
std::uint8_t s_cachedSolarAddress[8] = {0};
std::uint8_t s_cachedPoolAddress[8] = {0};

// Both roles in one value: solar ROM followed by pool ROM.
constexpr const char *kSensorMappingKey = "mapping";
constexpr size_t kSensorMappingRecordSize = 16;

struct SensorMappingReadResult final {
  bool readSucceeded{false};
  bool hasMapping{false};
};

SensorMappingReadResult readSensorMappingFromNvs(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  memset(solarAddr, 0, 8);
  memset(poolAddr, 0, 8);

  Preferences prefs;
  if (!prefs.begin("ds18b20", true)) {
    return {};
  }

  // The combined record is written in one NVS operation, so it always holds a
  // consistent pair. The legacy per-role keys are read only when it is absent.
  std::uint8_t record[kSensorMappingRecordSize] = {0};
  if (prefs.getBytes(kSensorMappingKey, record, sizeof(record)) == sizeof(record)) {
    prefs.end();
    memcpy(solarAddr, record, 8);
    memcpy(poolAddr, record + 8, 8);
    SensorMappingReadResult result;
    result.readSucceeded = true;
    result.hasMapping = true;
    return result;
  }

  const size_t solarLength = prefs.getBytes("solar_adr", solarAddr, 8);
  const size_t poolLength = prefs.getBytes("pool_adr", poolAddr, 8);
  prefs.end();

  if (solarLength != 8) {
    memset(solarAddr, 0, 8);
  }
  if (poolLength != 8) {
    memset(poolAddr, 0, 8);
  }

  SensorMappingReadResult result;
  result.readSucceeded = true;
  result.hasMapping = solarLength == 8 || poolLength == 8;
  return result;
}
}  // namespace

bool ConfigManager::saveSensorMapping(const uint8_t solarAddr[8], const uint8_t poolAddr[8]) {
  s_mappingCacheValid = false;
  Preferences prefs;
  if (!prefs.begin("ds18b20", false)) {  // read-write
    LOG_ERROR("✖ Failed to open NVS sensor-mapping namespace for writing\n");
    return false;
  }
  // A single putBytes() is all-or-nothing in NVS, so a failed write leaves the
  // previous pair in place instead of a new solar and an old pool ROM.
  std::uint8_t record[kSensorMappingRecordSize];
  memcpy(record, solarAddr, 8);
  memcpy(record + 8, poolAddr, 8);
  const bool written = prefs.putBytes(kSensorMappingKey, record, sizeof(record)) == sizeof(record);
  if (written) {
    // The record takes precedence; dropping the legacy keys only tidies up.
    prefs.remove("solar_adr");
    prefs.remove("pool_adr");
  }
  prefs.end();
  if (!written) {
    LOG_ERROR("✖ Failed to write sensor mapping to NVS\n");
    return false;
  }

  char buf[17];
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", solarAddr[0], solarAddr[1], solarAddr[2], solarAddr[3],
    solarAddr[4], solarAddr[5], solarAddr[6], solarAddr[7]);
  LOG_INFO("✓ Sensor mapping saved: Solar [%s]", buf);
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", poolAddr[0], poolAddr[1], poolAddr[2], poolAddr[3], poolAddr[4],
    poolAddr[5], poolAddr[6], poolAddr[7]);
  LOG_INFO(", Pool [%s]\n", buf);
  return true;
}

bool ConfigManager::loadSensorMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  // Preserve the legacy return contract: true means at least one role exists.
  return readSensorMappingFromNvs(solarAddr, poolAddr).hasMapping;
}

bool ConfigManager::loadSensorMappingCached(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  if (!s_mappingCacheValid) {
    std::uint8_t solarCandidate[8] = {0};
    std::uint8_t poolCandidate[8] = {0};
    const SensorMappingReadResult readResult = readSensorMappingFromNvs(solarCandidate, poolCandidate);
    if (!readResult.readSucceeded) {
      // Do not poison the process-wide cache on a transient NVS failure.
      memset(solarAddr, 0, 8);
      memset(poolAddr, 0, 8);
      return false;
    }
    memcpy(s_cachedSolarAddress, solarCandidate, 8);
    memcpy(s_cachedPoolAddress, poolCandidate, 8);
    s_mappingCacheValid = true;
  }
  memcpy(solarAddr, s_cachedSolarAddress, 8);
  memcpy(poolAddr, s_cachedPoolAddress, 8);
  return true;
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
