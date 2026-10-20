// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

#include <cstring>

#include "../../../src/DegradationManager.hpp"
#include "../../../src/TimeClientHelper.hpp"

// Test hook implemented in mocks/stubs.cpp.
void setTimeDegradationStateForTest(TimeDegradation state);
#include "NetworkManager.hpp"
#include "Nodes.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"

#include "test_assert.h"

using namespace PoolController;

// ── Reset helper: restore baseline state before every scenario ──
static void resetDegradationState() {
  advanceMillisForTest(5000);             // clear the evaluate() rate limit
  DegradationManager::unforceSafeMode();  // clear a forced boot-loop state from earlier tests
  NetworkManager::setWiFiConnected(true);
  NetworkManager::setMqttConnected(false);
  ESP.setFreeHeapForTest(180000);
  setTimeDegradationStateForTest(TimeDegradation::GREEN);
  DegradationManager::begin();
}

static int test_begin_resets_to_normal() {
  test_begin("DegradationManager", "begin() resets level to NORMAL");
  resetDegradationState();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
  ASSERT_TRUE(!DegradationManager::isSafe());
  return 0;
}

static int test_begin_preserves_forced_critical() {
  test_begin("DegradationManager", "begin() preserves a forced CRITICAL boot-loop safe mode");
  resetDegradationState();
  DegradationManager::forceSafeMode();
  DegradationManager::begin();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::CRITICAL);
  ASSERT_TRUE(DegradationManager::isSafe());
  return 0;
}

static int test_evaluate_normal_with_all_healthy() {
  test_begin("DegradationManager", "evaluate() stays NORMAL with WiFi, time, sensors and heap healthy");
  resetDegradationState();
  DegradationManager::reportSensorStatus("pool-temp", true);
  DegradationManager::reportSensorStatus("solar-temp", true);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
  return 0;
}

static int test_wifi_loss_is_single_failure() {
  test_begin("DegradationManager", "WiFi loss alone degrades to NO_WIFI, not safe mode");
  resetDegradationState();
  NetworkManager::setWiFiConnected(false);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_WIFI);
  ASSERT_TRUE(!DegradationManager::isSafe());
  return 0;
}

static int test_time_loss_with_wifi_is_single_failure() {
  test_begin("DegradationManager", "NTP loss with WiFi up degrades to NO_TIME");
  resetDegradationState();
  setTimeDegradationStateForTest(TimeDegradation::RED);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_TIME);
  ASSERT_TRUE(!DegradationManager::isSafe());
  return 0;
}

static int test_time_loss_without_wifi_is_consequence() {
  test_begin("DegradationManager", "NTP loss without WiFi does not escalate WiFi outage to CRITICAL");
  resetDegradationState();
  NetworkManager::setWiFiConnected(false);
  setTimeDegradationStateForTest(TimeDegradation::RED);
  advanceMillisForTest(5000);
  DegradationManager::evaluate();
  // Single failure: WiFi down; the NTP loss is a consequence of it and the
  // time-branch wins the "most specific level" ordering. Crucially: not CRITICAL.
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_TIME);
  ASSERT_TRUE(!DegradationManager::isSafe());
  return 0;
}

static int test_sensor_fault_is_single_failure() {
  test_begin("DegradationManager", "one invalid probe degrades to NO_SENSOR");
  resetDegradationState();
  DegradationManager::reportSensorStatus("pool-temp", true);
  DegradationManager::reportSensorStatus("solar-temp", false);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_SENSOR);
  return 0;
}

static int test_silent_sensors_treated_as_ok() {
  test_begin("DegradationManager", "sensors that never reported do not count as failure");
  resetDegradationState();
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
  return 0;
}

static int test_low_memory_is_critical() {
  test_begin("DegradationManager", "critically low heap forces safe mode regardless of other signals");
  resetDegradationState();
  ESP.setFreeHeapForTest(1000);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::CRITICAL);
  ASSERT_TRUE(DegradationManager::isSafe());
  return 0;
}

static int test_two_failures_are_not_critical() {
  test_begin("DegradationManager", "WiFi loss plus sensor fault reports the sensor fault without safe mode");
  resetDegradationState();
  NetworkManager::setWiFiConnected(false);
  DegradationManager::reportSensorStatus("pool-temp", false);
  DegradationManager::reportSensorStatus("solar-temp", true);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_SENSOR);
  ASSERT_TRUE(!DegradationManager::isSafe());
  return 0;
}

