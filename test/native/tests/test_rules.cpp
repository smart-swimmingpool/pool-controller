/**
 * @file test_rules.cpp
 * @brief Unit tests for Rule classes — effective runtime, timer logic.
 */

#include <stdio.h>
#include <cmath>

// Mock includes (these are picked up via -I mocks/)
#include "Arduino.h"
#include "Rule.hpp"
#include "RuleAuto.hpp"
#include "RuleManu.hpp"
#include "RuleBoost.hpp"
#include "RuleTimer.hpp"

// Test framework
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

int run_rule_tests() {
  int passed = 0, failed = 0;
  int rc;

  // ── Test: RuleTimer effective runtime ──
  {
    test_begin("RuleTimer", "base runtime without extension");
    RuleTimer timer;
    TimerSetting ts;
    ts.timerStartHour = 8;
    ts.timerStartMinutes = 0;
    ts.timerEndHour = 18;
    ts.timerEndMinutes = 0;
    timer.setTimerSetting(ts);

    // Base runtime: 18:00 - 08:00 = 10h = 600min
    uint16_t runtime = timer.getEffectiveRuntimeMinutes();
    rc = (runtime == 600) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 600, got %u", runtime);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("RuleTimer::base_runtime", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: RuleTimer midnight crossing ──
  {
    test_begin("RuleTimer", "midnight crossing runtime");
    RuleTimer timer;
    TimerSetting ts;
    ts.timerStartHour = 22;
    ts.timerStartMinutes = 0;
    ts.timerEndHour = 6;
    ts.timerEndMinutes = 0;
    timer.setTimerSetting(ts);

    // Midnight crossing: (1440 - 1320) + 360 = 120 + 360 = 480min = 8h
    uint16_t runtime = timer.getEffectiveRuntimeMinutes();
    rc = (runtime == 480) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 480, got %u", runtime);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("RuleTimer::midnight", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: RuleAuto effective runtime ──
  {
    test_begin("RuleAuto", "effective runtime equals timer base");
    RuleAuto autoRule;
    // Auto delegates to Timer internally
    // Default: 08:00-18:00 = 600min
    uint16_t runtime = autoRule.getEffectiveRuntimeMinutes();
    rc = (runtime == 600) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 600, got %u", runtime);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("RuleAuto::runtime", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: RuleBoost runtime ──
  {
    test_begin("RuleBoost", "24h boost runtime");
    RuleBoost boost;
    // Boost returns 1440 (24 hours)
    uint16_t runtime = boost.getEffectiveRuntimeMinutes();
    rc = (runtime == 1440) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 1440, got %u", runtime);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("RuleBoost::runtime", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: RuleManu runtime ──
  {
    test_begin("RuleManu", "manual mode returns 0 runtime");
    RuleManu manu;
    uint16_t runtime = manu.getEffectiveRuntimeMinutes();
    rc = (runtime == 0) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 0, got %u", runtime);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("RuleManu::runtime", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: RuleTimer setCustomEndMinutes ──
  {
    test_begin("RuleTimer", "extended runtime with custom end");
    RuleTimer timer;
    TimerSetting ts;
    ts.timerStartHour = 8;
    ts.timerStartMinutes = 0;
    ts.timerEndHour = 18;
    ts.timerEndMinutes = 0;
    timer.setTimerSetting(ts);
    timer.setCustomEndMinutes(20 * 60);  // 20:00

    // Extended: 20:00 - 08:00 = 12h = 720min
    uint16_t runtime = timer.getEffectiveRuntimeMinutes();
    rc = (runtime == 720) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 720, got %u", runtime);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("RuleTimer::extended", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  return passed + failed;
}
