#pragma once

#include "TimeClientHelper.hpp"

struct TimerSetting {
  unsigned int timerStartHour;
  unsigned int timerStartMinutes;
  unsigned int timerEndHour;
  unsigned int timerEndMinutes;
};

tm getCurrentDateTime();
tm getStartTime(const tm &baseTime, TimerSetting ts);
tm getEndTime(const tm &baseTime, TimerSetting ts);
