// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "OperationModeNode.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
#include "Utils.hpp"
#include "StateManager.hpp"
#include "MqttInterface.hpp"

// Static member definition
bool OperationModeNode::_suppressPersist = false;

// Helper: Validate and parse float value from MQTT string
static bool parseFloat(const String &value, float &result, float minVal, float maxVal) {
  if (value.length() == 0)
    return false;

  // Check if all characters are valid for a float
  bool hasDigit = false;
  bool hasDot = false;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (c == '-' || c == '+') {
      if (i != 0)
        return false;  // Sign only at start
    } else if (c == '.') {
      if (hasDot)
        return false;  // Only one decimal point
      hasDot = true;
    } else if (c >= '0' && c <= '9') {
      hasDigit = true;
    } else {
      return false;  // Invalid character
    }
  }

  if (!hasDigit)
    return false;  // Must have at least one digit

  result = value.toFloat();
  return (result >= minVal && result <= maxVal);
}

// Helper: Parse HH:MM time string into hours and minutes.
// Returns true on success; false if format is invalid or values out of range.
static bool parseTimeHHMM(const String &value, unsigned int &hour, unsigned int &minute) {
  if (value.length() != 5)
    return false;
  if (value.charAt(2) != ':')
    return false;

  for (int i = 0; i < 2; i++) {
    if (!isdigit(value.charAt(i)))
      return false;
    if (!isdigit(value.charAt(3 + i)))
      return false;
  }

  int h = (value.charAt(0) - '0') * 10 + (value.charAt(1) - '0');
  int m = (value.charAt(3) - '0') * 10 + (value.charAt(4) - '0');

  if (h < 0 || h > 23)
    return false;
  if (m < 0 || m > 59)
    return false;

  hour = static_cast<unsigned int>(h);
  minute = static_cast<unsigned int>(m);
  return true;
}

// Helper: Validate and parse int value from MQTT string
static bool parseInt(const String &value, int &result, int minVal, int maxVal) {
  if (value.length() == 0)
    return false;

  // Check if all characters are valid for an integer
  bool hasDigit = false;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (c == '-' || c == '+') {
      if (i != 0)
        return false;  // Sign only at start
    } else if (c >= '0' && c <= '9') {
      hasDigit = true;
    } else {
      return false;  // Invalid character
    }
  }

  if (!hasDigit)
    return false;  // Must have at least one digit

  result = value.toInt();
  return (result >= minVal && result <= maxVal);
}

/**
 *
 */
OperationModeNode::OperationModeNode(const char *id, const char *name, const int measurementInterval)
    : HomieNode(id, name, "switch") {
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = 0;

  setRunLoopDisconnected(true);
}

/**
 *
 */
void OperationModeNode::addRule(Rule *rule) {
  _ruleVec.PushBack(rule);
}

/**
 *
 */
