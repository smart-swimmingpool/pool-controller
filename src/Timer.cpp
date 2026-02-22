#include "Timer.hpp"
#include "TimeClientHelper.hpp"

/**
 * Get current date/time, with validation
 * Returns time with tm_year = -1 if time sync is invalid
 */
tm getCurrentDateTime() {
  TimeChangeRule* tcr = NULL;
  time_t t = getTimeFor(getTimezoneIndex(), &tcr);
  struct tm timeinfo = *localtime(&t);

  // Mark as invalid if time sync has failed
  if (!isTimeSyncValid() || t < MIN_VALID_TIME) {
    timeinfo.tm_year = -1;
  }

  return timeinfo;
}

tm getStartTime(TimerSetting timerSetting) {
  tm startTime = getCurrentDateTime();
  startTime.tm_hour = timerSetting.timerStartHour;
  startTime.tm_min = timerSetting.timerStartMinutes;
  startTime.tm_sec = 0;

  return startTime;
}

tm getEndTime(TimerSetting timerSetting) {
  tm endTime = getCurrentDateTime();
  endTime.tm_hour = timerSetting.timerEndHour;
  endTime.tm_min = timerSetting.timerEndMinutes;
  endTime.tm_sec = 0;

  return endTime;
}
