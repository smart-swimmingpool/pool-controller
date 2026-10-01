// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_setting_validation.cpp
 * @brief Tests for the shared numeric setting parser (SettingValidation.hpp)
 *        used by MqttPublisher and OperationModeNode (#197).
 */

#include <cstdio>

#include "SettingValidation.hpp"

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

using PoolController::FloatRange;
using PoolController::parseDecimalFloat;
using PoolController::parseFloatInRange;
namespace Limits = PoolController::SettingLimits;

static int test_plain_decimals_accepted() {
  test_begin("SettingValidation", "plain decimal numbers are accepted");
  float v = 0.0f;
  ASSERT_TRUE(parseDecimalFloat("27", v) && v == 27.0f);
  ASSERT_TRUE(parseDecimalFloat("28.5", v) && v == 28.5f);
  ASSERT_TRUE(parseDecimalFloat("-3.25", v) && v == -3.25f);
  ASSERT_TRUE(parseDecimalFloat("+1", v) && v == 1.0f);
  ASSERT_TRUE(parseDecimalFloat(".5", v) && v == 0.5f);
  ASSERT_TRUE(parseDecimalFloat("5.", v) && v == 5.0f);
  return 0;
}

static int test_malformed_rejected() {
  test_begin("SettingValidation", "malformed payloads are rejected");
  float v = 42.0f;
  ASSERT_FALSE(parseDecimalFloat(nullptr, v));
  ASSERT_FALSE(parseDecimalFloat("", v));
  ASSERT_FALSE(parseDecimalFloat("abc", v));
  ASSERT_FALSE(parseDecimalFloat("-", v));
  ASSERT_FALSE(parseDecimalFloat(".", v));
  ASSERT_FALSE(parseDecimalFloat("25abc", v));
  ASSERT_FALSE(parseDecimalFloat("25 ", v));
  ASSERT_FALSE(parseDecimalFloat(" 25", v));
  ASSERT_FALSE(parseDecimalFloat("1e1", v));
  ASSERT_FALSE(parseDecimalFloat("1.2.3", v));
  ASSERT_FALSE(parseDecimalFloat("--1", v));
  ASSERT_FALSE(parseDecimalFloat("nan", v));
  ASSERT_FALSE(parseDecimalFloat("inf", v));
  ASSERT_TRUE(v == 42.0f);  // output untouched on failure
  return 0;
}

static int test_range_check() {
  test_begin("SettingValidation", "range limits are inclusive");
  const FloatRange range{0.0f, 40.0f};
  float v = 0.0f;
  ASSERT_TRUE(parseFloatInRange("0", range, v) && v == 0.0f);
  ASSERT_TRUE(parseFloatInRange("40", range, v) && v == 40.0f);
  ASSERT_FALSE(parseFloatInRange("40.1", range, v));
  ASSERT_FALSE(parseFloatInRange("-0.1", range, v));
  ASSERT_FALSE(parseFloatInRange("abc", range, v));
  return 0;
}

static int test_setting_limits() {
  test_begin("SettingValidation", "setting limits match the published HA ranges");
  float v = 0.0f;
  ASSERT_TRUE(parseFloatInRange("40", Limits::kPoolMaxTemp, v));
  ASSERT_FALSE(parseFloatInRange("41", Limits::kPoolMaxTemp, v));
  ASSERT_TRUE(parseFloatInRange("100", Limits::kSolarMinTemp, v));
  ASSERT_FALSE(parseFloatInRange("101", Limits::kSolarMinTemp, v));
  ASSERT_TRUE(parseFloatInRange("10", Limits::kHysteresis, v));
  ASSERT_FALSE(parseFloatInRange("10.5", Limits::kHysteresis, v));
  ASSERT_TRUE(parseFloatInRange("40", Limits::kTempCircThreshold, v));
  ASSERT_FALSE(parseFloatInRange("40.5", Limits::kTempCircThreshold, v));
  return 0;
}

int run_setting_validation_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_plain_decimals_accepted, test_malformed_rejected, test_range_check, test_setting_limits};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("SettingValidation", passed, failed);
  return failed;
}
