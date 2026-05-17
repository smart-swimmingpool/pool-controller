/*
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "TimeLib.h"
#include "Timezone.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

struct TimeZoneInfo {
  char description[21];  // 20 chars max
  Timezone *timezone;
};

// Minimum valid time for time sync validation (2020-01-01 00:00:00 UTC)
constexpr time_t MIN_VALID_TIME = 1577836800;

/**
 * Time degradation levels for the three-stage model:
 *   GREEN  — last NTP sync < 1h ago, time is accurate
 *   YELLOW — last sync 1–24h ago, millis()-estimate is usable
 *   RED    — last sync > 24h ago OR never synced, time is effectively lost
 */
enum class TimeDegradation : uint8_t {
  GREEN = 0,
  YELLOW = 1,
  RED = 2
};

void timeClientSetup(const char *ntpServer);
int getTzCount();
time_t getUtcTime();
time_t getTimeFor(int index, TimeChangeRule **tcr);
String getTimeInfoFor(int index);
String getFormattedTime(time_t rawTime);
void setTimezoneIndex(int index);
int getTimezoneIndex();
bool isTimeSyncValid();
TimeDegradation getTimeDegradation();
time_t getLastValidSyncTime();
