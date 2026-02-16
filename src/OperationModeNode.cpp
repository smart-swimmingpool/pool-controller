// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "OperationModeNode.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
#include "Utils.hpp"
#include "StateManager.hpp"
#include "MqttInterface.hpp"

/**
 *
 */
OperationModeNode::OperationModeNode(const char* id, const char* name, const int measurementInterval)
    : HomieNode(id, name, "switch") {
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = 0;

  setRunLoopDisconnected(true);
  setRunLoopDisconnected(true);
}

/**
 *
 */
void OperationModeNode::addRule(Rule* rule) {
  _ruleVec.PushBack(rule);
}

/**
 *
 */
Rule* OperationModeNode::getRule() {
  Homie.getLogger() << F("getRule: mode=") << _mode << endl;

  for (int i = 0; i < _ruleVec.Size(); i++) {
    if (_mode.equals(_ruleVec[i]->getMode())) {
      Homie.getLogger() << F("getRule: Active Rule: ") << _ruleVec[i]->getMode() << endl;
      // update the properties
      _ruleVec[i]->setPoolMaxTemperature(getPoolMaxTemperature());
      _ruleVec[i]->setSolarMinTemperature(getSolarMinTemperature());
      _ruleVec[i]->setTemperatureHysteresis(getTemperatureHysteresis());
      _ruleVec[i]->setTimerSetting(getTimerSetting());

      _ruleVec[i]->setPoolTemperature(_currentPoolTempNode->getTemperature());
      _ruleVec[i]->setSolarTemperature(_currentSolarTempNode->getTemperature());

      return _ruleVec[i];
    }
  }

  return nullptr;
}

/**
 *
 */
bool OperationModeNode::setMode(String mode) {
  bool retval;

  if (mode.equals(STATUS_AUTO) || mode.equals(STATUS_MANU) || mode.equals(STATUS_BOOST) || mode.equals(STATUS_TIMER)) {
    _mode = mode;
    Homie.getLogger() << F("set mode: ") << _mode << endl;
    PoolController::MqttInterface::publishSelectState(
      *this, cMode, cMode, _mode.c_str());
    PoolController::MqttInterface::publishHomieProperty(
      *this, cHomieNodeState, cHomieNodeState_OK);
    saveState();  // Persist mode change
    retval = true;

  } else {
    Homie.getLogger() << F("✖ UNDEFINED Mode: ") << mode << F(" Current unchanged mode: ") << _mode << endl;
    PoolController::MqttInterface::publishHomieProperty(
      *this, cHomieNodeState, cHomieNodeState_Error);
    retval = false;
  }

  return retval;
}

/**
 *
 */
String OperationModeNode::getMode() {
  return _mode;
}

/**
 *
 */
void OperationModeNode::setup() {
  advertise(cHomieNodeState).setName(cHomieNodeStateName);
  advertise(cMode).setName(cModeName).setDatatype("enum").setFormat("manu,auto,boost,timer").settable();
  advertise(cPoolMaxTemp).setName(cPoolMaxTempName).setDatatype("float").setFormat("0:40").setUnit("°C").settable();
  advertise(cSolarMinTemp).setName(cSolarMinTempName).setDatatype("float").setFormat("0:100").setUnit("°C").settable();
  advertise(cHysteresis).setName(cHysteresisName).setDatatype("float").setFormat("0:10").setUnit("K").settable();

  advertise(cTimerStartHour).setName("Timer Start").setDatatype("float").setFormat("0:23").setUnit("hh").settable();
  advertise(cTimerStartMin).setName("Timer Start").setDatatype("float").setFormat("0:59").setUnit("MM").settable();

  advertise(cTimerEndHour).setName("Timer End").setDatatype("float").setFormat("0:23").setUnit("hh").settable();
  advertise(cTimerEndMin).setName("Timer End").setDatatype("float").setFormat("0:59").setUnit("MM").settable();

  advertise(cTimezone).setName(cTimezoneName).setDatatype("integer").setFormat("0:9").settable();
  advertise(cTimezoneInfo).setName(cTimezoneInfoName).setDatatype("string");
}

