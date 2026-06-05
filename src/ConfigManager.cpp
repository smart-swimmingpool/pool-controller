// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "ConfigManager.hpp"
#include "Version.h"
#include <LittleFS.h>
#include <Preferences.h>
#include <mbedtls/md.h>

namespace PoolController {

WiFiConfig ConfigManager::wifi_;
MqttConfig ConfigManager::mqtt_;
NtpConfig ConfigManager::ntp_;
ControllerSettings ConfigManager::settings_;
String ConfigManager::adminPasswordHash_ = "";
bool ConfigManager::configured_ = false;
bool ConfigManager::configRestored_ = false;

static String hashSha256(const String &input) {
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const uint8_t *)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  String result = "";
  for (int i = 0; i < 32; i++) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", hash[i]);
    result += buf;
  }
  return result;
}

static const String &defaultPasswordHash() {
  static const String kDefaultPasswordHash = hashSha256("admin");
  return kDefaultPasswordHash;
}

// ── OTA-Safe Config Parsing ──

bool ConfigManager::parseDocument(JsonDocument &doc) {
  wifi_.ssid = doc["wifi"]["ssid"] | "";
  wifi_.password = doc["wifi"]["password"] | "";

  mqtt_.host = doc["mqtt"]["host"] | "";
  mqtt_.port = doc["mqtt"]["port"] | 1883;
  mqtt_.username = doc["mqtt"]["username"] | "";
  mqtt_.password = doc["mqtt"]["password"] | "";
  mqtt_.useTls = doc["mqtt"]["use_tls"] | false;

  ntp_.server = doc["ntp"]["server"] | "pool.ntp.org";
  ntp_.timezone = doc["ntp"]["timezone"] | 0;

  settings_.loopInterval = doc["settings"]["loop_interval"] | 10;
  settings_.tempMaxPool = doc["settings"]["temp_max_pool"] | 28.5;
  settings_.tempMinSolar = doc["settings"]["temp_min_solar"] | 55.0;
  settings_.tempHysteresis = doc["settings"]["temp_hysteresis"] | 1.0;
  settings_.opMode = doc["settings"]["op_mode"] | "auto";
  settings_.timezoneIndex = doc["settings"]["timezone_index"] | 0;
  settings_.timeLossGreenHours = doc["settings"]["time_loss_green_hours"] | 1;
  settings_.timeLossRedHours = doc["settings"]["time_loss_red_hours"] | 24;

  adminPasswordHash_ = doc["admin_password_hash"] | defaultPasswordHash();
  configured_ = doc["configured"] | false;

  if (configRestored_) {
    Serial.println("✓ Config restored from OTA backup — all parameters intact");
  } else {
    Serial.println("✓ Configuration loaded successfully");
  }
  return true;
}

// ── Lifecycle ──

bool ConfigManager::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("✖ LittleFS Mount Failed! Formatting...");
    if (!LittleFS.format()) {
      Serial.println("✖ LittleFS Format Failed!");
      return false;
    }
    if (!LittleFS.begin(true)) {
      Serial.println("✖ LittleFS Mount Failed after format!");
      return false;
    }
  }
  Serial.println("✓ LittleFS mounted successfully");
  return load();
}

bool ConfigManager::load() {
  configRestored_ = false;

  // Attempt to load the main config file
  if (LittleFS.exists(kConfigPath)) {
    File configFile = LittleFS.open(kConfigPath, "r");
    if (configFile) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, configFile);
      configFile.close();

      if (!error) {
        return parseDocument(doc);
      }
      Serial.printf("✖ Config deserialization failed: %s\n", error.c_str());
    } else {
      Serial.println("✖ Failed to open config file for reading");
    }
  } else {
    Serial.println("⚠ Configuration file does not exist");
  }

  // ── Config missing or corrupt — try OTA backup restore ──
  if (LittleFS.exists(kConfigBackupPath)) {
    Serial.println("→ Attempting restore from OTA backup (config.json.ota)...");
    File backupFile = LittleFS.open(kConfigBackupPath, "r");
    if (backupFile) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, backupFile);
      backupFile.close();

      if (!error) {
        // Restore: write backup over main config, then parse
        File restoreFile = LittleFS.open(kConfigPath, "w");
        if (restoreFile) {
          serializeJson(doc, restoreFile);
          restoreFile.close();
          configRestored_ = true;
          Serial.println("✓ OTA backup valid — restored to config.json");

          return parseDocument(doc);
        }
      }
      Serial.println("✖ OTA backup is also corrupt — cannot restore config");
    }
  } else {
    Serial.println("  (no OTA backup at " + String(kConfigBackupPath) + ")");
  }

  // Last resort: factory defaults
  Serial.println("⚠ Using factory defaults — all settings will be lost");
  reset();
  return false;
}

