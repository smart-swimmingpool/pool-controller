
#include "RuleAuto.hpp"
#include "TimeClientHelper.hpp"

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
void RuleAuto::addPumpSwitchNode(RelayModuleNode* node) {
  _poolRelay = node;
}

/**
 *
 */
void RuleAuto::addSolarSwitchNode(RelayModuleNode* node) {
  _solarRelay = node;
}

/**
 *
 */
void RuleAuto::loop() {

  _poolRelay->setSwitch(checkPoolPumpTimer());

  if (_poolRelay->getSwitch()) {
    if ((!_solarRelay->getSwitch()) && (getPoolTemperature() <= (getPoolMaxTemperature() - getTemperaturHysteresis()))) {
      Homie.getLogger() << cIndent << "RuleAuto: below max. Temperature. Switch solar on" << endl;
      _solarRelay->setSwitch(true);

    } else if ((_solarRelay->getSwitch()) && (getPoolTemperature() > (getPoolMaxTemperature() + getTemperaturHysteresis()))) {
      Homie.getLogger() << cIndent << "RuleAuto: Max. Temperature reached. Switch solar off" << endl;
      _solarRelay->setSwitch(false);

    } else {
      // no change of status
    }
  } else {
    Homie.getLogger() << cIndent << "RuleAuto: pool pump is disabled." << endl;
  }
}

/**
 *
 */
bool RuleAuto::checkPoolPumpTimer() {
  Homie.getLogger() << "↕  checkPoolPumpTimer" << endl;

  tm  time = getCurrentDateTime();
  bool retval;

  int startHour      = poolPumpStart.substring(0, 2).toInt();
  int startMin       = poolPumpStart.substring(3, 5).toInt();
  tm startTime      = getCurrentDateTime();
  startTime.tm_hour = startHour;
  startTime.tm_min  = startMin;
  startTime.tm_sec  = 0;

  int endHour      = poolPumpEnd.substring(0, 2).toInt();
  int endMin       = poolPumpEnd.substring(3, 5).toInt();
  tm endTime      = getCurrentDateTime();
  endTime.tm_hour = endHour;
  endTime.tm_min  = endMin;
  endTime.tm_sec  = 0;

  Homie.getLogger() << cIndent << "time=      " << asctime(&time);
  Homie.getLogger() << cIndent << "startTime= " << asctime(&startTime);
  Homie.getLogger() << cIndent << "endTime=   " << asctime(&endTime);

  if (difftime(mktime(&time), mktime(&startTime)) >= 0 && difftime(mktime(&time), mktime(&endTime)) <= 0) {
    retval = true;

  } else {
    retval = false;
  }

  Homie.getLogger() << cIndent << "checkPoolPumpTimer = " << retval << endl;
  return retval;
}

/**
 *
 */
tm RuleAuto::getCurrentDateTime() {

  TimeChangeRule *tcr = NULL;
  time_t     t        = getTimeFor(0, &tcr);
  struct tm timeinfo =  *localtime(&t);

  return timeinfo;
}
