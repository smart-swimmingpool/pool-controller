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
#include "TestTime.hpp"

// Test framework
extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

/**
 * @brief Test harness exposing the protected pump-timer logic.
 */
class PumpTimerHarness : public Rule {
public:
  using Rule::checkPoolPumpTimer;  // expose both overloads
  using Rule::setTimerSetting;
  using Rule::getActiveEndMinutes;
  const char *getMode() override { return "test"; }
  void loop() override {}
};

/**
 * @brief Set the mock wall-clock time used by getCurrentDateTime().
 *
 * The epoch is built with mktime() from a tm struct, so the localtime_r()
 * round-trip inside getCurrentDateTime() yields the same wall-clock time
 * regardless of the host timezone.
 */
static void setWallClock(int hour, int min) {
  tm t = {};
  t.tm_year = 2026 - 1900;
  t.tm_mon = 5;  // June — avoids DST ambiguity
  t.tm_mday = 15;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = 0;
  t.tm_isdst = -1;
  setMockTime(mktime(&t));
}

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

  // ── Test: temp extension wrapping past midnight with same-day base timer ──
  // Base 16:00-20:00 (does NOT cross midnight), pool 40.0°C:
  //   threshold 24.0, factor 30 → extra = 16*30 = 480
  //   baseRuntime = 240, totalRuntime = min(240+480, 720) = 720
  //   extended = 960 + 720 = 1680 → 04:00 next day (wraps past midnight)
  // Regression: Step 2 previously keyed the midnight check on the BASE timer
  // (crossesMidnight=false), so during the base window the extension was reset
  // and the pump turned OFF at 20:00 instead of running until 04:00.
  {
    test_begin("PumpTimer", "wrap-past-midnight extension keeps pump ON");

    PumpTimerHarness timer;
    TimerSetting ts;
    ts.timerStartHour = 16;
    ts.timerStartMinutes = 0;
    ts.timerEndHour = 20;
    ts.timerEndMinutes = 0;
    timer.setTimerSetting(ts);

    setWallClock(18, 0);  // inside base window → extension applied
    bool on = timer.checkPoolPumpTimer(40.0f);
    bool extSet = (timer.getActiveEndMinutes() == 1680);
    rc = (on && extSet) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[128];
      snprintf(msg, sizeof(msg), "on=%d activeEnd=%u (expected 1 and 1680)", on, timer.getActiveEndMinutes());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }

    setWallClock(20, 30);  // past base end, still inside extended window
    on = timer.checkPoolPumpTimer(40.0f);
    rc = (on) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Pump OFF at 20:30 — extension was reset (bug)");
      failed++;
    }

    setWallClock(2, 0);  // 02:00 next day, still inside extended window
    on = timer.checkPoolPumpTimer(40.0f);
    rc = (on) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Pump OFF at 02:00 — expected extension active until 04:00");
      failed++;
    }

    setWallClock(4, 30);  // past extended end → extension expired
    on = timer.checkPoolPumpTimer(40.0f);
    rc = (!on && timer.getActiveEndMinutes() == 0) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[128];
      snprintf(msg, sizeof(msg), "on=%d activeEnd=%u (expected 0 and 0)", on, timer.getActiveEndMinutes());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }

    test_suite_end("PumpTimer::wrap_past_midnight", rc == 0 ? 4 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: same-day extension still works (no regression) ──
  // Base 08:00-18:00, pool 28.0°C → extra = 4*30 = 120, total = 720,
  // extended = 480 + 720 = 1200 (20:00, no wrap).
  {
    test_begin("PumpTimer", "same-day extension keeps pump ON");

    PumpTimerHarness timer;
    TimerSetting ts;
    ts.timerStartHour = 8;
    ts.timerStartMinutes = 0;
    ts.timerEndHour = 18;
    ts.timerEndMinutes = 0;
    timer.setTimerSetting(ts);

    setWallClock(10, 0);  // inside base window → extension applied
    bool on = timer.checkPoolPumpTimer(28.0f);
    bool extSet = (timer.getActiveEndMinutes() == 1200);
    rc = (on && extSet) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[128];
      snprintf(msg, sizeof(msg), "on=%d activeEnd=%u (expected 1 and 1200)", on, timer.getActiveEndMinutes());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }

    setWallClock(19, 0);  // past base end, inside extended window (until 20:00)
    on = timer.checkPoolPumpTimer(28.0f);
    rc = (on) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Pump OFF at 19:00 — expected extension active until 20:00");
      failed++;
    }

    setWallClock(20, 30);  // past extended end → extension expired
    on = timer.checkPoolPumpTimer(28.0f);
    rc = (!on && timer.getActiveEndMinutes() == 0) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[128];
      snprintf(msg, sizeof(msg), "on=%d activeEnd=%u (expected 0 and 0)", on, timer.getActiveEndMinutes());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }

    test_suite_end("PumpTimer::same_day", rc == 0 ? 3 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: base timer crossing midnight without extension (no regression) ──
  // Base 22:00-06:00, pool 20.0°C (below threshold) → no extension.
  // Window must still be active at e.g. 02:00 and inactive at 12:00.
  {
    test_begin("PumpTimer", "midnight-crossing base timer without extension");

    PumpTimerHarness timer;
    TimerSetting ts;
    ts.timerStartHour = 22;
    ts.timerStartMinutes = 0;
    ts.timerEndHour = 6;
    ts.timerEndMinutes = 0;
    timer.setTimerSetting(ts);

    setWallClock(2, 0);  // inside base window (after midnight)
    bool on = timer.checkPoolPumpTimer(20.0f);
    rc = (on) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Pump OFF at 02:00 — expected base window active");
      failed++;
    }

    setWallClock(12, 0);  // outside base window
    on = timer.checkPoolPumpTimer(20.0f);
    rc = (!on) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Pump ON at 12:00 — expected outside base window");
      failed++;
    }

    test_suite_end("PumpTimer::base_crosses_midnight", rc == 0 ? 2 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: base timer crossing midnight WITH extension (no regression) ──
  // Base 22:00-06:00, pool 30.0°C → extra = 6*30 = 180, total = 480+180 = 660,
  // extended = 1320 + 660 = 1980 → 09:00 next day (wraps).
  {
    test_begin("PumpTimer", "midnight-crossing base timer with extension");

    PumpTimerHarness timer;
    TimerSetting ts;
    ts.timerStartHour = 22;
    ts.timerStartMinutes = 0;
    ts.timerEndHour = 6;
    ts.timerEndMinutes = 0;
    timer.setTimerSetting(ts);

    setWallClock(23, 0);  // inside base window → extension applied
    bool on = timer.checkPoolPumpTimer(30.0f);
    bool extSet = (timer.getActiveEndMinutes() == 1980);
    rc = (on && extSet) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[128];
      snprintf(msg, sizeof(msg), "on=%d activeEnd=%u (expected 1 and 1980)", on, timer.getActiveEndMinutes());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }

    setWallClock(8, 0);  // after base end, inside extended window (until 09:00)
    on = timer.checkPoolPumpTimer(30.0f);
    rc = (on) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Pump OFF at 08:00 — expected extension active until 09:00");
      failed++;
    }

    setWallClock(10, 0);  // past extended end → extension expired
    on = timer.checkPoolPumpTimer(30.0f);
    rc = (!on) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Pump ON at 10:00 — expected extension expired");
      failed++;
    }

    test_suite_end("PumpTimer::crossing_with_extension", rc == 0 ? 3 : 0, rc != 0 ? 1 : 0);
  }

  return passed + failed;
}
