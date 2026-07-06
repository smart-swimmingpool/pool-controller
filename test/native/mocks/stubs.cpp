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

#include <cstdint>
#include <ctime>

#include "Arduino.h"
#include "TimeLib.h"

// ═══════════════════════════════════════════════════════════════════════════════
// strlcat — provide our own when the host libc does not (e.g. glibc < 2.38).
// macOS and glibc ≥ 2.38 provide it natively; CMake's CheckSymbolExists sets
// HAVE_STRLCAT accordingly.
// ═══════════════════════════════════════════════════════════════════════════════
#ifndef HAVE_STRLCAT
#include <cstring>
extern "C" size_t strlcat(char *dst, const char *src, size_t siz) {
  size_t dlen = strlen(dst);
  size_t slen = strlen(src);
  if (dlen >= siz) return siz + slen;
  size_t n = siz - dlen - 1;
  strncat(dst + dlen, src, n);
  dst[dlen + (n < slen ? n : slen)] = '\0';
  return dlen + slen;
}
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// Local enum definition matching production TimeClientHelper.hpp
// Defined here to avoid pulling in the production header (which transitively
// includes Timezone.h / NTPClient.h via Timer.hpp → Rule.hpp chain when
// compiled from subagent context).
// ═══════════════════════════════════════════════════════════════════════════════

enum class TimeDegradation : uint8_t { GREEN = 0, YELLOW = 1, RED = 2 };

// ═══════════════════════════════════════════════════════════════════════════════
// Time helpers — only provide stubs for functions NOT in Timer.cpp
// (Timer.cpp is compiled as a service source and provides real implementations
//  of getCurrentDateTime, getStartTime, getEndTime, calculateEffectiveEndMinutes)
// ═══════════════════════════════════════════════════════════════════════════════

bool isTimeSyncValid() {
  return true;
}

// TimeClientHelper stubs
int getTzCount() {
  return 1;
}
int getTimezoneLabelCount() {
  return 0;
}
const char *const *getTimezoneLabelList() {
  static const char *labels[] = {"UTC", "Europe/Berlin", nullptr};
  return labels;
}
int getTimezoneIndexFromLabel(const String &) {
  return 0;
}
String getFormattedTime(time_t) {
  return String("12:00");
}
time_t getUtcTime() {
  return 0;
}
int getTimezoneIndex() {
  return 0;
}
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

time_t getTimeFor(int tzIndex, TimeChangeRule **tcr) {
  if (tcr)
    *tcr = nullptr;
  return 0;
}
String getTimeInfoFor(int) {
  return String("UTC");
}
TimeDegradation getTimeDegradation() {
  return TimeDegradation::GREEN;
}
void setTimezoneIndex(int) {}
void timeClientSetup(const char *) {}

// Stub for system clock sync (used by OtaUpdater.cpp in native tests)
void syncSystemClock() {
  // In native tests, we don't have a real system clock
  // Just set a valid time so TLS checks pass
  // This stub prevents linker errors when OtaUpdater.cpp is compiled
}

// TimeLib stubs
time_t now() {
  return 0;
}
void setTime(time_t) {}
void setTime(int, int, int, int, int, int) {}
time_t makeTime(const tmElements_t &) {
  return 0;
}
void breakTime(time_t, tmElements_t &) {}
int dayStr(int) {
  return 0;
}
int monthStr(int) {
  return 0;
}
time_t previousMidnight(time_t t) {
  (void)t;
  return 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// ESP32 hardware stubs
// ═══════════════════════════════════════════════════════════════════════════════

uint8_t temprature_sens_read() {
  return 25;
}
