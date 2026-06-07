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

/**
 * @brief Calculate temperature-extended end time in minutes since midnight.
 *
 * Computes: baseRuntime + max(0, (poolTemp - threshold) × factor), capped to maxRuntime.
 * The result is returned as minutes-since-midnight of the (possibly extended) end time.
 * May exceed 1440 (midnight crossing).
 *
 * @param baseStartMinutes  Timer start as minutes since midnight (e.g. 10*60 = 600).
 * @param baseEndMinutes    Timer end as minutes since midnight (e.g. 18*60 = 1080).
 * @param poolTemp          Current pool water temperature in °C.
 * @return  Extended end time in minutes since midnight, or baseEndMinutes if no extension.
 */
uint16_t calculateEffectiveEndMinutes(uint16_t baseStartMinutes, uint16_t baseEndMinutes, float poolTemp);
