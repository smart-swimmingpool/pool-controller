/**
 * @file stubs.cpp
 * @brief Stub implementations for free functions and production dependencies
 *        not directly under test.
 *
 * Node class methods and ConfigManager/NetworkManager/... are all provided
 * inline by the mock headers in mocks/ — no definitions needed here.
 *
 * This file only provides definitions for:
 *   - Free functions (TimeClientHelper, TimeLib, Timer helpers)
 *   - Any non-inline static members still needed by production headers
 */

#include "Arduino.h"

#include <cstdint>

#include "Rule.hpp"
#include "TimeLib.h"
#include "Timer.hpp"

// ═══════════════════════════════════════════════════════════════════════════════
// Time helpers (normally in TimeClientHelper.cpp / Timer.cpp)
// ═══════════════════════════════════════════════════════════════════════════════

tm getCurrentDateTime() {
  tm t = {};
  t.tm_year = 2024 - 1900;
  t.tm_mon = 5;
  t.tm_mday = 15;
  t.tm_hour = 12;
  t.tm_min = 0;
  t.tm_sec = 0;
  t.tm_wday = 6;
  return t;
}

bool isTimeSyncValid() { return true; }

tm getStartTime(const tm &baseTime, TimerSetting ts) {
  tm t = baseTime;
  t.tm_hour = ts.timerStartHour;
  t.tm_min = ts.timerStartMinutes;
  t.tm_sec = 0;
  return t;
}

tm getEndTime(const tm &baseTime, TimerSetting ts) {
  tm t = baseTime;
  t.tm_hour = ts.timerEndHour;
  t.tm_min = ts.timerEndMinutes;
  t.tm_sec = 0;
  return t;
}

uint16_t calculateEffectiveEndMinutes(uint16_t baseStartMinutes, uint16_t baseEndMinutes, float poolTemp) {
  (void)baseStartMinutes;
  (void)poolTemp;
  return baseEndMinutes;
}

// TimeClientHelper stubs
int getTzCount() { return 1; }
int getTimezoneLabelCount() { return 0; }
const char *const *getTimezoneLabelList() {
  static const char *labels[] = {"UTC", "Europe/Berlin", nullptr};
  return labels;
}
int getTimezoneIndexFromLabel(const String &) { return 0; }
String getFormattedTime(time_t) { return String("12:00"); }
time_t getUtcTime() { return 0; }
int getTimezoneIndex() { return 0; }
time_t getLastValidSyncTime() { return 0; }
bool forceNtpUpdate() { return true; }
void setTimeDegradationGreenHours(uint8_t) {}
uint8_t getTimeDegradationGreenHours() { return 1; }
void setTimeDegradationRedHours(uint8_t) {}
uint8_t getTimeDegradationRedHours() { return 24; }

time_t getTimeFor(int tzIndex, TimeChangeRule **tcr) {
  if (tcr) *tcr = nullptr;
  return 0;
}
String getTimeInfoFor(int) { return String("UTC"); }
TimeDegradation getTimeDegradation() { return TimeDegradation::GREEN; }
void setTimezoneIndex(int) {}
void timeClientSetup(const char *) {}

// TimeLib stubs
time_t now() { return 0; }
void setTime(time_t) {}
void setTime(int, int, int, int, int, int) {}
time_t makeTime(const tmElements_t &) { return 0; }
void breakTime(time_t, tmElements_t &) {}
int dayStr(int) { return 0; }
int monthStr(int) { return 0; }
time_t previousMidnight(time_t t) { (void)t; return 0; }

// ═══════════════════════════════════════════════════════════════════════════════
// ESP32 hardware stubs
// ═══════════════════════════════════════════════════════════════════════════════

uint8_t temprature_sens_read() { return 25; }