/**
 *
 */
void OperationModeNode::loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    Homie.getLogger() << F("〽 OperatioalMode update rule ") << endl;
    // call loop to evaluate the current rule
    Rule* rule = getRule();
    if (rule != nullptr) {
      rule->loop();
    } else {
      Homie.getLogger() << cIndent << F("✖ no rule defined: ") << _mode << endl;
    }
    if (Homie.isConnected()) {
      /*
      /*
      Homie.getLogger() << cIndent << F("mode: ") << _mode << endl;
      Homie.getLogger() << cIndent << F("SolarMinTemp: ") <<
                           _solarMinTemp << endl;
      Homie.getLogger() << cIndent << F("PoolMaxTemp:  ") <<
                           _poolMaxTemp << endl;
      Homie.getLogger() << cIndent << F("Hysteresis:   ") <<
                           _hysteresis << endl;
*/
      // Optimize memory: avoid String allocations by using stack
      // buffers. Buffer size: 20 bytes sufficient for temperature
      // values (-100.00 to 999.99)
      char buffer[20];

      PoolController::MqttInterface::publishSelectState(
          *this, cMode, cMode, _mode.c_str());

      Utils::floatToString(_solarMinTemp, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cSolarMinTemp, cSolarMinTemp, buffer);

      Utils::floatToString(_poolMaxTemp, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cPoolMaxTemp, cPoolMaxTemp, buffer);

      Utils::floatToString(_hysteresis, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cHysteresis, cHysteresis, buffer);

      Utils::intToString(_timerSetting.timerStartHour, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cTimerStartHour, cTimerStartHour, buffer);

      Utils::intToString(_timerSetting.timerStartMinutes, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cTimerStartMin, cTimerStartMin, buffer);

      Utils::intToString(_timerSetting.timerEndHour, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cTimerEndHour, cTimerEndHour, buffer);

      Utils::intToString(_timerSetting.timerEndMinutes, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cTimerEndMin, cTimerEndMin, buffer);

      Utils::intToString(getTimezoneIndex(), buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(
          *this, cTimezone, cTimezone, buffer);

      String tzInfo = getTimeInfoFor(getTimezoneIndex());
      PoolController::MqttInterface::publishTextState(
          *this, cTimezoneInfo, cTimezoneInfo, tzInfo.c_str());
    } else {
      Homie.getLogger() << F("✖ OperationalMode: not connected.") << endl;
    }

    _lastMeasurement = millis();
  }
}

/**
 * Handle update by Homie message.
 */
bool OperationModeNode::handleInput(const HomieRange& range, const String& property, const String& value) {
  printCaption();

  Homie.getLogger() << cIndent << F("〽 handleInput -> property '") << property << F("' value=") << value << endl;
  bool retval = applyProperty(property, value);

  // set 0 to force call of loop explicite on changes
  _lastMeasurement = 0;

  return retval;
}

bool OperationModeNode::handleHomeAssistantCommand(const char* property, const char* value) {
  printCaption();

  Homie.getLogger() << cIndent << F("〽 HA command -> property '") << property << F("' value=") << value << endl;
  bool retval = applyProperty(String(property), String(value));

  _lastMeasurement = 0;
  return retval;
}

