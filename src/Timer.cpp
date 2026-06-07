// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Timer.cpp
 * @brief Timer calculation helpers — daily timer active checks and time parsing.
 */

#include "Timer.hpp"
#include "TimeClientHelper.hpp"
#include "ConfigManager.hpp"

/**
 * Get current date/time, with validation
 * Returns time with tm_year = -1 only when time degradation is RED
 * ( > 24h since last NTP sync or never synced).
 *
 * YELLOW degradation (1–24h) returns the millis()-estimated time so that
 * timer scheduling continues to work — the estimate is accurate to within
 * seconds over that window.
 */
tm getCurrentDateTime() {
  TimeChangeRule *tcr = NULL;
  time_t t = getTimeFor(getTimezoneIndex(), &tcr);
  struct tm timeinfo = *localtime(&t);

  // Only mark as invalid when time is effectively lost (RED)
  if (getTimeDegradation() == TimeDegradation::RED) {
    timeinfo.tm_year = -1;
  }

  return timeinfo;
}

tm getStartTime(const tm &baseTime, TimerSetting timerSetting) {
  tm startTime = baseTime;
  startTime.tm_hour = timerSetting.timerStartHour;
  startTime.tm_min = timerSetting.timerStartMinutes;
  startTime.tm_sec = 0;

  return startTime;
}

tm getEndTime(const tm &baseTime, TimerSetting timerSetting) {
  tm endTime = baseTime;
  endTime.tm_hour = timerSetting.timerEndHour;
  endTime.tm_min = timerSetting.timerEndMinutes;
  endTime.tm_sec = 0;

  return endTime;
}

uint16_t calculateEffectiveEndMinutes(uint16_t baseStartMinutes, uint16_t baseEndMinutes, float poolTemp) {
  auto &s = PoolController::ConfigManager::getSettings();

  // NaN or below threshold: no extension
  if (poolTemp != poolTemp || poolTemp <= static_cast<float>(s.tempCircThreshold)) {
    return baseEndMinutes;
  }

  // Base runtime = difference between start and end (handles midnight crossing)
  uint16_t baseRuntime;
  if (baseEndMinutes >= baseStartMinutes) {
    baseRuntime = baseEndMinutes - baseStartMinutes;
  } else {
    baseRuntime = (1440 - baseStartMinutes) + baseEndMinutes; // Midnight crossing
  }

  if (baseRuntime == 0) {
    return baseEndMinutes;
  }

  // Extension: diff °C × factor minutes/°C
  float diff = poolTemp - static_cast<float>(s.tempCircThreshold);
  float extraMinutes = diff * s.tempCircFactor;
  uint16_t extra = static_cast<uint16_t>(extraMinutes + 0.5f);

  uint16_t totalRuntime = baseRuntime + extra;

  // Cap to max runtime
  if (totalRuntime > s.tempCircMaxRuntime) {
    totalRuntime = s.tempCircMaxRuntime;
  }

  // Calculate new end minutes (add runtime to start, can wrap past midnight)
  uint16_t extended = baseStartMinutes + totalRuntime;

  Serial.printf("  → TempCirc: %.1f°C, base=%umin, extra=%umin, total=%umin, end=%02d:%02d\n",
    poolTemp, baseRuntime, extra, totalRuntime,
    (extended / 60) % 24, extended % 60);

  return extended;
}
