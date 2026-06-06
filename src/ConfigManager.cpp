// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "ConfigManager.hpp"
#include "Version.h"
#include <Preferences.h>
#include <mbedtls/md.h>

namespace PoolController {

WiFiConfig ConfigManager::wifi_;
MqttConfig ConfigManager::mqtt_;
NtpConfig ConfigManager::ntp_;
ControllerSettings ConfigManager::settings_;
String ConfigManager::adminPasswordHash_ = "";
bool ConfigManager::configured_ = false;

// Default password is "admin"
static constexpr const char *kDefaultPasswordHash = "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";

static String hashSha256(const String &input) {
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const uint8_t *)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  char result[65];
  for (int i = 0; i < 32; i++) {
    snprintf(result + (i * 2), 3, "%02x", hash[i]);
  }
  result[64] = '\0';
  return String(result);
}

// ── NVS Key Names ──

static constexpr const char *kNvsNamespace = "config";
static constexpr const char *kWifiSsid      = "wifi_ssid";
static constexpr const char *kWifiPass      = "wifi_pass";
static constexpr const char *kMqttHost      = "mqtt_host";
static constexpr const char *kMqttPort      = "mqtt_port";
static constexpr const char *kMqttUser      = "mqtt_user";
static constexpr const char *kMqttPass      = "mqtt_pass";
static constexpr const char *kMqttTls       = "mqtt_tls";
static constexpr const char *kNtpServer     = "ntp_server";
static constexpr const char *kNtpTz         = "ntp_tz";
static constexpr const char *kSetInterval   = "set_interval";
static constexpr const char *kSetMaxPool    = "set_maxpool";
static constexpr const char *kSetMinSolar   = "set_minsolar";
static constexpr const char *kSetHyst       = "set_hyst";
static constexpr const char *kSetOpMode     = "set_opmode";
static constexpr const char *kSetTzIdx      = "set_tzidx";
static constexpr const char *kSetGreen      = "set_green";
static constexpr const char *kSetRed        = "set_red";
static constexpr const char *kAdmPass       = "adm_pass";
static constexpr const char *kCfgConfigured = "cfg_configured";

// ── Lifecycle ──

bool ConfigManager::begin() {
  Serial.println("✓ NVS config namespace opened");
  return load();
}

bool ConfigManager::load() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true)) {  // read-only mode
    Serial.println("✖ Failed to open NVS config namespace");
    reset();
    return false;
  }

  wifi_.ssid       = prefs.getString(kWifiSsid, "");
  wifi_.password   = prefs.getString(kWifiPass, "");

  mqtt_.host       = prefs.getString(kMqttHost, "");
  mqtt_.port       = prefs.getUInt(kMqttPort, 1883);
  mqtt_.username   = prefs.getString(kMqttUser, "");
  mqtt_.password   = prefs.getString(kMqttPass, "");
  mqtt_.useTls     = prefs.getBool(kMqttTls, false);

  ntp_.server      = prefs.getString(kNtpServer, "pool.ntp.org");
  ntp_.timezone    = prefs.getLong(kNtpTz, 0);

  settings_.loopInterval       = prefs.getLong(kSetInterval, 10);
  settings_.tempMaxPool        = prefs.getDouble(kSetMaxPool, 28.5);
  settings_.tempMinSolar       = prefs.getDouble(kSetMinSolar, 55.0);
  settings_.tempHysteresis     = prefs.getDouble(kSetHyst, 1.0);
  settings_.opMode             = prefs.getString(kSetOpMode, "auto");
  settings_.timezoneIndex      = prefs.getInt(kSetTzIdx, 0);
  settings_.timeLossGreenHours = prefs.getUChar(kSetGreen, 1);
  settings_.timeLossRedHours   = prefs.getUChar(kSetRed, 24);

  adminPasswordHash_ = prefs.getString(kAdmPass, kDefaultPasswordHash);
  configured_        = prefs.getBool(kCfgConfigured, false);

  prefs.end();

  Serial.println("✓ Configuration loaded from NVS");
  return true;
}

bool ConfigManager::save() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {  // read-write mode
    Serial.println("✖ Failed to open NVS config namespace for writing");
    return false;
  }

  prefs.putString(kWifiSsid, wifi_.ssid);
  prefs.putString(kWifiPass, wifi_.password);

  prefs.putString(kMqttHost, mqtt_.host);
  prefs.putUInt(kMqttPort, mqtt_.port);
  prefs.putString(kMqttUser, mqtt_.username);
  prefs.putString(kMqttPass, mqtt_.password);
  prefs.putBool(kMqttTls, mqtt_.useTls);

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

  prefs.putString(kAdmPass, adminPasswordHash_);
  prefs.putBool(kCfgConfigured, configured_);

  prefs.end();

  Serial.println("✓ Configuration saved to NVS");
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
  adminPasswordHash_ = kDefaultPasswordHash;
  configured_ = false;

  Serial.println("✓ Configuration reset to factory defaults");
}

// ── Boot Version Tracking ──

void ConfigManager::logOtaTransition() {
  Preferences prefs;
  prefs.begin("ota-version", false);  // namespace "ota-version" in NVS

  String previousVersion = prefs.getString("fw_version", "");
  String runningVersion = FW_VERSION;

  if (previousVersion.isEmpty()) {
    // First boot ever — nothing to compare
    Serial.printf("ℹ First boot — firmware version %s\n", runningVersion.c_str());
  } else if (previousVersion != runningVersion) {
    // Version changed — OTA update just happened
    Serial.printf("◉ OTA UPDATE DETECTED: %s → %s\n", previousVersion.c_str(), runningVersion.c_str());

    // Update stored version to match running version
    prefs.putString("fw_version", runningVersion);
  } else {
    // Normal boot — same version
    Serial.printf("ℹ Normal boot — firmware %s (no OTA change)\n", runningVersion.c_str());
  }

  prefs.end();
}

void ConfigManager::setAdminPassword(const String &newPassword) {
  adminPasswordHash_ = hashSha256(newPassword);
}

bool ConfigManager::verifyAdminPassword(const String &password) {
  return hashSha256(password) == adminPasswordHash_;
}

}  // namespace PoolController
