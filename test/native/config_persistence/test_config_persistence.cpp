// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_config_persistence.cpp
 * @brief Tests the real ConfigManager against the Preferences mock when a
 *        single NVS write fails.
 */

#include <cstdio>
#include <cstring>
#include <vector>

#include "ConfigManager.hpp"

using PoolController::ConfigManager;

namespace {

int g_passed = 0;
int g_failed = 0;

void check(bool cond, const char *file, int line, const char *expr) {
  if (cond) {
    g_passed++;
  } else {
    g_failed++;
    fprintf(stderr, "  ✗ FAIL %s:%d — %s\n", file, line, expr);
  }
}

#define EXPECT(cond) check((cond), __FILE__, __LINE__, #cond)

void section(const char *name) {
  printf("\n== %s ==\n", name);
}

const uint8_t kSolarA[8] = {0x28, 0x01, 0, 0, 0, 0, 0, 0xA1};
const uint8_t kPoolA[8] = {0x28, 0x02, 0, 0, 0, 0, 0, 0xB2};
const uint8_t kSolarB[8] = {0x28, 0x03, 0, 0, 0, 0, 0, 0xC3};
const uint8_t kPoolB[8] = {0x28, 0x04, 0, 0, 0, 0, 0, 0xD4};

void resetStore() {
  Preferences prefs;
  prefs.begin("config", false);
  prefs.clear();
  prefs.end();
  Preferences::s_failingPutKeys.clear();
}

/** Store a non-default configuration and reload it from NVS. */
void storeCustomConfig() {
  ConfigManager::getSettings().tempMaxPool = 30.5;
  ConfigManager::getSettings().tempHysteresis = 2.0;
  ConfigManager::getWiFi().ssid = "pool-net";
  ConfigManager::setConfigured(true);
  EXPECT(ConfigManager::save());
  EXPECT(ConfigManager::load());
}

void test_failed_record_write_fails_save() {
  section("save() reports a failed record write");
  resetStore();
  ConfigManager::getSettings() = PoolController::ControllerSettings{};
  EXPECT(ConfigManager::save());

  Preferences::s_failingPutKeys.insert("cfg_record");
  EXPECT(!ConfigManager::save());

  Preferences::s_failingPutKeys.clear();
  EXPECT(ConfigManager::save());
}

void test_failed_factory_reset_restores_nvs() {
  section("failed factory reset leaves the previous configuration in NVS");
  resetStore();
  storeCustomConfig();

  Preferences::s_failingPutKeys.insert("cfg_record");
  EXPECT(!ConfigManager::resetToDefaults());
  EXPECT(ConfigManager::getSettings().tempMaxPool == 30.5);
  EXPECT(ConfigManager::isConfigured());

  // After a reboot the device must come up with the old values, not a mix.
  Preferences::s_failingPutKeys.clear();
  ConfigManager::getSettings() = PoolController::ControllerSettings{};
  ConfigManager::setConfigured(false);
  EXPECT(ConfigManager::load());
  EXPECT(ConfigManager::getSettings().tempMaxPool == 30.5);
  EXPECT(ConfigManager::getSettings().tempHysteresis == 2.0);
  EXPECT(ConfigManager::getWiFi().ssid == "pool-net");
  EXPECT(ConfigManager::isConfigured());
}

void test_record_round_trips_every_field() {
  section("the record restores every persisted field");
  resetStore();
  ConfigManager::getWiFi().ssid = "pool-net";
  ConfigManager::getWiFi().password = "w1f1-secret";
  ConfigManager::getMqtt().host = "broker.local";
  ConfigManager::getMqtt().port = 8883;
  ConfigManager::getMqtt().username = "pool";
  ConfigManager::getMqtt().password = "mqtt-secret";
  ConfigManager::getNtp().server = "ntp.example.org";
  ConfigManager::getNtp().timezone = -3600;
  PoolController::ControllerSettings &s = ConfigManager::getSettings();
  s.loopInterval = 15;
  s.tempMaxPool = 29.25;
  s.tempMinSolar = 48.5;
  s.tempHysteresis = 0.75;
  s.tempCircThreshold = 26.5;
  s.tempCircFactor = 42;
  s.tempCircMaxRuntime = 600;
  s.opMode = "timer";
  s.timeLossGreenHours = 2;
  s.timeLossRedHours = 36;
  s.timezoneIndex = 3;
  s.btn1Min = 3000;
  s.btn1Max = 3400;
  s.btn2Min = 3400;
  s.btn2Max = 3800;
  s.btn3Min = 3800;
  s.btn3Max = 4000;
  s.btnNoPress = 4090;
  ConfigManager::setAdminPassword("pool-admin");
  const String adminHash = ConfigManager::getAdminPasswordHash();
  ConfigManager::setConfigured(true);
  EXPECT(ConfigManager::save());

  // Start from defaults so every field below must come from the record.
  ConfigManager::getWiFi() = PoolController::WiFiConfig{};
  ConfigManager::getMqtt() = PoolController::MqttConfig{};
  ConfigManager::getNtp() = PoolController::NtpConfig{};
  s = PoolController::ControllerSettings{};
  ConfigManager::setAdminPassword("admin");
  ConfigManager::setConfigured(false);
  EXPECT(ConfigManager::load());

  EXPECT(ConfigManager::getWiFi().ssid == "pool-net");
  EXPECT(ConfigManager::getWiFi().password == "w1f1-secret");
  EXPECT(ConfigManager::getMqtt().host == "broker.local");
  EXPECT(ConfigManager::getMqtt().port == 8883);
  EXPECT(ConfigManager::getMqtt().username == "pool");
  EXPECT(ConfigManager::getMqtt().password == "mqtt-secret");
  EXPECT(ConfigManager::getNtp().server == "ntp.example.org");
  EXPECT(ConfigManager::getNtp().timezone == -3600);
  EXPECT(s.loopInterval == 15);
  EXPECT(s.tempMaxPool == 29.25);
  EXPECT(s.tempMinSolar == 48.5);
  EXPECT(s.tempHysteresis == 0.75);
  EXPECT(s.tempCircThreshold == 26.5);
  EXPECT(s.tempCircFactor == 42);
  EXPECT(s.tempCircMaxRuntime == 600);
  EXPECT(s.opMode == "timer");
  EXPECT(s.timeLossGreenHours == 2);
  EXPECT(s.timeLossRedHours == 36);
  EXPECT(s.timezoneIndex == 3);
  EXPECT(s.btn1Min == 3000);
  EXPECT(s.btn1Max == 3400);
  EXPECT(s.btn2Min == 3400);
  EXPECT(s.btn2Max == 3800);
  EXPECT(s.btn3Min == 3800);
  EXPECT(s.btn3Max == 4000);
  EXPECT(s.btnNoPress == 4090);
  EXPECT(ConfigManager::getAdminPasswordHash() == adminHash);
  EXPECT(ConfigManager::verifyAdminPassword("pool-admin"));
  EXPECT(ConfigManager::isConfigured());
}

void writeLegacyKeys() {
  Preferences prefs;
  prefs.begin("config", false);
  prefs.putString("wifi_ssid", "legacy-net");
  prefs.putDouble("set_maxpool", 27.0);
  prefs.putString("set_opmode", "manu");
  prefs.putBool("cfg_configured", true);
  prefs.end();
}

void test_legacy_keys_are_migrated() {
  section("older per-key configuration is read until the first record is saved");
  resetStore();
  writeLegacyKeys();
  ConfigManager::getSettings() = PoolController::ControllerSettings{};
  EXPECT(ConfigManager::load());
  EXPECT(ConfigManager::getWiFi().ssid == "legacy-net");
  EXPECT(ConfigManager::getSettings().tempMaxPool == 27.0);
  EXPECT(ConfigManager::getSettings().opMode == "manu");
  EXPECT(ConfigManager::getSettings().tempHysteresis == 1.0);  // absent key keeps its default
  EXPECT(ConfigManager::isConfigured());

  ConfigManager::getSettings().tempMaxPool = 31.0;
  EXPECT(ConfigManager::save());
  EXPECT(ConfigManager::load());
  EXPECT(ConfigManager::getSettings().tempMaxPool == 31.0);  // the record wins over the old key
  EXPECT(ConfigManager::getWiFi().ssid == "legacy-net");
}

/** Overwrite one byte of the stored record. */
void corruptRecord(size_t offset, uint8_t value) {
  Preferences prefs;
  prefs.begin("config", false);
  const size_t length = prefs.getBytesLength("cfg_record");
  std::vector<uint8_t> record(length);
  prefs.getBytes("cfg_record", record.data(), length);
  record[offset] = value;
  prefs.putBytes("cfg_record", record.data(), length);
  prefs.end();
}

void test_invalid_record_falls_back_to_legacy_keys() {
  section("a corrupted or unknown record is never applied");
  resetStore();
  writeLegacyKeys();
  EXPECT(ConfigManager::load());
  ConfigManager::getSettings().tempMaxPool = 31.0;
  EXPECT(ConfigManager::save());

  corruptRecord(12, 0x5A);  // payload byte: CRC mismatch
  EXPECT(ConfigManager::load());
  EXPECT(ConfigManager::getSettings().tempMaxPool == 27.0);

  EXPECT(ConfigManager::save());
  corruptRecord(4, 0x7F);  // version written by unknown firmware
  EXPECT(ConfigManager::load());
  EXPECT(ConfigManager::getSettings().tempMaxPool == 27.0);
  EXPECT(ConfigManager::getWiFi().ssid == "legacy-net");
}

void test_mapping_is_written_as_one_record() {
  section("a failed mapping write keeps the previous pair");
  resetStore();
  EXPECT(ConfigManager::saveSensorMapping(kSolarA, kPoolA));

  Preferences::s_failingPutKeys.insert("mapping");
  EXPECT(!ConfigManager::saveSensorMapping(kSolarB, kPoolB));

  uint8_t solar[8];
  uint8_t pool[8];
  EXPECT(ConfigManager::loadSensorMapping(solar, pool));
  EXPECT(memcmp(solar, kSolarA, 8) == 0);  // not the new solar ROM ...
  EXPECT(memcmp(pool, kPoolA, 8) == 0);    // ... next to the old pool ROM
  EXPECT(ConfigManager::loadSensorMappingCached(solar, pool));
  EXPECT(memcmp(solar, kSolarA, 8) == 0);
}

void test_legacy_mapping_keys_are_migrated() {
  section("legacy per-role keys are read and replaced by the record");
  resetStore();
  {
    Preferences prefs;
    prefs.begin("ds18b20", false);
    prefs.putBytes("solar_adr", kSolarA, 8);
    prefs.putBytes("pool_adr", kPoolA, 8);
    prefs.end();
  }

  uint8_t solar[8];
  uint8_t pool[8];
  EXPECT(ConfigManager::loadSensorMapping(solar, pool));
  EXPECT(memcmp(solar, kSolarA, 8) == 0);
  EXPECT(memcmp(pool, kPoolA, 8) == 0);

  EXPECT(ConfigManager::saveSensorMapping(kSolarB, kPoolB));
  Preferences prefs;
  prefs.begin("ds18b20", true);
  EXPECT(!prefs.isKey("solar_adr"));
  EXPECT(!prefs.isKey("pool_adr"));
  prefs.end();
  EXPECT(ConfigManager::loadSensorMapping(solar, pool));
  EXPECT(memcmp(solar, kSolarB, 8) == 0);
  EXPECT(memcmp(pool, kPoolB, 8) == 0);
}

}  // namespace

int main() {
  test_failed_record_write_fails_save();
  test_failed_factory_reset_restores_nvs();
  test_record_round_trips_every_field();
  test_legacy_keys_are_migrated();
  test_invalid_record_falls_back_to_legacy_keys();
  test_mapping_is_written_as_one_record();
  test_legacy_mapping_keys_are_migrated();

  printf("\n══════════════════════════════════════════════════\n");
  printf("  Config Persistence Results: %d passed, %d failed\n", g_passed, g_failed);
  printf("══════════════════════════════════════════════════\n");

  return g_failed == 0 ? 0 : 1;
}
