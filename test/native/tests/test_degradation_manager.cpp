/**
 * @file test_degradation_manager.cpp
 * @brief Unit tests for DegradationManager — thread-safe sensor status
 *        reporting (reportSensorStatus from SensorTask) and safe-mode
 *        transitions.
 */

#include <stdio.h>
#include <string.h>
#include <thread>
#include <chrono>

#include "DegradationManager.hpp"
#include "NetworkManager.hpp"

using namespace PoolController;  // NOLINT(build/namespaces)

extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_TRUE(cond)                                     \
  do {                                                        \
    if (!(cond)) {                                            \
      test_fail(__FILE__, __LINE__, "Expected true: " #cond); \
      return 1;                                               \
    }                                                         \
    test_pass(__FILE__, __LINE__);                            \
  } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b)                                                                                          \
  do {                                                                                                           \
    auto _a = (a);                                                                                               \
    auto _b = (b);                                                                                               \
    if (_a != _b) {                                                                                              \
      char _msg[256];                                                                                            \
      snprintf(_msg, sizeof(_msg), "Expected %s == %s: got %lld vs %lld", #a, #b, (long long)_a, (long long)_b); \
      test_fail(__FILE__, __LINE__, _msg);                                                                       \
      return 1;                                                                                                  \
    }                                                                                                            \
    test_pass(__FILE__, __LINE__);                                                                               \
  } while (0)

int run_degradation_manager_tests() {
  int passed = 0, failed = 0;

  // ── Test: safe-mode round-trip (no rate limit involved) ──
  {
    DegradationManager::begin();
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
    ASSERT_FALSE(DegradationManager::isSafe());

    DegradationManager::forceSafeMode();
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::CRITICAL);
    ASSERT_TRUE(DegradationManager::isSafe());

    DegradationManager::unforceSafeMode();
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
    ASSERT_FALSE(DegradationManager::isSafe());

    test_suite_end("DegradationManager::safe_mode", 1, 0);
    passed++;
  }

  // ── Test: reportSensorStatus drives the sensor level (thread-safe path) ──
  {
    // WiFi up, time GREEN (stub), memory healthy (mock): only the sensor
    // flags decide between NORMAL and NO_SENSOR.
    NetworkManager::setWiFiConnected(true);
    DegradationManager::begin();

    DegradationManager::reportSensorStatus("pool-temp", true);
    DegradationManager::reportSensorStatus("solar-temp", true);
    DegradationManager::evaluate();  // first evaluation runs immediately
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);

    // One probe failing → NO_SENSOR. evaluate() is rate-limited to 5 s,
    // so wait past the interval before the next call.
    DegradationManager::reportSensorStatus("pool-temp", false);
    std::this_thread::sleep_for(std::chrono::milliseconds(5100));
    DegradationManager::evaluate();
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_SENSOR);

    // Recovery restores NORMAL.
    DegradationManager::reportSensorStatus("pool-temp", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(5100));
    DegradationManager::evaluate();
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);

    test_suite_end("DegradationManager::sensor_status", 1, 0);
    passed++;
  }

  (void)passed;
  (void)failed;
  return 0;
}
