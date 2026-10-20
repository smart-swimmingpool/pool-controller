// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_config_persistence.cpp
 * @brief Tests the real ConfigManager against the Preferences mock when a
 *        single NVS write fails.
 */

#include <cstdio>
#include <cstring>

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

void test_single_failed_put_fails_save() {
  section("save() reports a single failed put");
  resetStore();
  ConfigManager::getSettings() = PoolController::ControllerSettings{};
  EXPECT(ConfigManager::save());

  Preferences::s_failingPutKeys.insert("set_hyst");
  EXPECT(!ConfigManager::save());

  Preferences::s_failingPutKeys.clear();
  EXPECT(ConfigManager::save());
}

void test_failed_empty_string_write_is_detected() {
  section("a failed write of an empty string fails save()");
  resetStore();
  storeCustomConfig();
  EXPECT(ConfigManager::getWiFi().ssid == "pool-net");

  ConfigManager::getWiFi().ssid = "";
  Preferences::s_failingPutKeys.insert("wifi_ssid");
  EXPECT(!ConfigManager::save());  // the old SSID would be loaded again on boot

  Preferences::s_failingPutKeys.clear();
  EXPECT(ConfigManager::save());
}

void test_failed_factory_reset_restores_nvs() {
  section("failed factory reset leaves the previous configuration in NVS");
  resetStore();
  storeCustomConfig();

  // The hysteresis write fails; every other key would already hold its default.
  Preferences::s_failingPutKeys.insert("set_hyst");
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
  test_single_failed_put_fails_save();
  test_failed_empty_string_write_is_detected();
  test_failed_factory_reset_restores_nvs();
  test_mapping_is_written_as_one_record();
  test_legacy_mapping_keys_are_migrated();

  printf("\n══════════════════════════════════════════════════\n");
  printf("  Config Persistence Results: %d passed, %d failed\n", g_passed, g_failed);
  printf("══════════════════════════════════════════════════\n");

  return g_failed == 0 ? 0 : 1;
}
