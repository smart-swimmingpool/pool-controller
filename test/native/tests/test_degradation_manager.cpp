/**
 * @file test_degradation_manager.cpp
 * @brief Unit tests for DegradationManager — thread-safe sensor status
 *        reporting (reportSensorStatus from SensorTask) and safe-mode
 *        transitions.
 */

#include <stdio.h>
#include <string.h>

#include "DegradationManager.hpp"
#include "NetworkManager.hpp"
#include "test_assert.h"

using namespace PoolController;  // NOLINT(build/namespaces)

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
    // so advance the mock clock past the interval before the next call.
    DegradationManager::reportSensorStatus("pool-temp", false);
    advanceMillisForTest(5100);
    DegradationManager::evaluate();
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_SENSOR);

    // Recovery restores NORMAL.
    DegradationManager::reportSensorStatus("pool-temp", true);
    advanceMillisForTest(5100);
    DegradationManager::evaluate();
    ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);

    test_suite_end("DegradationManager::sensor_status", 1, 0);
    passed++;
  }

  (void)passed;
  (void)failed;
  return 0;
}
