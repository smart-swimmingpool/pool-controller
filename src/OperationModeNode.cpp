// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file OperationModeNode.cpp
 * @brief Operation mode selector — mode switching, rule delegation, NVS persistence.
 */

#include "OperationModeNode.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
#include "Utils.hpp"
#include "StateManager.hpp"

// Static member definition
bool OperationModeNode::_suppressPersist = false;

// Helper: Validate and parse float value from string
static bool parseFloat(const String &value, float &result, float minVal, float maxVal) {
  if (value.length() == 0)
    return false;

  bool hasDigit = false;
  bool hasDot = false;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (c == '-' || c == '+') {
      if (i != 0)
        return false;
    } else if (c == '.') {
      if (hasDot)
        return false;
      hasDot = true;
    } else if (c >= '0' && c <= '9') {
      hasDigit = true;
    } else {
      return false;
    }
  }

  if (!hasDigit)
    return false;

  result = value.toFloat();
  return (result >= minVal && result <= maxVal);
}

// Helper: Validate and parse int value from string
static bool parseInt(const String &value, int &result, int minVal, int maxVal) {
  if (value.length() == 0)
    return false;

  bool hasDigit = false;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (c == '-' || c == '+') {
      if (i != 0)
        return false;
    } else if (c >= '0' && c <= '9') {
      hasDigit = true;
    } else {
      return false;
    }
  }

  if (!hasDigit)
    return false;

  result = value.toInt();
  return (result >= minVal && result <= maxVal);
}

OperationModeNode::OperationModeNode(const char *id, const char *name, const int measurementInterval) {
  _id = id;
  _name = name;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = 0;
}

void OperationModeNode::addRule(Rule *rule) {
  // Transfer ownership to unique_ptr in standard vector (F19 Fix!)
  _ruleVec.push_back(std::unique_ptr<Rule>(rule));
}

Rule *OperationModeNode::getRule() {
  Serial.printf("getRule: mode = %s\n", _mode.c_str());

  for (size_t i = 0; i < _ruleVec.size(); i++) {
    if (_mode.equals(_ruleVec[i]->getMode())) {
      Serial.printf("getRule: Active Rule: %s\n", _ruleVec[i]->getMode());

      // Update ruleset properties
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

      return _ruleVec[i].get();
    }
  }

  return nullptr;
}

bool OperationModeNode::setMode(String mode) {
  if (mode.equals(STATUS_AUTO) || mode.equals(STATUS_MANU) || mode.equals(STATUS_BOOST) || mode.equals(STATUS_TIMER)) {
    // Reset temperature-based runtime extension on mode change
    for (auto &rule : _ruleVec) {
      rule->resetTemperatureExtension();
    }
    _mode = mode;
    Serial.printf("set mode: %s\n", _mode.c_str());
    if (!_suppressPersist)
      saveState();
    return true;
  } else {
    Serial.printf("✖ UNDEFINED Mode: %s. Current unchanged mode: %s\n", mode.c_str(), _mode.c_str());
    return false;
  }
}

void OperationModeNode::begin() {
  Serial.printf("• OperationMode Node '%s' initialized.\n", _id);
}

void OperationModeNode::loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();
    Serial.println("〽 OperationalMode update rule");

    // Check time synchronization status
    static bool lastTimeSyncState = isTimeSyncValid();
    bool currentTimeSyncState = isTimeSyncValid();

    if (!currentTimeSyncState && lastTimeSyncState) {
      Serial.println("  ⚠ WARNING: NTP time sync failed! Using cached estimate.");
    } else if (currentTimeSyncState && !lastTimeSyncState) {
      Serial.println("  ✓ NTP time sync recovered.");
    }
    lastTimeSyncState = currentTimeSyncState;

    // Evaluate the active rule
    Rule *rule = getRule();
    if (rule != nullptr) {
      rule->loop();
    } else {
      Serial.printf("  ✖ no rule defined for mode: %s. Falling back to manual.\n", _mode.c_str());
      _mode = STATUS_MANU;
      saveState();
    }
  }
}