static int test_critical_deenergizes_relays() {
  test_begin("DegradationManager", "entering safe mode de-energizes both pumps");
  resetDegradationState();
  poolPumpNode.setSwitch(true);
  solarPumpNode.setSwitch(true);
  ESP.setFreeHeapForTest(1000);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::CRITICAL);
  ASSERT_TRUE(!poolPumpNode.getSwitch());
  ASSERT_TRUE(!solarPumpNode.getSwitch());
  return 0;
}

static int test_recovery_downgrades_level() {
  test_begin("DegradationManager", "evaluate() recovers to NORMAL once the fault clears");
  resetDegradationState();
  NetworkManager::setWiFiConnected(false);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_WIFI);
  NetworkManager::setWiFiConnected(true);
  advanceMillisForTest(5000);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
  return 0;
}

static int test_forced_safe_mode_never_downgrades() {
  test_begin("DegradationManager", "forced safe mode is not cleared by healthy signals");
  resetDegradationState();
  DegradationManager::forceSafeMode();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::CRITICAL);
  DegradationManager::reportSensorStatus("pool-temp", true);
  DegradationManager::reportSensorStatus("solar-temp", true);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::CRITICAL);
  return 0;
}

static int test_unforce_recovers() {
  test_begin("DegradationManager", "unforceSafeMode() returns to NORMAL");
  resetDegradationState();
  DegradationManager::forceSafeMode();
  ASSERT_TRUE(DegradationManager::isSafe());
  DegradationManager::unforceSafeMode();
  ASSERT_TRUE(!DegradationManager::isSafe());
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
  return 0;
}

static int test_level_to_string_mapping() {
  test_begin("DegradationManager", "levelToString() maps every level");
  ASSERT_EQ(std::strcmp(DegradationManager::levelToString(DegradationLevel::NORMAL), "normal"), 0);
  ASSERT_EQ(std::strcmp(DegradationManager::levelToString(DegradationLevel::NO_WIFI), "no-wifi"), 0);
  ASSERT_EQ(std::strcmp(DegradationManager::levelToString(DegradationLevel::NO_TIME), "no-time"), 0);
  ASSERT_EQ(std::strcmp(DegradationManager::levelToString(DegradationLevel::NO_SENSOR), "no-sensor"), 0);
  ASSERT_EQ(std::strcmp(DegradationManager::levelToString(DegradationLevel::CRITICAL), "critical"), 0);
  return 0;
}

static int test_sensor_status_tracks_each_probe() {
  test_begin("DegradationManager", "reportSensorStatus() tracks pool and solar probes independently");
  resetDegradationState();
  DegradationManager::reportSensorStatus("pool-temp", false);
  DegradationManager::reportSensorStatus("solar-temp", true);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_SENSOR);
  // Pool probe recovers alone.
  DegradationManager::reportSensorStatus("pool-temp", true);
  advanceMillisForTest(5000);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NORMAL);
  return 0;
}

static int test_evaluate_is_rate_limited() {
  test_begin("DegradationManager", "evaluate() is rate-limited to the evaluation interval");
  resetDegradationState();
  NetworkManager::setWiFiConnected(false);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_WIFI);
  // Health returns, but the rate limit suppresses re-evaluation.
  NetworkManager::setWiFiConnected(true);
  DegradationManager::evaluate();
  ASSERT_EQ(DegradationManager::getLevel(), DegradationLevel::NO_WIFI);
  return 0;
}

int run_degradation_manager_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {
    test_begin_resets_to_normal,
    test_begin_preserves_forced_critical,
    test_evaluate_normal_with_all_healthy,
    test_wifi_loss_is_single_failure,
    test_time_loss_with_wifi_is_single_failure,
    test_time_loss_without_wifi_is_consequence,
    test_sensor_fault_is_single_failure,
    test_silent_sensors_treated_as_ok,
    test_low_memory_is_critical,
    test_two_failures_are_not_critical,
    test_critical_deenergizes_relays,
    test_recovery_downgrades_level,
    test_forced_safe_mode_never_downgrades,
    test_unforce_recovers,
    test_level_to_string_mapping,
    test_sensor_status_tracks_each_probe,
    test_evaluate_is_rate_limited,
  };
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("DegradationManager", passed, failed);
  return failed;
}
