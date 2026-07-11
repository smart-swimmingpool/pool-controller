// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_relay_safety.cpp
 * @brief Regression tests guarding against the NORVI AE01-R active-HIGH
 *        relay polarity bug (solar pump stuck ON while the circulation
 *        pump is OFF) across every operation mode that can drive the
 *        solar relay: Auto, Boost, and Timer.
 *
 * Unlike test_rules.cpp (which uses simplified mocks to test only the
 * shared Rule.hpp timer arithmetic), this binary links the REAL
 * RelayModuleNode, RuleAuto, RuleBoost and RuleTimer production classes
 * against a digitalWrite-capturing GPIO mock, so it actually exercises the
 * physical pin level each mode would drive — not just the logical
 * getSwitch()/setSwitch() state.
 */

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "Arduino.h"
#include "RelayModuleNode.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
#include "RuleTimer.hpp"

// From stubs.cpp — lets tests deterministically control "now" and time-sync
// health without touching the real NTP/Timezone machinery.
void test_setFakeNow(time_t t);
void test_setTimeSyncValid(bool valid);

// From Arduino.h mock — digitalWrite capture.
extern int digitalRead(uint8_t pin);
extern void _resetDigitalPinState();

namespace {

int g_passed = 0;
int g_failed = 0;

void check(bool cond, const char *file, int line, const char *expr) {
  if (cond) {
    g_passed++;
  } else {
    g_failed++;
    fprintf(stderr, "  ✗ FAIL %s:%d — %s\n", file, line, expr);
  }
}

#define CHECK(cond) check((cond), __FILE__, __LINE__, #cond)

void section(const char *name) {
  printf("\n== %s ==\n", name);
}

// Compute a timer window guaranteed to be OUTSIDE (or INSIDE) "now",
// expressed purely relative to whatever localtime() reports for
// g_fakeNow under the forced UTC timezone — no hardcoded wall-clock
// assumptions.
struct Window {
  unsigned int startHour, startMin, endHour, endMin;
};

Window outsideWindowRelativeToNow(time_t now) {
  tm t = *localtime(&now);
  unsigned int h = (t.tm_hour + 2) % 24;
  unsigned int h2 = (t.tm_hour + 3) % 24;
  return Window{h, 0, h2, 0};
}

Window insideWindowRelativeToNow(time_t now) {
  tm t = *localtime(&now);
  unsigned int h1 = (t.tm_hour + 23) % 24;  // 1h before "now"
  unsigned int h2 = (t.tm_hour + 1) % 24;   // 1h after "now"
  return Window{h1, 0, h2, 0};
}

TimerSetting toTimerSetting(const Window &w) {
  TimerSetting ts;
  ts.timerStartHour = w.startHour;
  ts.timerStartMinutes = w.startMin;
  ts.timerEndHour = w.endHour;
  ts.timerEndMinutes = w.endMin;
  return ts;
}

// ── RelayModuleNode polarity regression ────────────────────────────────────
// This is the exact bug that shipped: RelayModuleNode hardcoded active-LOW
// digitalWrite logic while the NORVI AE01-R's built-in relays are
// active-HIGH, so "OFF" (state=false) energized the relay instead of
// de-energizing it.
void test_relay_polarity() {
  section("RelayModuleNode polarity");

  const uint8_t PIN_NORVI = 12;
  const uint8_t PIN_STANDARD = 13;

  _resetDigitalPinState();

  // NORVI AE01-R built-in relay: active-HIGH (activeLow = false).
  {
    RelayModuleNode norviRelay("test-norvi", "Test NORVI Relay", PIN_NORVI, false);
    norviRelay.begin();
    CHECK(norviRelay.getSwitch() == false);
    CHECK(digitalRead(PIN_NORVI) == LOW);  // OFF must be LOW on active-HIGH hw

    norviRelay.setSwitch(true);
    CHECK(norviRelay.getSwitch() == true);
    CHECK(digitalRead(PIN_NORVI) == HIGH);  // ON must be HIGH on active-HIGH hw

    norviRelay.setSwitch(false);
    CHECK(norviRelay.getSwitch() == false);
    CHECK(digitalRead(PIN_NORVI) == LOW);  // OFF must be LOW again
  }

  // Standard external relay module: active-LOW (activeLow = true, default).
  {
    RelayModuleNode standardRelay("test-standard", "Test Standard Relay", PIN_STANDARD, true);
    standardRelay.begin();
    CHECK(standardRelay.getSwitch() == false);
    CHECK(digitalRead(PIN_STANDARD) == HIGH);  // OFF must be HIGH on active-LOW hw

    standardRelay.setSwitch(true);
    CHECK(digitalRead(PIN_STANDARD) == LOW);  // ON must be LOW on active-LOW hw

    standardRelay.setSwitch(false);
    CHECK(digitalRead(PIN_STANDARD) == HIGH);
  }
}

// ── RuleAuto: solar must go off when the circulation pump is off ──────────
void test_rule_auto_solar_off_when_pool_off() {
  section("RuleAuto — solar off when pool pump off");

  const uint8_t PIN_POOL = 14;
  const uint8_t PIN_SOLAR = 12;
  _resetDigitalPinState();

  RelayModuleNode poolPump("pool-pump", "Pool Pump", PIN_POOL, false);
  RelayModuleNode solarPump("solar-pump", "Solar Pump", PIN_SOLAR, false);
  poolPump.begin();
  solarPump.begin();

  RuleAuto rule(&solarPump, &poolPump);

  time_t now = 1750000000;
  test_setFakeNow(now);
  test_setTimeSyncValid(true);

  // Timer window clearly OUTSIDE "now" → checkPoolPumpTimer() must return
  // false → pool pump goes OFF this cycle.
  rule.setTimerSetting(toTimerSetting(outsideWindowRelativeToNow(now)));
  rule.setPoolMaxTemperature(28.5f);
  rule.setSolarMinTemperature(20.0f);
  rule.setTemperatureHysteresis(1.0f);
  rule.setPoolTemperature(25.0f);
  rule.setSolarTemperature(30.0f);

  // Simulate solar having been left ON from a previous cycle.
  solarPump.setSwitch(true);
  CHECK(solarPump.getSwitch() == true);

  rule.loop();

  CHECK(poolPump.getSwitch() == false);
  CHECK(solarPump.getSwitch() == false);
  // Physical pin must reflect OFF using this relay's active-HIGH polarity.
  CHECK(digitalRead(PIN_SOLAR) == LOW);
  CHECK(digitalRead(PIN_POOL) == LOW);
}

// ── RuleBoost: solar must go off when the circulation pump is off ─────────
void test_rule_boost_solar_off_when_pool_off() {
  section("RuleBoost — solar off when pool pump off");

  const uint8_t PIN_POOL = 15;
  const uint8_t PIN_SOLAR = 16;
  _resetDigitalPinState();

  RelayModuleNode poolPump("pool-pump-b", "Pool Pump", PIN_POOL, false);
  RelayModuleNode solarPump("solar-pump-b", "Solar Pump", PIN_SOLAR, false);
  poolPump.begin();
  solarPump.begin();

  RuleBoost rule(&solarPump, &poolPump);
  rule.setPoolMaxTemperature(28.5f);
  rule.setTemperatureHysteresis(1.0f);
  rule.setPoolTemperature(25.0f);
  rule.setSolarTemperature(30.0f);

  // RuleBoost does not drive the pool relay itself — it only reacts to its
  // current state, so the caller (WebPortal / OperationModeNode) is
  // responsible for that. Simulate the pool pump already being off.
  poolPump.setSwitch(false);
  solarPump.setSwitch(true);  // stuck-on from a previous cycle
  CHECK(solarPump.getSwitch() == true);

  rule.loop();

  CHECK(solarPump.getSwitch() == false);
  CHECK(digitalRead(PIN_SOLAR) == LOW);
}

// ── RuleTimer: solar must go off when the circulation pump is off ─────────
void test_rule_timer_solar_off_when_pool_off() {
  section("RuleTimer — solar off when pool pump off");

  const uint8_t PIN_POOL = 17;
  const uint8_t PIN_SOLAR = 18;
  _resetDigitalPinState();

  RelayModuleNode poolPump("pool-pump-t", "Pool Pump", PIN_POOL, false);
  RelayModuleNode solarPump("solar-pump-t", "Solar Pump", PIN_SOLAR, false);
  poolPump.begin();
  solarPump.begin();

  RuleTimer rule(&solarPump, &poolPump);

  time_t now = 1750000000;
  test_setFakeNow(now);
  test_setTimeSyncValid(true);
  rule.setTimerSetting(toTimerSetting(outsideWindowRelativeToNow(now)));
  rule.setPoolTemperature(25.0f);

  solarPump.setSwitch(true);  // RuleTimer never turns solar on itself, but
                               // guard against it ever failing to turn it off.
  CHECK(solarPump.getSwitch() == true);

  rule.loop();

  CHECK(poolPump.getSwitch() == false);
  CHECK(solarPump.getSwitch() == false);
  CHECK(digitalRead(PIN_SOLAR) == LOW);
  CHECK(digitalRead(PIN_POOL) == LOW);
}

// ── RuleAuto: sanity check the pool-ON path still drives the pin HIGH ─────
void test_rule_auto_pool_on_drives_pin_high() {
  section("RuleAuto — pool pump ON drives pin HIGH (sanity)");

  const uint8_t PIN_POOL = 19;
  const uint8_t PIN_SOLAR = 20;
  _resetDigitalPinState();

  RelayModuleNode poolPump("pool-pump-2", "Pool Pump", PIN_POOL, false);
  RelayModuleNode solarPump("solar-pump-2", "Solar Pump", PIN_SOLAR, false);
  poolPump.begin();
  solarPump.begin();

  RuleAuto rule(&solarPump, &poolPump);

  time_t now = 1750000000;
  test_setFakeNow(now);
  test_setTimeSyncValid(true);

  // Timer window clearly INSIDE "now" → pool pump goes ON this cycle.
  rule.setTimerSetting(toTimerSetting(insideWindowRelativeToNow(now)));
  rule.setPoolMaxTemperature(28.5f);
  rule.setSolarMinTemperature(20.0f);
  rule.setTemperatureHysteresis(1.0f);
  rule.setPoolTemperature(25.0f);
  rule.setSolarTemperature(30.0f);  // solar hotter than pool → should turn on

  rule.loop();

  CHECK(poolPump.getSwitch() == true);
  CHECK(digitalRead(PIN_POOL) == HIGH);
  CHECK(solarPump.getSwitch() == true);
  CHECK(digitalRead(PIN_SOLAR) == HIGH);
}

}  // namespace

int main() {
  setenv("TZ", "UTC", 1);
  tzset();

  test_relay_polarity();
  test_rule_auto_solar_off_when_pool_off();
  test_rule_boost_solar_off_when_pool_off();
  test_rule_timer_solar_off_when_pool_off();
  test_rule_auto_pool_on_drives_pin_high();

  printf("\n══════════════════════════════════════════════════\n");
  printf("  Relay Safety Results: %d passed, %d failed\n", g_passed, g_failed);
  printf("══════════════════════════════════════════════════\n");

  return g_failed == 0 ? 0 : 1;
}
