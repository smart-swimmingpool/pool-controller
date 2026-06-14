/**
 * @file test_config_manager.cpp
 * @brief Unit tests for ConfigManager — settings load/save round-trip.
 */

#include <stdio.h>
#include <string.h>
#include "Arduino.h"
#include "ConfigManager.hpp"

using namespace PoolController;  // NOLINT(build/namespaces)

extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_TRUE(cond) do { \
  if (!(cond)) { \
    test_fail(__FILE__, __LINE__, "Expected true: " #cond); \
    return 1; \
  } \
  test_pass(__FILE__, __LINE__); \
} while (0)

#define ASSERT_EQ(a, b) do { \
  auto _a = (a); auto _b = (b); \
  if (_a != _b) { \
    char _msg[256]; snprintf(_msg, sizeof(_msg), "Expected %s == %s: got %lld vs %lld", #a, #b, (long long)_a, (long long)_b); \
    test_fail(__FILE__, __LINE__, _msg); return 1; \
  } \
  test_pass(__FILE__, __LINE__); \
} while (0)

int run_config_manager_tests() {
  int passed = 0, failed = 0;
  int rc;

  // ── Test: Default settings ──
  {
    test_begin("ConfigManager", "default settings");
    ConfigManager::load();
    Settings &s = ConfigManager::getSettings();

    int errs = 0;
    if (s.opMode != "auto") { test_fail(__FILE__, __LINE__, "opMode should be auto"); errs++; }  // NOLINT
    else { test_pass(__FILE__, __LINE__); passed++; }  // NOLINT

    rc = (errs == 0) ? 0 : 1;
    if (rc != 0) failed++;
    test_suite_end("ConfigManager::defaults", errs == 0 ? 1 : 0, errs);
  }

  // ── Test: WiFi config ──
  {
    test_begin("ConfigManager", "WiFi config access");
    WiFiConfig &w = ConfigManager::getWiFi();
    ASSERT_TRUE(w.ssid.length() > 0);  // NOLINT(readability/check)
    ASSERT_TRUE(w.password.length() > 0);  // NOLINT(readability/check)
    test_suite_end("ConfigManager::wifi", 1, 0);
    passed++;
  }

  // ── Test: MQTT config ──
  {
    test_begin("ConfigManager", "MQTT config access");
    MqttConfig &m = ConfigManager::getMqtt();
    ASSERT_TRUE(m.host.length() > 0);  // NOLINT(readability/check)
    ASSERT_EQ(m.port, 1883);
    test_suite_end("ConfigManager::mqtt", 1, 0);
    passed++;
  }

  return passed + failed;
}
