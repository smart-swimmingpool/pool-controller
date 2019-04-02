
#include "RuleAuto.hpp"

/**
 *
 */
RuleAuto::RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;

  // Setup simpleDSTadjust Library rules
  struct dstRule startRule = {"MESZ", Last, Sun, Mar, 1, 3600};  // Daylight time = UTC/GMT -4 hours
  struct dstRule endRule   = {"MEZ", Last, Sun, Oct, 1, 0};      // Standard time = UTC/GMT -5 hour
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
  Homie.getLogger() << "checkPoolPumpTimer" << endl;

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

  if (difftime(mktime(time), mktime(startTime)) > 0 && difftime(mktime(time), mktime(endTime)) < 0) {
    retval = true;

  } else {
    retval = false;
  }

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

void computeTimeDifference(struct tm t1, struct tm t2, struct tm* difference) {

  if (t2.tm_sec > t1.tm_sec) {
    --t1.tm_min;
    t1.tm_sec += 60;
  }

  difference->tm_sec = t1.tm_sec - t2.tm_sec;
  if (t2.tm_min > t1.tm_min) {
    --t1.tm_hour;
    t1.tm_min += 60;
  }
  difference->tm_min  = t1.tm_min - t2.tm_min;
  difference->tm_hour = t1.tm_hour - t2.tm_hour;
}
