
#include "RuleAuto.hpp"
#include <cmath>  // for isnan()

RuleAuto::RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;
}

void RuleAuto::loop() {
  Homie.getLogger() << cIndent << F("§ RuleAuto: loop") << endl;

  // Validate temperature readings before making decisions
  float poolTemp  = getPoolTemperature();
  float solarTemp = getSolarTemperature();

  if (isnan(poolTemp) || isnan(solarTemp)) {
    Homie.getLogger() << cIndent << F("⚠ RuleAuto: Invalid temperature readings detected") << endl;
    Homie.getLogger() << cIndent << F("  Pool temp: ") << poolTemp << endl;
    Homie.getLogger() << cIndent << F("  Solar temp: ") << solarTemp << endl;
    Homie.getLogger() << cIndent << F("  Turning off solar pump for safety") << endl;
    // Turn off solar pump for safety, but keep pool pump running on timer
    _solarRelay->setSwitch(false);
    _poolRelay->setSwitch(checkPoolPumpTimer());
    return;  // Skip rest of logic
  }

  _poolRelay->setSwitch(checkPoolPumpTimer());

  if (_poolRelay->getSwitch()) {
    // pool pump is running

    if (_solarRelay->getSwitch()) {
      // solar is on

      float hyst = getTemperatureHysteresis();
      if (getSolarTemperature() < (getSolarMinTemperature() - hyst)) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: Solar below min. required solar temp. (") << getSolarMinTemperature()
                          << F("). Switch solar off") << endl;
        _solarRelay->setSwitch(false);

      } else if (getPoolTemperature() >= (getSolarTemperature() + hyst)) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: Pool temp. (") << getPoolTemperature() << F(") reaches solar temp (")
                          << getSolarTemperature() << F("). Switch solar off") << endl;
        _solarRelay->setSwitch(false);

      } else if (getPoolTemperature() >= (getPoolMaxTemperature() + hyst)) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: Pool temp. (") << getPoolTemperature() << F(") above max. temperature (")
                          << getPoolMaxTemperature() << F("). Switch solar off") << endl;
        _solarRelay->setSwitch(false);

      } else {
        // leave it on.
        Homie.getLogger() << cIndent << F("§ RuleAuto: Solar on -> no change") << endl;
      }

    } else {
      // solar is off: !_solarRelay->getSwitch()
      if ((getPoolTemperature() <= getPoolMaxTemperature()) && (getPoolTemperature() <= getSolarTemperature()) &&
          (getSolarMinTemperature() <= getSolarTemperature())) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: below max. Temperature (") << getPoolMaxTemperature()
                          << F("). Switch solar on") << endl;
        _solarRelay->setSwitch(true);

      } else {
        // no change of status
        Homie.getLogger() << cIndent << F("§ RuleAuto: Solar off -> no change") << endl;
      }
    }
  } else {

    if (_solarRelay->getSwitch()) {
      Homie.getLogger() << cIndent << F("§ RuleAuto: pool pump is disabled. Switch solar off") << endl;
      _solarRelay->setSwitch(false);
    }
  }
  Homie.getLogger() << cIndent << F("§ RuleAuto: Pool temp. :     ") << getPoolTemperature() << endl;
  Homie.getLogger() << cIndent << F("§ RuleAuto: max. Pool temp.: ") << getPoolMaxTemperature() << endl;
  Homie.getLogger() << cIndent << F("§ RuleAuto: Solar temp. :     ") << getSolarTemperature() << endl;
  Homie.getLogger() << cIndent << F("§ RuleAuto: min. Solar temp.: ") << getSolarMinTemperature() << endl;
}

bool RuleAuto::checkPoolPumpTimer() {
  Homie.getLogger() << F("↕  checkPoolPumpTimer") << endl;

  tm time = getCurrentDateTime();

  // Check if time sync is valid
  if (time.tm_year == -1) {
    Homie.getLogger() << cIndent << F("⚠ Time sync invalid - timer disabled") << endl;
    // Return false (pump off) when time is invalid for safety
    // User should check MQTT for time sync error notifications
    return false;
  }

  bool retval;

  tm startTime = getStartTime(getTimerSetting());
  tm endTime   = getEndTime(getTimerSetting());

  Homie.getLogger() << cIndent << F("currenttime=") << asctime(&time);
  Homie.getLogger() << cIndent << F("startTime=  ") << asctime(&startTime);
  Homie.getLogger() << cIndent << F("endTime=    ") << asctime(&endTime);

  // Handle midnight crossing: check if timer spans midnight
  TimerSetting ts              = getTimerSetting();
  bool         crossesMidnight = (ts.timerStartHour > ts.timerEndHour) ||
                         (ts.timerStartHour == ts.timerEndHour && ts.timerStartMinutes > ts.timerEndMinutes);

  if (crossesMidnight) {
    // Timer crosses midnight (e.g., 22:00 - 02:00)
    // Active if: time >= start OR time <= end
    retval = (difftime(mktime(&time), mktime(&startTime)) >= 0) || (difftime(mktime(&time), mktime(&endTime)) <= 0);
  } else {
    // Normal case: timer within same day
    // Active if: time >= start AND time <= end
    retval = (difftime(mktime(&time), mktime(&startTime)) >= 0) && (difftime(mktime(&time), mktime(&endTime)) <= 0);
  }

  Homie.getLogger() << cIndent << F("checkPoolPumpTimer = ") << retval << endl;
  return retval;
}
