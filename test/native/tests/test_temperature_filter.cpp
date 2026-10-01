// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_temperature_filter.cpp
 * @brief Tests for TemperatureReadingFilter — regression for #198: the
 *        DS18B20 85 °C power-on value and out-of-range readings must not be
 *        used as real temperatures.
 */

#include <cmath>
#include <cstdio>

#include "TemperatureReadingFilter.hpp"

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

using PoolController::TemperatureReadingFilter;
using Action = PoolController::TemperatureReadingFilter::Action;

static int test_normal_values_accepted() {
  test_begin("TemperatureFilter", "normal values are accepted");
  TemperatureReadingFilter filter;
  ASSERT_TRUE(filter.apply(24.5f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(-10.0f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(70.0f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(-55.0f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(125.0f) == Action::ACCEPT);
  return 0;
}

static int test_invalid_values_rejected() {
  test_begin("TemperatureFilter", "disconnected, NaN and out-of-range values are rejected");
  TemperatureReadingFilter filter;
  ASSERT_TRUE(filter.apply(-127.0f) == Action::REJECT);  // DEVICE_DISCONNECTED_C
  ASSERT_TRUE(filter.apply(NAN) == Action::REJECT);
  ASSERT_TRUE(filter.apply(-55.5f) == Action::REJECT);
  ASSERT_TRUE(filter.apply(125.5f) == Action::REJECT);
  ASSERT_TRUE(filter.apply(4000.0f) == Action::REJECT);
  return 0;
}

static int test_power_on_value_held() {
  test_begin("TemperatureFilter", "85 °C after a normal reading is held");
  TemperatureReadingFilter filter;
  ASSERT_TRUE(filter.apply(24.0f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(85.0f) == Action::HOLD);
  ASSERT_TRUE(filter.apply(24.1f) == Action::ACCEPT);  // glitch over
  ASSERT_TRUE(filter.apply(85.0f) == Action::HOLD);    // next glitch held again
  return 0;
}

static int test_power_on_value_without_history_rejected() {
  test_begin("TemperatureFilter", "85 °C as first reading is rejected");
  TemperatureReadingFilter filter;
  ASSERT_TRUE(filter.apply(85.0f) == Action::REJECT);
  return 0;
}

static int test_power_on_value_confirmed_by_repetition() {
  test_begin("TemperatureFilter", "repeated 85 °C is accepted as real value");
  TemperatureReadingFilter filter;
  ASSERT_TRUE(filter.apply(30.0f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(85.0f) == Action::HOLD);
  ASSERT_TRUE(filter.apply(85.0f) == Action::HOLD);
  ASSERT_TRUE(filter.apply(85.0f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(85.0f) == Action::ACCEPT);  // continues the accepted value
  return 0;
}

static int test_power_on_value_continuing_hot_collector() {
  test_begin("TemperatureFilter", "85 °C after a hot collector reading is accepted");
  TemperatureReadingFilter filter;
  ASSERT_TRUE(filter.apply(83.5f) == Action::ACCEPT);
  ASSERT_TRUE(filter.apply(85.0f) == Action::ACCEPT);
  return 0;
}

static int test_reset_forgets_history() {
  test_begin("TemperatureFilter", "reset forgets the last accepted value");
  TemperatureReadingFilter filter;
  ASSERT_TRUE(filter.apply(84.0f) == Action::ACCEPT);
  filter.reset();
  ASSERT_TRUE(filter.apply(85.0f) == Action::REJECT);
  return 0;
}

int run_temperature_filter_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_normal_values_accepted, test_invalid_values_rejected, test_power_on_value_held,
    test_power_on_value_without_history_rejected, test_power_on_value_confirmed_by_repetition,
    test_power_on_value_continuing_hot_collector, test_reset_forgets_history};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("TemperatureFilter", passed, failed);
  return failed;
}
