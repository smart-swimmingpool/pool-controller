// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file TimeClientHelper.hpp
 * @brief NTP time client and timezone database for pool scheduling.
 *
 * PoolController integration of the upstream TimeClientHelper by Lübbe Onken.
 * Provides an NTP-synced local time via TimeLib + timezone support, a table
 * of selectable timezone definitions, and helper functions to work with
 * time-of-day and day-of-week for rule scheduling.
 */

#pragma once

#include "TimeLib.h"
#include "Timezone.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <memory>

struct TimeZoneInfo {
  char description[40];  // friendly name, e.g. "Central Europe (CET/CEST)"
  Timezone *timezone;
};

// Minimum valid time for time sync validation (2020-01-01 00:00:00 UTC)
constexpr time_t MIN_VALID_TIME = 1577836800;

// External declaration for NTP client (defined in .cpp)
extern std::unique_ptr<NTPClient> timeClient;

/**
 * Time degradation levels for the configurable three-stage model:
 *   GREEN  — last NTP sync < greenMaxHours, time is accurate
 *   YELLOW — last sync between greenMaxHours and redAfterHours,
 *            millis()-estimate is usable
 *   RED    — last sync > redAfterHours OR never synced, time is effectively lost
 *
 * Thresholds configured via ConfigManager:
 *   time-loss-green-hours (default 1h)
 *   time-loss-red-hours   (default 24h)
 */
enum class TimeDegradation : uint8_t { GREEN = 0, YELLOW = 1, RED = 2 };

void timeClientSetup(const char *ntpServer);
void syncSystemClock();
int getTzCount();
time_t getUtcTime();
time_t getTimeFor(int index, TimeChangeRule **tcr);
String getTimeInfoFor(int index);
const char *const *getTimezoneLabelList();
int getTimezoneLabelCount();
int getTimezoneIndexFromLabel(const String &label);
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