bool OperationModeNode::applyProperty(const String& property, const String& value) {
  bool retval;

  if (property.equalsIgnoreCase(cMode)) {
    Homie.getLogger() << cIndent << F("✔ set operational mode: ") << value << endl;
    retval = this->setMode(value);

  } else if (property.equalsIgnoreCase(cHysteresis)) {
    Homie.getLogger() << cIndent << F("✔ hysteresis: ") << value << endl;
    _hysteresis = value.toFloat();
    retval      = true;

  } else if (property.equalsIgnoreCase(cSolarMinTemp)) {
    Homie.getLogger() << cIndent << F("✔ solar min temp: ") << value << endl;
    _solarMinTemp = value.toFloat();
    retval        = true;

  } else if (property.equalsIgnoreCase(cPoolMaxTemp)) {
    Homie.getLogger() << cIndent << F("✔ pool max temp: ") << value << endl;
    _poolMaxTemp = value.toFloat();
    retval       = true;

  } else if (property.equalsIgnoreCase(cTimerStartHour)) {
    Homie.getLogger() << cIndent << F("✔ Timer start hh: ") << value << endl;
    TimerSetting timerSetting   = getTimerSetting();
    timerSetting.timerStartHour = value.toInt();
    setTimerSetting(timerSetting);
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerStartMin)) {
    Homie.getLogger() << cIndent << F("✔  Timer start min.: ") << value << endl;
    TimerSetting timerSetting      = getTimerSetting();
    timerSetting.timerStartMinutes = value.toInt();
    setTimerSetting(timerSetting);
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerEndHour)) {
    Homie.getLogger() << cIndent << F("✔ Timer end h: ") << value << endl;
    TimerSetting timerSetting = getTimerSetting();
    timerSetting.timerEndHour = value.toInt();
    setTimerSetting(timerSetting);
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerEndMin)) {
    Homie.getLogger() << cIndent << F("✔ Timer end min.: ") << value << endl;
    TimerSetting timerSetting    = getTimerSetting();
    timerSetting.timerEndMinutes = value.toInt();
    setTimerSetting(timerSetting);
    retval = true;

  } else if (property.equalsIgnoreCase(cTimezone)) {
    Homie.getLogger() << cIndent << F("✔ Timezone: ") << value << endl;
    int tzIndex = value.toInt();
    if (tzIndex >= 0 && tzIndex < getTzCount()) {
      setTimezoneIndex(tzIndex);
      Homie.getLogger() << cIndent << F("  Set to: ") << getTimeInfoFor(tzIndex) << endl;
      // Note: This only updates the timezone at runtime; persistence is handled via configuration.
      retval = true;
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid timezone index: ") << tzIndex << endl;
      retval = false;
    }

  } else {
    retval = false;
  }

  return retval;
}

/**
 *
 */
void OperationModeNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}

/**
 * Load persisted state from storage
 */
void OperationModeNode::loadState() {
  using PoolController::StateManager;

  // Load operation mode
  String savedMode = StateManager::loadString("opmode", STATUS_AUTO);
  setMode(savedMode);

  // Load temperature settings
  _poolMaxTemp  = StateManager::loadFloat("poolMaxTemp", 28.5);
  _solarMinTemp = StateManager::loadFloat("solarMinTemp", 55.0);
  _hysteresis   = StateManager::loadFloat("hysteresis", 1.0);

  // Load timer settings
  _timerSetting.timerStartHour    = StateManager::loadInt("timerStartH", 10);
  _timerSetting.timerStartMinutes = StateManager::loadInt("timerStartM", 30);
  _timerSetting.timerEndHour      = StateManager::loadInt("timerEndH", 17);
  _timerSetting.timerEndMinutes   = StateManager::loadInt("timerEndM", 30);

  Homie.getLogger() << F("✓ State loaded from persistent storage") << endl;
}

/**
 * Save current state to persistent storage
 */
void OperationModeNode::saveState() {
  using PoolController::StateManager;

  StateManager::saveString("opmode", _mode);
  StateManager::saveFloat("poolMaxTemp", _poolMaxTemp);
  StateManager::saveFloat("solarMinTemp", _solarMinTemp);
  StateManager::saveFloat("hysteresis", _hysteresis);
  StateManager::saveInt("timerStartH", _timerSetting.timerStartHour);
  StateManager::saveInt("timerStartM", _timerSetting.timerStartMinutes);
  StateManager::saveInt("timerEndH", _timerSetting.timerEndHour);
  StateManager::saveInt("timerEndM", _timerSetting.timerEndMinutes);
}
