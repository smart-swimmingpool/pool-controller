// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Timer.hpp
 * @brief Timer utility — timer-setting struct and time calculation helpers.
 */

#pragma once

#include "TimeClientHelper.hpp"

/**
 * @brief Timer configuration for pump scheduling.
 *
 * Defines the start and end time for the pool pump timer window.
 * Supports midnight crossing (e.g. 22:00 – 02:00).
 */
struct TimerSetting {
  unsigned int timerStartHour;
  unsigned int timerStartMinutes;
  unsigned int timerEndHour;
  unsigned int timerEndMinutes;
};

tm getCurrentDateTime();
tm getStartTime(const tm &baseTime, TimerSetting ts);
tm getEndTime(const tm &baseTime, TimerSetting ts);
