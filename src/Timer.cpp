#include "Timer.hpp"
#include "TimeClientHelper.hpp"

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
