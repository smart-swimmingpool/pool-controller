// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_degradation.cpp
 * @brief Tests for the degradation policy (DegradationPolicy.hpp).
 *
 * Regression for #194: combinations of WiFi/time/sensor problems must not
 * put the controller into safe mode (which switches the filter pump off),
 * and a persistent low-memory state must be detected for a reboot.
 */

#include <cstdint>
#include <cstdio>

#include "DegradationPolicy.hpp"

void test_begin(const char *suite, const char *name);
void test_pass(const char *file, int line);
void test_fail(const char *file, int line, const char *msg);
void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_TRUE(cond)                                     \
  do {                                                        \
    if (!(cond)) {                                            \
      test_fail(__FILE__, __LINE__, "Expected true: " #cond); \
      return 1;                                               \
    }                                                         \
    test_pass(__FILE__, __LINE__);                            \
  } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

using PoolController::classifyDegradation;
using PoolController::DegradationLevel;
using PoolController::PersistentConditionTimer;

static int test_all_ok_is_normal() {
  test_begin("Degradation", "all ok → NORMAL");
  ASSERT_TRUE(classifyDegradation(true, true, true, true) == DegradationLevel::NORMAL);
  return 0;
}

static int test_single_failures() {
  test_begin("Degradation", "single failures map to their level");
  ASSERT_TRUE(classifyDegradation(false, true, true, true) == DegradationLevel::NO_WIFI);
  ASSERT_TRUE(classifyDegradation(true, false, true, true) == DegradationLevel::NO_TIME);
  ASSERT_TRUE(classifyDegradation(true, true, false, true) == DegradationLevel::NO_SENSOR);
  return 0;
}

static int test_multiple_failures_are_not_critical() {
  test_begin("Degradation", "multiple non-memory failures never enter safe mode");
  // Every combination of WiFi/time/sensor problems with healthy memory
  for (int mask = 0; mask < 8; mask++) {
    bool wifiOk = (mask & 1) == 0;
    bool timeOk = (mask & 2) == 0;
    bool sensorOk = (mask & 4) == 0;
    ASSERT_TRUE(classifyDegradation(wifiOk, timeOk, sensorOk, true) != DegradationLevel::CRITICAL);
  }
  // WiFi down + sensor fault (the scenario from #194) reports the sensor fault
  ASSERT_TRUE(classifyDegradation(false, true, false, true) == DegradationLevel::NO_SENSOR);
  return 0;
}

static int test_low_memory_is_critical() {
  test_begin("Degradation", "low memory → CRITICAL");
  ASSERT_TRUE(classifyDegradation(true, true, true, false) == DegradationLevel::CRITICAL);
  ASSERT_TRUE(classifyDegradation(false, false, false, false) == DegradationLevel::CRITICAL);
  return 0;
}

static int test_persistent_timer_fires_after_limit() {
  test_begin("Degradation", "persistent low memory fires after limit");
  PersistentConditionTimer timer;
  const uint32_t limit = 300000;
  ASSERT_FALSE(timer.update(1000, true, limit));
  ASSERT_FALSE(timer.update(1000 + limit - 1, true, limit));
  ASSERT_TRUE(timer.update(1000 + limit, true, limit));
  return 0;
}

static int test_persistent_timer_resets_on_recovery() {
  test_begin("Degradation", "recovery resets the low-memory timer");
  PersistentConditionTimer timer;
  const uint32_t limit = 300000;
  ASSERT_FALSE(timer.update(0, true, limit));
  ASSERT_FALSE(timer.update(200000, false, limit));  // memory recovered
  ASSERT_FALSE(timer.update(250000, true, limit));   // low again — restarts
  ASSERT_FALSE(timer.update(500000, true, limit));   // only 250 s since restart
  ASSERT_TRUE(timer.update(550000, true, limit));
  return 0;
}

static int test_persistent_timer_millis_wrap() {
  test_begin("Degradation", "low-memory timer survives millis() wrap");
  PersistentConditionTimer timer;
  const uint32_t limit = 300000;
  const uint32_t start = 0xFFFFFFFFu - 1000u;
  ASSERT_FALSE(timer.update(start, true, limit));
  ASSERT_FALSE(timer.update(start + 100000u, true, limit));  // wrapped
  ASSERT_TRUE(timer.update(start + limit, true, limit));
  return 0;
}

int run_degradation_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_all_ok_is_normal, test_single_failures, test_multiple_failures_are_not_critical,
    test_low_memory_is_critical, test_persistent_timer_fires_after_limit, test_persistent_timer_resets_on_recovery,
    test_persistent_timer_millis_wrap};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("Degradation", passed, failed);
  return failed;
}
