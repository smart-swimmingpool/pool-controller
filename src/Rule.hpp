// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Rule.hpp
 * @brief Abstract base class for all operation mode rules.
 */

#pragma once

#include <Arduino.h>
#include "Timer.hpp"

/**
 * @brief Abstract base for operation mode rule implementations.
 *
 * Each concrete subclass (RuleAuto, RuleManu, RuleBoost, RuleTimer)
 * implements a specific pump control strategy. The base class provides
 * shared temperature accessors and the pool-pump timer logic.
 */
class Rule {
public:
  Rule() : _poolTemp(0.0), _solarTemp(0.0), _poolMaxTemp(0.0), _solarMinTemp(0.0), _hysteresis(0.0) {}
  virtual ~Rule() = default;

  void setPoolTemperature(float temp) { _poolTemp = temp; }
  float getPoolTemperature() const { return _poolTemp; }
  void setSolarTemperature(float temp) { _solarTemp = temp; }
  float getSolarTemperature() const { return _solarTemp; }

  void setPoolMaxTemperature(float temp) { _poolMaxTemp = temp; }
  float getPoolMaxTemperature() const { return _poolMaxTemp; }

  void setSolarMinTemperature(float temp) { _solarMinTemp = temp; }
  float getSolarMinTemperature() const { return _solarMinTemp; }

  void setTemperatureHysteresis(float temp) { _hysteresis = temp; }
  float getTemperatureHysteresis() const { return _hysteresis; }

  void setTimerSetting(TimerSetting setting) { _timerSetting = setting; }
  TimerSetting getTimerSetting() const { return _timerSetting; }

  /**
   * @brief Get the mode identifier for this rule.
   * @return String constant e.g. "auto", "manu", "boost", "timer".
   */
  virtual const char *getMode() = 0;
  virtual void loop() = 0;

  /**
   * @brief Reset temperature-based runtime extension (call on mode change).
   */
  void resetTemperatureExtension() { _activeEndMinutes = 0; }

  /**
   * @brief Get current temperature-extended end time in minutes since midnight.
   * @return 0 if no extension active, otherwise extended end in minutes since midnight.
   */
  uint16_t getActiveEndMinutes() const { return _activeEndMinutes; }

protected:
  static constexpr const char *cIndent = "  ";

