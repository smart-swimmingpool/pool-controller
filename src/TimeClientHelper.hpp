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

void   timeClientSetup(const char* ntpServer);
int    getTzCount();
time_t getUtcTime();
time_t getTimeFor(int index, TimeChangeRule** tcr);
String getTimeInfoFor(int index);
String getFormattedTime(time_t rawTime);
void   setTimezoneIndex(int index);
int    getTimezoneIndex();
