
#include "RuleAuto.hpp"

/**
 *
 */
RuleAuto::RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;

}


/**
 *
 */
void RuleAuto::loop() {

  _poolRelay->setSwitch(checkPoolPumpTimer());

  float hyst = getTemperaturHysteresis();

  if (_poolRelay->getSwitch()) {
    if((_solarRelay->getSwitch() && getSolarTemperature() < getSolarMinTemperature() + hyst)) {
      Homie.getLogger() << cIndent << F("§ RuleAuto: Solar below min. Temperature (") << getSolarMinTemperature() << F("). Switch solar off") << endl;
      _solarRelay->setSwitch(false);

    } else {

      if ((!_solarRelay->getSwitch()) &&
        ((getPoolTemperature() < (getPoolMaxTemperature() - hyst))
        || (getPoolTemperature() < (getSolarTemperature() - hyst)))
        ) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: below max. Temperature (") << getPoolMaxTemperature() << F("). Switch solar on") << endl;
        _solarRelay->setSwitch(true);

      } else if ((_solarRelay->getSwitch()) &&
        ((getPoolTemperature() > (getPoolMaxTemperature() + hyst ))
        || (getPoolTemperature() > (getSolarTemperature() + hyst)))
        ) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: Max. Temperature (") << getPoolMaxTemperature() << F(") reached. Switch solar off") << endl;
        _solarRelay->setSwitch(false);

      } else {
        // no change of status
        Homie.getLogger() << cIndent << F("§ RuleAuto: no change")<< endl;
      }
    }
  } else {
    Homie.getLogger() << cIndent << F("§ RuleAuto: pool pump is disabled.") << endl;
    if (_solarRelay->getSwitch()) {
      _solarRelay->setSwitch(false);
    }
  }
}

/**
 *
 */
bool RuleAuto::checkPoolPumpTimer() {
  Homie.getLogger() << F("↕  checkPoolPumpTimer") << endl;

  tm  time = getCurrentDateTime();
  bool retval;

  tm startTime = getStartTime(_timerSetting);
  tm endTime   = getEndTime(_timerSetting);

  Homie.getLogger() << cIndent << F("time=      ") << asctime(&time);
  Homie.getLogger() << cIndent << F("startTime= ") << asctime(&startTime);
  Homie.getLogger() << cIndent << F("endTime=   ") << asctime(&endTime);

  if (difftime(mktime(&time), mktime(&startTime)) >= 0
    && difftime(mktime(&time), mktime(&endTime)) <= 0) {
    retval = true;

  } else {
    retval = false;
  }

  Homie.getLogger() << cIndent << F("checkPoolPumpTimer = ") << retval << endl;
  return retval;
}