  /**
   * @brief Check pool pump timer with temperature-based runtime extension.
   *
   * Extends the timer end time when pool temperature exceeds the configured
   * threshold (tempCircThreshold). The end time only moves later ("only-extend"
   * policy) to avoid the pump shutting off unexpectedly during a cycle.
   *
   * @param poolTemp  Current pool water temperature in °C. Pass NaN or <= 0
   *                  to use the standard timer without extension.
   * @return true if the pump should be ON (within the timer window or extended
   *         window).
   */
  bool checkPoolPumpTimer(float poolTemp) {
    Serial.println("↕  checkPoolPumpTimer");

    tm time = getCurrentDateTime();

    // Check if time sync is valid
    if (time.tm_year == -1) {
      Serial.println("  ⚠ Time sync RED - timer disabled");
      Serial.println("  Pool pump stays ON for safety & hygiene");
      return true;
    }

    TimerSetting ts = getTimerSetting();
    tm startTime = getStartTime(time, ts);
    tm endTime = getEndTime(time, ts);

    Serial.printf("  currenttime = %s", asctime(&time));
    Serial.printf("  startTime   = %s", asctime(&startTime));
    Serial.printf("  endTime     = %s", asctime(&endTime));

    time_t now = mktime(&time);
    time_t start = mktime(&startTime);
    time_t end = mktime(&endTime);

    bool crossesMidnight = (ts.timerStartHour > ts.timerEndHour) ||
      (ts.timerStartHour == ts.timerEndHour && ts.timerStartMinutes > ts.timerEndMinutes);

    // Determine if the base timer window is active
    bool timerActive;
    if (crossesMidnight) {
      timerActive = (difftime(now, start) >= 0) || (difftime(now, end) <= 0);
    } else {
      timerActive = (difftime(now, start) >= 0) && (difftime(now, end) <= 0);
    }

    if (!timerActive) {
      // Timer is not active → pump stays OFF
      _activeEndMinutes = 0;
      Serial.printf("  checkPoolPumpTimer = false\n");
      return false;
    }

    // Timer is active — apply temperature extension if temperature is valid
    if (poolTemp > 0.0f && poolTemp == poolTemp) {
      // Call the free function from Timer.hpp (uses ConfigManager internally)
      uint16_t baseStartMinutes = ts.timerStartHour * 60 + ts.timerStartMinutes;
      uint16_t baseEndMinutes = ts.timerEndHour * 60 + ts.timerEndMinutes;
      uint16_t extendedEnd = calculateEffectiveEndMinutes(baseStartMinutes, baseEndMinutes, poolTemp);

      // Only extend, never shorten
      if (extendedEnd > _activeEndMinutes) {
        _activeEndMinutes = extendedEnd;
        uint8_t eh = (_activeEndMinutes / 60) % 24;
        uint8_t em = _activeEndMinutes % 60;
        Serial.printf("  → Temperature extension: end now %02d:%02d\n", eh, em);
      }

      // Check if we're still within the extended window
      if (_activeEndMinutes > 0 && crossesMidnight) {
        // With midnight crossing of extended end, check if now < extended end
        uint16_t nowMinutes = time.tm_hour * 60 + time.tm_min;
        // Extended end wraps: active if now >= baseStart OR now <= extendedEnd
        if (nowMinutes >= baseStartMinutes || nowMinutes <= _activeEndMinutes) {
          Serial.printf("  checkPoolPumpTimer = true (extended, crosses midnight)\n");
          return true;
        }
      } else if (_activeEndMinutes > 0) {
        uint16_t nowMinutes = time.tm_hour * 60 + time.tm_min;
        if (nowMinutes < _activeEndMinutes) {
          Serial.printf("  checkPoolPumpTimer = true (extended to %02d:%02d)\n",
            (uint8_t)(_activeEndMinutes / 60), (uint8_t)(_activeEndMinutes % 60));
          return true;
        }
        // Extended window expired — reset
        _activeEndMinutes = 0;
      }
    }

    // Standard timer is active, no extension (or extension expired)
    Serial.printf("  checkPoolPumpTimer = true\n");
    return true;
  }

  /** @brief Standard timer check without temperature extension. */
  bool checkPoolPumpTimer() {
    Serial.println("↕  checkPoolPumpTimer");

    tm time = getCurrentDateTime();

    // Check if time sync is valid
    if (time.tm_year == -1) {
      Serial.println("  ⚠ Time sync RED - timer disabled");
      Serial.println("  Pool pump stays ON for safety & hygiene");
      return true;
    }

    bool retval;

    tm startTime = getStartTime(time, getTimerSetting());
    tm endTime = getEndTime(time, getTimerSetting());

    Serial.printf("  currenttime = %s", asctime(&time));
    Serial.printf("  startTime   = %s", asctime(&startTime));
    Serial.printf("  endTime     = %s", asctime(&endTime));

    // Convert tm structs to time_t once to avoid multiple mktime calls
    time_t now = mktime(&time);
    time_t start = mktime(&startTime);
    time_t end = mktime(&endTime);

    // Handle midnight crossing: check if timer spans midnight
    TimerSetting ts = getTimerSetting();
    bool crossesMidnight = (ts.timerStartHour > ts.timerEndHour) ||
      (ts.timerStartHour == ts.timerEndHour && ts.timerStartMinutes > ts.timerEndMinutes);

    if (crossesMidnight) {
      // Timer crosses midnight (e.g., 22:00 - 02:00)
      // Active if: time >= start OR time <= end
      retval = (difftime(now, start) >= 0) || (difftime(now, end) <= 0);
    } else {
      // Normal case: timer within same day
      // Active if: time >= start AND time <= end
      retval = (difftime(now, start) >= 0) && (difftime(now, end) <= 0);
    }

    Serial.printf("  checkPoolPumpTimer = %s\n", retval ? "true" : "false");
    return retval;
  }

  float _poolTemp;
  float _solarTemp;

  float _poolMaxTemp;
  float _solarMinTemp;

  float _hysteresis;

  TimerSetting _timerSetting;
  uint16_t _activeEndMinutes = 0;  ///< Temperature-extended end in minutes since midnight
};
