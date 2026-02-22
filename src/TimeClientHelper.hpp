/*
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "TimeLib.h"
#include "Timezone.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

struct TimeZoneInfo {
  char      description[21];  // 20 chars max
  Timezone* timezone;
};

// Minimum valid time for time sync validation (2020-01-01 00:00:00 UTC)
constexpr time_t MIN_VALID_TIME = 1577836800;

void   timeClientSetup(const char* ntpServer);
int    getTzCount();
time_t getUtcTime();
time_t getTimeFor(int index, TimeChangeRule** tcr);
String getTimeInfoFor(int index);
String getFormattedTime(time_t rawTime);
void   setTimezoneIndex(int index);
int    getTimezoneIndex();
bool   isTimeSyncValid();
time_t getLastValidSyncTime();
