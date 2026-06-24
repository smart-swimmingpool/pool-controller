/**
 * @file test_timer.cpp
 * @brief Unit tests for Timer.cpp — calculateEffectiveEndMinutes, time helpers.
 */

#include <stdio.h>
#include <cmath>

#include "Arduino.h"
#include "Timer.hpp"
#include "ConfigManager.hpp"

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

#define ASSERT_NEAR(a, b, eps)                                                                             \
  do {                                                                                                     \
    float _a = (a);                                                                                        \
    float _b = (b);                                                                                        \
    if (fabs(_a - _b) > (eps)) {                                                                           \
      char _msg[256];                                                                                      \
      snprintf(_msg, sizeof(_msg), "Expected |%s - %s| < %f: got %f vs %f", #a, #b, (float)(eps), _a, _b); \
      test_fail(__FILE__, __LINE__, _msg);                                                                 \
      return 1;                                                                                            \
    }                                                                                                      \
    test_pass(__FILE__, __LINE__);                                                                         \
  } while (0)

int run_timer_tests() {
  int passed = 0, failed = 0;
  int rc;

  // ── Test: calculateEffectiveEndMinutes returns base when temp NaN ──
  {
    test_begin("Timer", "NaN temp returns base end minutes");

    uint16_t result = calculateEffectiveEndMinutes(480, 1080, NAN);

    ASSERT_EQ(result, 1080);

    test_suite_end("Timer::nan_temp", 1, 0);
    passed++;
  }

  // ── Test: calculateEffectiveEndMinutes returns base when temp below threshold ──
  {
    test_begin("Timer", "temp below threshold returns base end");

    // Default threshold is 24.0°C in mock ConfigManager
    uint16_t result = calculateEffectiveEndMinutes(480, 1080, 20.0f);

    ASSERT_EQ(result, 1080);

    test_suite_end("Timer::below_threshold", 1, 0);
    passed++;
  }

  // ── Test: calculateEffectiveEndMinutes with temp extension ──
  {
    test_begin("Timer", "temp above threshold adds extension");

    // Setup: start=08:00 (480), end=18:00 (1080), baseRuntime=600
    // temp=28.0°C, threshold=24.0°C → diff=4.0°C, factor=30 → extra=120
    // totalRuntime = 600 + 120 = 720
    // extended = 480 + 720 = 1200 (20:00)
    uint16_t result = calculateEffectiveEndMinutes(480, 1080, 28.0f);

    ASSERT_EQ(result, 1200);

    test_suite_end("Timer::temp_extension", 1, 0);
    passed++;
  }

  // ── Test: calculateEffectiveEndMinutes cap at maxRuntime ──
  {
    test_begin("Timer", "temp extension capped at max runtime");

    // Setup: start=08:00 (480), end=18:00 (1080), baseRuntime=600
    // temp=40.0°C, diff=16.0°C, factor=30 → extra=480
    // totalRuntime = 600 + 480 = 1080
    // maxRuntime = 720 → capped at 720
    // extended = 480 + 720 = 1200 (20:00)
    uint16_t result = calculateEffectiveEndMinutes(480, 1080, 40.0f);

    // With default maxRuntime=720: total=600+480=1080 → cap to 720
    // extended = 480 + 720 = 1200
    ASSERT_EQ(result, 1200);

    test_suite_end("Timer::max_runtime_cap", 1, 0);
    passed++;
  }

  // ── Test: calculateEffectiveEndMinutes zero base runtime ──
  {
    test_begin("Timer", "zero base runtime returns base end");

    // start == end
    uint16_t result = calculateEffectiveEndMinutes(600, 600, 30.0f);

    ASSERT_EQ(result, 600);

    test_suite_end("Timer::zero_runtime", 1, 0);
    passed++;
  }

  // ── Test: calculateEffectiveEndMinutes midnight crossing ──
  {
    test_begin("Timer", "midnight crossing base runtime");

    // start=22:00 (1320), end=06:00 (360)
    // baseRuntime = (1440-1320) + 360 = 480
    // temp=30°C, diff=6°C, factor=30 → extra=180
    // totalRuntime = 480 + 180 = 660
    // extended = 1320 + 660 = 1980 → wraps past midnight
    uint16_t result = calculateEffectiveEndMinutes(1320, 360, 30.0f);

    // extended = 1320 + min(660, 720) = 1320 + 660 = 1980
    ASSERT_EQ(result, 1980);

    test_suite_end("Timer::midnight_crossing", 1, 0);
    passed++;
  }

  // ── Test: getStartTime and getEndTime ──
  {
    test_begin("Timer", "getStartTime / getEndTime helpers");

    tm base = {};
    base.tm_year = 2024 - 1900;
    base.tm_mon = 5;
    base.tm_mday = 15;
    base.tm_hour = 12;
    base.tm_min = 0;
    base.tm_sec = 0;

    TimerSetting ts;
    ts.timerStartHour = 8;
    ts.timerStartMinutes = 30;
    ts.timerEndHour = 18;
    ts.timerEndMinutes = 45;

    tm start = getStartTime(base, ts);
    tm end = getEndTime(base, ts);

    ASSERT_EQ(start.tm_hour, 8);
    ASSERT_EQ(start.tm_min, 30);
    ASSERT_EQ(start.tm_sec, 0);
    ASSERT_EQ(end.tm_hour, 18);
    ASSERT_EQ(end.tm_min, 45);
    ASSERT_EQ(end.tm_sec, 0);

    test_suite_end("Timer::start_end_helpers", 1, 0);
    passed++;
  }

  printf("\n  Timer Tests: %d passed, %d failed\n", passed, failed);
  return passed + failed;
}
