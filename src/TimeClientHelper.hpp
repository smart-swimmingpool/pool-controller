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
 * Time degradation levels for the configurable three-stage model:
 *   GREEN  — last NTP sync < greenMaxHours, time is accurate
 *   YELLOW — last sync between greenMaxHours and redAfterHours,
 *            millis()-estimate is usable
 *   RED    — last sync > redAfterHours OR never synced, time is effectively lost
 *
 * Thresholds configured via HomieSettings (P9):
 *   time-loss-green-hours (default 1h)
 *   time-loss-red-hours   (default 24h)
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

/** Force an NTP sync attempt.  Returns true if a valid time was obtained. */
bool forceNtpUpdate();

/** Configure GREEN→YELLOW threshold (hours). Default 1. */
void setTimeDegradationGreenHours(uint8_t hours);

/** Return current GREEN→YELLOW threshold. */
uint8_t getTimeDegradationGreenHours();

/** Configure YELLOW→RED threshold (hours). Default 24.
 *  Enforces red > green to guarantee a non-zero YELLOW window. */
void setTimeDegradationRedHours(uint8_t hours);

/** Return current YELLOW→RED threshold. */
uint8_t getTimeDegradationRedHours();
