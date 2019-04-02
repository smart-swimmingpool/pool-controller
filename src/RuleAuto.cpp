
#include "RuleAuto.hpp"

/**
 *
 */
RuleAuto::RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;

  // Setup simpleDSTadjust Library rules
  // TODO: externalize to make it configurable
  struct dstRule startRule = {"MESZ", Last, Sun, Mar, 2, 0};  // Daylight time = UTC/GMT -0 hours
  struct dstRule endRule   = {"MEZ", Last, Sun, Oct, 2, 3600};      // Standard time = UTC/GMT +1 hour
  _dstAdjusted             = new simpleDSTadjust(startRule, endRule);
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
  Homie.getLogger() << cIndent << "checkPoolPumpTimer" << endl;

  tm*  time = getCurrentDateTime();
  bool retval;

  int startHour      = poolPumpStart.substring(0, 2).toInt();
  int startMin       = poolPumpStart.substring(2, 2).toInt();
  tm* startTime      = getCurrentDateTime();
  startTime->tm_hour = startHour;
  startTime->tm_min  = startMin;

  int endHour      = poolPumpEnd.substring(0, 2).toInt();
  int endMin       = poolPumpEnd.substring(2, 2).toInt();
  tm* endTime      = getCurrentDateTime();
  endTime->tm_hour = endHour;
  endTime->tm_min  = endMin;

  Homie.getLogger() << cIndent << "time= " << asctime(time) << endl;
  Homie.getLogger() << cIndent << "startTime= " << asctime(startTime) << endl;
  Homie.getLogger() << cIndent << "endTime= " << asctime(endTime) << endl;

  if (difftime(mktime(time), mktime(startTime)) >= 0 && difftime(mktime(time), mktime(endTime)) <= 0) {
    retval = true;

  } else {
    retval = false;
  }

  Homie.getLogger() << cIndent << "checkPoolPumpTimer() = " << retval << endl;
  return retval;
}

/**
 *
 */
tm* RuleAuto::getCurrentDateTime() {

  char*      dstAbbrev;
  time_t     t        = _dstAdjusted->time(&dstAbbrev);
  struct tm* timeinfo = localtime(&t);

  return timeinfo;
}