Rule *OperationModeNode::getRule() {
  Homie.getLogger() << F("getRule: mode=") << _mode << endl;

  for (int i = 0; i < _ruleVec.Size(); i++) {
    if (_mode.equals(_ruleVec[i]->getMode())) {
      Homie.getLogger() << F("getRule: Active Rule: ") << _ruleVec[i]->getMode() << endl;
      // update the properties
      _ruleVec[i]->setPoolMaxTemperature(getPoolMaxTemperature());
      _ruleVec[i]->setSolarMinTemperature(getSolarMinTemperature());
      _ruleVec[i]->setTemperatureHysteresis(getTemperatureHysteresis());
      _ruleVec[i]->setTimerSetting(getTimerSetting());

      if (_currentPoolTempNode != nullptr) {
        _ruleVec[i]->setPoolTemperature(_currentPoolTempNode->getTemperature());
      }
      if (_currentSolarTempNode != nullptr) {
        _ruleVec[i]->setSolarTemperature(_currentSolarTempNode->getTemperature());
      }

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
    PoolController::MqttInterface::publishSelectState(*this, cMode, cMode, _mode.c_str());
    PoolController::MqttInterface::publishHomieProperty(*this, cHomieNodeState, cHomieNodeState_OK);
    if (!_suppressPersist)
      saveState();  // Persist mode change
    retval = true;

  } else {
    Homie.getLogger() << F("✖ UNDEFINED Mode: ") << mode << F(" Current unchanged mode: ") << _mode << endl;
    PoolController::MqttInterface::publishHomieProperty(*this, cHomieNodeState, cHomieNodeState_Error);
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

    // Check time synchronization status
    // Initialize from current state to avoid false "just failed" on first boot
    static bool lastTimeSyncState = isTimeSyncValid();
    bool currentTimeSyncState = isTimeSyncValid();

    if (!currentTimeSyncState && lastTimeSyncState) {
      // Time sync just failed
      Homie.getLogger() << F("⚠ WARNING: NTP time sync failed!") << endl;
      Homie.getLogger() << F("  Using cached time + millis()") << endl;
      Homie.getLogger() << F("  Timer mode may be inaccurate") << endl;
      if (Homie.isConnected()) {
        PoolController::MqttInterface::publishHomieProperty(*this, cHomieNodeState, "time-sync-failed");
      }
    } else if (currentTimeSyncState && !lastTimeSyncState) {
      // Time sync recovered
      Homie.getLogger() << F("✓ NTP time sync recovered") << endl;
      if (Homie.isConnected()) {
        PoolController::MqttInterface::publishHomieProperty(*this, cHomieNodeState, cHomieNodeState_OK);
      }
    }
    lastTimeSyncState = currentTimeSyncState;

    // call loop to evaluate the current rule
    Rule *rule = getRule();
    if (rule != nullptr) {
      rule->loop();
    } else {
      Homie.getLogger() << cIndent << F("✖ no rule defined: ") << _mode << endl;
      // Fallback to safe mode: switch to manual mode
      Homie.getLogger() << cIndent << F("⚠ No matching rule - switching to manual mode") << endl;
      _mode = STATUS_MANU;
      saveState();
      if (Homie.isConnected()) {
        PoolController::MqttInterface::publishHomieProperty(*this, cHomieNodeState, cHomieNodeState_Error);
      }
    }
    if (Homie.isConnected()) {
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

      PoolController::MqttInterface::publishSelectState(*this, cMode, cMode, _mode.c_str());

      Utils::floatToString(_solarMinTemp, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(*this, cSolarMinTemp, cSolarMinTemp, buffer);

      Utils::floatToString(_poolMaxTemp, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(*this, cPoolMaxTemp, cPoolMaxTemp, buffer);

      Utils::floatToString(_hysteresis, buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(*this, cHysteresis, cHysteresis, buffer);

      if (PoolController::MqttInterface::isHomeAssistant()) {
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02u:%02u", _timerSetting.timerStartHour, _timerSetting.timerStartMinutes);
        PoolController::MqttInterface::publishTextEntityState(*this, cTimerStart, cTimerStart, timeStr);
        snprintf(timeStr, sizeof(timeStr), "%02u:%02u", _timerSetting.timerEndHour, _timerSetting.timerEndMinutes);
        PoolController::MqttInterface::publishTextEntityState(*this, cTimerEnd, cTimerEnd, timeStr);
      } else {
        Utils::intToString(_timerSetting.timerStartHour, buffer, sizeof(buffer));
        PoolController::MqttInterface::publishNumberState(*this, cTimerStartHour, cTimerStartHour, buffer);

        Utils::intToString(_timerSetting.timerStartMinutes, buffer, sizeof(buffer));
        PoolController::MqttInterface::publishNumberState(*this, cTimerStartMin, cTimerStartMin, buffer);

        Utils::intToString(_timerSetting.timerEndHour, buffer, sizeof(buffer));
        PoolController::MqttInterface::publishNumberState(*this, cTimerEndHour, cTimerEndHour, buffer);

        Utils::intToString(_timerSetting.timerEndMinutes, buffer, sizeof(buffer));
        PoolController::MqttInterface::publishNumberState(*this, cTimerEndMin, cTimerEndMin, buffer);
      }

      Utils::intToString(getTimezoneIndex(), buffer, sizeof(buffer));
      PoolController::MqttInterface::publishNumberState(*this, cTimezone, cTimezone, buffer);

      String tzInfo = getTimeInfoFor(getTimezoneIndex());
      PoolController::MqttInterface::publishTextState(*this, cTimezoneInfo, cTimezoneInfo, tzInfo.c_str());
    } else {
      Homie.getLogger() << F("✖ OperationalMode: not connected.") << endl;
    }

    _lastMeasurement = millis();
  }
}

/**
 * Handle update by Homie message.
 */
bool OperationModeNode::handleInput(const HomieRange &range, const String &property, const String &value) {
  printCaption();

  Homie.getLogger() << cIndent << F("〽 handleInput -> property '") << property << F("' value=") << value << endl;
  bool retval = applyProperty(property, value);

  // set 0 to force call of loop explicite on changes
  _lastMeasurement = 0;

  return retval;
}

bool OperationModeNode::handleHomeAssistantCommand(const char *property, const char *value) {
  printCaption();

  Homie.getLogger() << cIndent << F("〽 HA command -> property '") << property << F("' value=") << value << endl;
  bool retval = applyProperty(String(property), String(value));

  _lastMeasurement = 0;
  return retval;
}

bool OperationModeNode::applyProperty(const String &property, const String &value) {
  bool retval;

  if (property.equalsIgnoreCase(cMode)) {
    Homie.getLogger() << cIndent << F("✔ set operational mode: ") << value << endl;
    retval = this->setMode(value);

  } else if (property.equalsIgnoreCase(cHysteresis)) {
    Homie.getLogger() << cIndent << F("✔ hysteresis: ") << value << endl;
    float newValue;
    if (parseFloat(value, newValue, 0.0, 10.0)) {
      if (newValue != _hysteresis) {
        _hysteresis = newValue;
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid hysteresis value (must be 0-10): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cSolarMinTemp)) {
    Homie.getLogger() << cIndent << F("✔ solar min temp: ") << value << endl;
    float newValue;
    if (parseFloat(value, newValue, 0.0, 60.0)) {
      if (newValue != _solarMinTemp) {
        _solarMinTemp = newValue;
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid solar min temp (must be 0-60°C): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cPoolMaxTemp)) {
    Homie.getLogger() << cIndent << F("✔ pool max temp: ") << value << endl;
    float newValue;
    if (parseFloat(value, newValue, 0.0, 60.0)) {
      if (newValue != _poolMaxTemp) {
        _poolMaxTemp = newValue;
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid pool max temp (must be 0-60°C): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerStart)) {
    Homie.getLogger() << cIndent << F("✔ Timer start HH:MM: ") << value << endl;
    TimerSetting timerSetting = getTimerSetting();
    unsigned int newHour, newMinute;
    if (parseTimeHHMM(value, newHour, newMinute)) {
      if (newHour != timerSetting.timerStartHour || newMinute != timerSetting.timerStartMinutes) {
        timerSetting.timerStartHour = newHour;
        timerSetting.timerStartMinutes = newMinute;
        setTimerSetting(timerSetting);
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid timer start time (must be HH:MM, 00:00-23:59): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerEnd)) {
    Homie.getLogger() << cIndent << F("✔ Timer end HH:MM: ") << value << endl;
    TimerSetting timerSetting = getTimerSetting();
    unsigned int newHour, newMinute;
    if (parseTimeHHMM(value, newHour, newMinute)) {
      if (newHour != timerSetting.timerEndHour || newMinute != timerSetting.timerEndMinutes) {
        timerSetting.timerEndHour = newHour;
        timerSetting.timerEndMinutes = newMinute;
        setTimerSetting(timerSetting);
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid timer end time (must be HH:MM, 00:00-23:59): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerStartHour)) {
    Homie.getLogger() << cIndent << F("✔ Timer start hh: ") << value << endl;
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 23)) {
      if ((unsigned int)newValue != timerSetting.timerStartHour) {
        timerSetting.timerStartHour = newValue;
        setTimerSetting(timerSetting);
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid start hour (must be 0-23): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerStartMin)) {
    Homie.getLogger() << cIndent << F("✔  Timer start min.: ") << value << endl;
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 59)) {
      if ((unsigned int)newValue != timerSetting.timerStartMinutes) {
        timerSetting.timerStartMinutes = newValue;
        setTimerSetting(timerSetting);
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid start minutes (must be 0-59): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerEndHour)) {
    Homie.getLogger() << cIndent << F("✔ Timer end h: ") << value << endl;
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 23)) {
      if ((unsigned int)newValue != timerSetting.timerEndHour) {
        timerSetting.timerEndHour = newValue;
        setTimerSetting(timerSetting);
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid end hour (must be 0-23): ") << value << endl;
    }
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerEndMin)) {
    Homie.getLogger() << cIndent << F("✔ Timer end min.: ") << value << endl;
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 59)) {
      if ((unsigned int)newValue != timerSetting.timerEndMinutes) {
        timerSetting.timerEndMinutes = newValue;
        setTimerSetting(timerSetting);
        saveState();  // Persist to survive reboot
      }
    } else {
      Homie.getLogger() << cIndent << F("✖ Invalid end minutes (must be 0-59): ") << value << endl;
    }
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

  // Load operation mode — assign directly so we don't re-persist what the
  // user already saved.  The Homie property publish will happen on the next
  // periodic update or on MQTT reconnect.
  String savedMode = StateManager::loadString("opmode", STATUS_AUTO);
  if (savedMode == STATUS_AUTO || savedMode == STATUS_MANU || savedMode == STATUS_BOOST || savedMode == STATUS_TIMER) {
    _mode = savedMode;
  }

  // Load temperature settings
  _poolMaxTemp = StateManager::loadFloat("poolMaxTemp", 28.5);
  _solarMinTemp = StateManager::loadFloat("solarMinTemp", 55.0);
  _hysteresis = StateManager::loadFloat("hysteresis", 1.0);

  // Load timer settings
  _timerSetting.timerStartHour = StateManager::loadInt("timerStartH", 10);
  _timerSetting.timerStartMinutes = StateManager::loadInt("timerStartM", 30);
  _timerSetting.timerEndHour = StateManager::loadInt("timerEndH", 17);
  _timerSetting.timerEndMinutes = StateManager::loadInt("timerEndM", 30);

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
