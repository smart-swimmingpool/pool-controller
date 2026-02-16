#include "Timer.hpp"

/**
 *
 */
tm getCurrentDateTime() {

  TimeChangeRule* tcr      = NULL;
  time_t          t        = getTimeFor(getTimezoneIndex(), &tcr);
  struct tm       timeinfo = *localtime(&t);

  return timeinfo;
}

tm getStartTime(TimerSetting timerSetting) {
  tm startTime      = getCurrentDateTime();
  startTime.tm_hour = timerSetting.timerStartHour;
  startTime.tm_min  = timerSetting.timerStartMinutes;
  startTime.tm_sec  = 0;

  return startTime;
}

tm getEndTime(TimerSetting timerSetting) {
  tm endTime      = getCurrentDateTime();
  endTime.tm_hour = timerSetting.timerEndHour;
  endTime.tm_min  = timerSetting.timerEndMinutes;
  endTime.tm_sec  = 0;

  return endTime;
}
