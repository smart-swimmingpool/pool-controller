
#include "RuleTimer.hpp"

/**
 *
 */
RuleTimer::RuleTimer(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;
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

  tm   time = getCurrentDateTime();
  bool retval;

  tm startTime = getStartTime(getTimerSetting());
  tm endTime   = getEndTime(getTimerSetting());

  Homie.getLogger() << cIndent << F("time=      ") << asctime(&time);
  Homie.getLogger() << cIndent << F("startTime= ") << asctime(&startTime);
  Homie.getLogger() << cIndent << F("endTime=   ") << asctime(&endTime);

  if (difftime(mktime(&time), mktime(&startTime)) >= 0 && difftime(mktime(&time), mktime(&endTime)) <= 0) {
    retval = true;

  } else {
    retval = false;
  }

  Homie.getLogger() << cIndent << F("checkPoolPumpTimer = ") << retval << endl;
  return retval;
}