bool ConfigManager::save() {
  File configFile = LittleFS.open(kConfigPath, "w");
  if (!configFile) {
    Serial.println("✖ Failed to open config file for writing");
    return false;
  }

  JsonDocument doc;

  JsonObject wifiObj = doc["wifi"].to<JsonObject>();
  wifiObj["ssid"] = wifi_.ssid;
  wifiObj["password"] = wifi_.password;

  JsonObject mqttObj = doc["mqtt"].to<JsonObject>();
  mqttObj["host"] = mqtt_.host;
  mqttObj["port"] = mqtt_.port;
  mqttObj["username"] = mqtt_.username;
  mqttObj["password"] = mqtt_.password;
  mqttObj["use_tls"] = mqtt_.useTls;

  JsonObject ntpObj = doc["ntp"].to<JsonObject>();
  ntpObj["server"] = ntp_.server;
  ntpObj["timezone"] = ntp_.timezone;

  JsonObject settingsObj = doc["settings"].to<JsonObject>();
  settingsObj["loop_interval"] = settings_.loopInterval;
  settingsObj["temp_max_pool"] = settings_.tempMaxPool;
  settingsObj["temp_min_solar"] = settings_.tempMinSolar;
  settingsObj["temp_hysteresis"] = settings_.tempHysteresis;
  settingsObj["op_mode"] = settings_.opMode;
  settingsObj["timezone_index"] = settings_.timezoneIndex;
  settingsObj["time_loss_green_hours"] = settings_.timeLossGreenHours;
  settingsObj["time_loss_red_hours"] = settings_.timeLossRedHours;

  doc["admin_password_hash"] = adminPasswordHash_;
  doc["configured"] = configured_;

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("✖ Failed to write config to file");
    configFile.close();
    return false;
  }

  configFile.close();

  // Also update the OTA backup so it reflects the latest settings
  File backupFile = LittleFS.open(kConfigBackupPath, "w");
  if (backupFile) {
    serializeJson(doc, backupFile);
    backupFile.close();
  }

  Serial.println("✓ Configuration saved successfully");
  return true;
}

void ConfigManager::reset() {
  wifi_ = WiFiConfig{};
  mqtt_ = MqttConfig{};
  ntp_ = NtpConfig{};
  settings_ = ControllerSettings{};
  adminPasswordHash_ = kDefaultPasswordHash;
  configured_ = false;
}

// ── OTA-Safe Config: Backup + Restore ──

bool ConfigManager::backupConfig() {
  if (!LittleFS.exists(kConfigPath)) {
    Serial.println("⚠ backupConfig: no config.json to back up");
    return false;
  }

  File src = LittleFS.open(kConfigPath, "r");
  if (!src) {
    Serial.println("✖ backupConfig: cannot open config.json for reading");
    return false;
  }

  File dst = LittleFS.open(kConfigBackupPath, "w");
  if (!dst) {
    Serial.println("✖ backupConfig: cannot create config.json.ota");
    src.close();
    return false;
  }

  // Copy byte by byte via buffer
  uint8_t buf[256];
  size_t bytesRead;
  while ((bytesRead = src.read(buf, sizeof(buf))) > 0) {
    dst.write(buf, bytesRead);
  }

  src.close();
  dst.close();

  Serial.printf("✓ Config backed up to %s for OTA safety\n", kConfigBackupPath);
  return true;
}

bool ConfigManager::restoreConfig() {
  if (!LittleFS.exists(kConfigBackupPath)) {
    Serial.println("✖ restoreConfig: no OTA backup found");
    return false;
  }

  File src = LittleFS.open(kConfigBackupPath, "r");
  if (!src) {
    Serial.println("✖ restoreConfig: cannot open OTA backup");
    return false;
  }

  File dst = LittleFS.open(kConfigPath, "w");
  if (!dst) {
    Serial.println("✖ restoreConfig: cannot write config.json");
    src.close();
    return false;
  }

  uint8_t buf[256];
  size_t bytesRead;
  while ((bytesRead = src.read(buf, sizeof(buf))) > 0) {
    dst.write(buf, bytesRead);
  }

  src.close();
  dst.close();

  Serial.printf("✓ Config restored from OTA backup (%s → %s)\n", kConfigBackupPath, kConfigPath);
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
    Serial.printf("ℹ First boot — firmware version %s\n", runningVersion.c_str());
  } else if (previousVersion != runningVersion) {
    // Version changed — OTA update just happened
    Serial.printf("◉ OTA UPDATE DETECTED: %s → %s\n", previousVersion.c_str(), runningVersion.c_str());

    Serial.printf("  ◉ Config backup:   %s\n", LittleFS.exists(kConfigBackupPath) ? "present ✓" : "not found");

    Serial.printf("  ◉ Config integrity: %s\n", configured_ ? "valid ✓" : "INVALID — using defaults");

    if (configRestored_) {
      Serial.println("  ◉ Config source: restored from OTA backup (auto) ✓");
    } else if (configured_) {
      Serial.println("  ◉ Config source: original config.json (intact) ✓");
    }

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
