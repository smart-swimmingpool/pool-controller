
#include "RuleTimer.hpp"

/**
 *
 */
RuleTimer::RuleTimer(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay = poolRelay;
}

/**
 *
 */
void RuleTimer::loop() {
  Homie.getLogger() << cIndent << F("§ RuleTimer: loop") << endl;

  _poolRelay->setSwitch(checkPoolPumpTimer());

  if (_solarRelay->getSwitch()) {
    _solarRelay->setSwitch(false);
  }
}

/**
 *
 */
bool RuleTimer::checkPoolPumpTimer() {
  Homie.getLogger() << F("↕  checkPoolPumpTimer") << endl;

  tm time = getCurrentDateTime();
  // Check if time is valid (tm_year == -1 indicates RED degradation)
  if (time.tm_year == -1) {
    Homie.getLogger() << cIndent << F("⚠ Time sync RED - timer disabled") << endl;
    Homie.getLogger() << cIndent << F("  Pool pump stays ON for safety") << endl;
    // Pool pump stays ON for pool hygiene when time is unavailable.
    // Solar pump remains OFF in timer mode (handled by loop()).
    return true;
  }

  bool retval;

  // Capture timer setting once for consistency
  TimerSetting ts = getTimerSetting();
  tm startTime = getStartTime(time, ts);
  tm endTime = getEndTime(time, ts);

  Homie.getLogger() << cIndent << F("time=      ") << asctime(&time);
  Homie.getLogger() << cIndent << F("startTime= ") << asctime(&startTime);
  Homie.getLogger() << cIndent << F("endTime=   ") << asctime(&endTime);

  // Convert tm structs to time_t once to avoid multiple mktime calls
  time_t now = mktime(&time);
  time_t start = mktime(&startTime);
  time_t end = mktime(&endTime);

  // Handle midnight crossing: check if timer spans midnight
  bool crossesMidnight =
    (ts.timerStartHour > ts.timerEndHour) || (ts.timerStartHour == ts.timerEndHour && ts.timerStartMinutes > ts.timerEndMinutes);

  if (crossesMidnight) {
    // Timer crosses midnight (e.g., 22:00 - 02:00)
    // Active if: time >= start OR time <= end
    retval = (difftime(now, start) >= 0) || (difftime(now, end) <= 0);
  } else {
    // Normal case: timer within same day
    // Active if: time >= start AND time <= end
    retval = (difftime(now, start) >= 0) && (difftime(now, end) <= 0);
  }

  Homie.getLogger() << cIndent << F("checkPoolPumpTimer = ") << retval << endl;
  return retval;
}
