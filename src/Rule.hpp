// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>
#include "Timer.hpp"

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
   * get the Mode for which the Rule is created.
   */
  virtual const char *getMode() = 0;
  virtual void loop() = 0;

protected:
  static constexpr const char *cIndent = "  ";

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
};