bool OperationModeNode::handleHomeAssistantCommand(const char *property, const char *value) {
  Serial.printf("  ◦ HA command -> property '%s' value = %s\n", property, value);
  bool retval = applyProperty(String(property), String(value));
  _lastMeasurement = 0;  // Trigger instant loop evaluation
  return retval;
}

bool OperationModeNode::applyProperty(const String &property, const String &value) {
  bool retval = false;

  if (property.equalsIgnoreCase("mode")) {
    Serial.printf("  ✔ set operational mode: %s\n", value.c_str());
    retval = this->setMode(value);
  } else if (property.equalsIgnoreCase("hysteresis")) {
    Serial.printf("  ✔ hysteresis: %s\n", value.c_str());
    float newValue;
    if (parseFloat(value, newValue, 0.0f, 10.0f)) {
      if (newValue != _hysteresis) {
        _hysteresis = newValue;
        saveState();
      }
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid hysteresis value (must be 0-10): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("solar-min-temp")) {
    Serial.printf("  ✔ solar min temp: %s\n", value.c_str());
    float newValue;
    if (parseFloat(value, newValue, 0.0f, 60.0f)) {
      if (newValue != _solarMinTemp) {
        _solarMinTemp = newValue;
        saveState();
      }
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid solar min temp (must be 0-60°C): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("pool-max-temp")) {
    Serial.printf("  ✔ pool max temp: %s\n", value.c_str());
    float newValue;
    if (parseFloat(value, newValue, 0.0f, 40.0f)) {
      if (newValue != _poolMaxTemp) {
        _poolMaxTemp = newValue;
        saveState();
      }
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid pool max temp (must be 0-60°C): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("timer-start-h")) {
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 23)) {
      if ((unsigned int)newValue != timerSetting.timerStartHour) {
        timerSetting.timerStartHour = newValue;
        setTimerSetting(timerSetting);
      }
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid start hour (must be 0-23): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("timer-start-min")) {
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 59)) {
      if ((unsigned int)newValue != timerSetting.timerStartMinutes) {
        timerSetting.timerStartMinutes = newValue;
        setTimerSetting(timerSetting);
      }
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid start minutes (must be 0-59): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("timer-end-h")) {
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 23)) {
      if ((unsigned int)newValue != timerSetting.timerEndHour) {
        timerSetting.timerEndHour = newValue;
        setTimerSetting(timerSetting);
      }
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid end hour (must be 0-23): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("timer-end-min")) {
    TimerSetting timerSetting = getTimerSetting();
    int newValue;
    if (parseInt(value, newValue, 0, 59)) {
      if ((unsigned int)newValue != timerSetting.timerEndMinutes) {
        timerSetting.timerEndMinutes = newValue;
        setTimerSetting(timerSetting);
      }
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid end minutes (must be 0-59): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("timezone")) {
    int tzIndex = value.toInt();
    if (tzIndex >= 0 && tzIndex < getTzCount()) {
      setTimezoneIndex(tzIndex);
      retval = true;
    } else {
      Serial.printf("  ✖ Invalid timezone index: %d\n", tzIndex);
    }
  }

  return retval;
}

void OperationModeNode::loadState() {
  using PoolController::StateManager;

  String savedMode = StateManager::loadString("opmode", STATUS_AUTO);
  if (savedMode == STATUS_AUTO || savedMode == STATUS_MANU || savedMode == STATUS_BOOST || savedMode == STATUS_TIMER) {
    _mode = savedMode;
  }

  _poolMaxTemp = StateManager::loadFloat("poolMaxTemp", 28.5f);
  _solarMinTemp = StateManager::loadFloat("solarMinTemp", 55.0f);
  _hysteresis = StateManager::loadFloat("hysteresis", 1.0f);

  _timerSetting.timerStartHour = StateManager::loadInt("timerStartH", 10);
  _timerSetting.timerStartMinutes = StateManager::loadInt("timerStartM", 30);
  _timerSetting.timerEndHour = StateManager::loadInt("timerEndH", 17);
  _timerSetting.timerEndMinutes = StateManager::loadInt("timerEndM", 30);

  Serial.println("✓ Operational mode state loaded from persistent storage.");
}

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
