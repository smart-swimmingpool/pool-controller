// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file stubs.cpp
 * @brief Minimal stand-ins for the small set of production dependencies that
 *        RelayModuleNode / RuleAuto / RuleBoost / RuleTimer pull in but that
 *        this isolated test binary does not want to build for real
 *        (ConfigManager NVS I/O, DegradationManager health tracking, NTP).
 *
 * Everything else (RelayModuleNode, Rule, RuleAuto, RuleBoost, RuleTimer,
 * Timer) is compiled from the REAL src/ sources — this binary exists
 * specifically to exercise that production logic against a real
 * digitalWrite-capturing GPIO mock, not against the hand-rolled
 * simplified mocks used by the main test_runner (which would create an ODR
 * clash if linked together with these real classes).
 */

#include "Arduino.h"
#include "ConfigManager.hpp"
#include "DegradationManager.hpp"
#include "TimeClientHelper.hpp"

// Static storage backing the mock Preferences (NVS) key/value store.
std::map<std::string, std::string> Preferences::s_data;

// Serial mock capture state (disabled by default) — LogCapture.cpp's
// Serial mirror references these statics via Arduino.h.
std::string SerialClass::s_capture;
bool SerialClass::s_captureEnabled = false;

// ── ConfigManager: only the static settings_ member is referenced (via
//    getSettings(), which Timer.cpp's calculateEffectiveEndMinutes() calls).
//    Default member initializers in ControllerSettings give sane defaults. ──
namespace PoolController {
ControllerSettings ConfigManager::settings_;
}

// ── DegradationManager: only isSafe() is referenced, by
//    RelayModuleNode::setSwitch() to block ON transitions in safe mode.
//    Tests never simulate safe mode, so always report "not safe". ──
namespace PoolController {
bool DegradationManager::isSafe() {
  return false;
}
}  // namespace PoolController

// ── TimeClientHelper: controllable fake clock so tests can deterministically
//    put the pool-pump timer inside / outside its active window. ──
static time_t g_fakeNow = 1750000000;  // arbitrary fixed instant
static bool g_timeSyncValid = true;

void test_setFakeNow(time_t t) {
  g_fakeNow = t;
}

void test_setTimeSyncValid(bool valid) {
  g_timeSyncValid = valid;
}

bool isTimeSyncValid() {
  return g_timeSyncValid;
}

TimeDegradation getTimeDegradation() {
  return g_timeSyncValid ? TimeDegradation::GREEN : TimeDegradation::RED;
}

int getTimezoneIndex() {
  return 0;
}

time_t getTimeFor(int /*tzIndex*/, TimeChangeRule **tcr) {
  if (tcr)
    *tcr = nullptr;
  return g_fakeNow;
}

// Unused by the code paths under test, but declared in TimeClientHelper.hpp
// and required to satisfy the linker if anything odr-uses them transitively.
void timeClientSetup(const char *) {}
void syncSystemClock() {}
int getTzCount() {
  return 1;
}
String getTimeInfoFor(int) {
  return String("UTC");
}
const char *const *getTimezoneLabelList() {
  static const char *labels[] = {"UTC", nullptr};
  return labels;
}
int getTimezoneLabelCount() {
  return 0;
}
int getTimezoneIndexFromLabel(const String &) {
  return 0;
}
String getFormattedTime(time_t) {
  return String("00:00");
}
time_t getUtcTime() {
  return 0;
}
void setTimezoneIndex(int) {}
time_t getLastValidSyncTime() {
  return 0;
}
bool forceNtpUpdate() {
  return true;
}
void setTimeDegradationGreenHours(uint8_t) {}
uint8_t getTimeDegradationGreenHours() {
  return 1;
}
void setTimeDegradationRedHours(uint8_t) {}
uint8_t getTimeDegradationRedHours() {
  return 24;
}
