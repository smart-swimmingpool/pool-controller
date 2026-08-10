// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

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
#include "LogCapture.hpp"

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
  LOG_DEBUG("getRule: mode = %s\n", _mode.c_str());

  for (size_t i = 0; i < _ruleVec.size(); i++) {
    if (_mode.equals(_ruleVec[i]->getMode())) {
      LOG_DEBUG("getRule: Active Rule: %s\n", _ruleVec[i]->getMode());

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
  return setMode(mode, "unspecified");
}

bool OperationModeNode::setMode(String mode, const char *source) {
  if (source == nullptr) {
    source = "unspecified";
  }
  if (mode.equals(STATUS_AUTO) || mode.equals(STATUS_MANU) || mode.equals(STATUS_BOOST) || mode.equals(STATUS_TIMER)) {
    if (!_mode.equals(mode)) {
      // Reset temperature-based runtime extension on mode change
      for (auto &rule : _ruleVec) {
        rule->resetTemperatureExtension();
      }
      PoolController::LogCapture::logEvent("MODE_CHANGED", "Mode changed %s -> %s (source=%s, persist=%s)", _mode.c_str(),
        mode.c_str(), source, _suppressPersist ? "no" : "yes");
      _mode = mode;
      LOG_DEBUG("set mode: %s (source=%s)\n", _mode.c_str(), source);
      if (!_suppressPersist)
        saveState();
    } else {
      LOG_INFO("Mode set requested: %s -> %s (source=%s, changed=no, persist=no)\n", _mode.c_str(), mode.c_str(), source);
    }
    return true;
  } else {
    LOG_ERROR("✖ UNDEFINED Mode: %s. Current unchanged mode: %s (source=%s)\n", mode.c_str(), _mode.c_str(), source);
    return false;
  }
}

void OperationModeNode::begin() {
  LOG_INFO("• OperationMode Node '%s' initialized.\n", _id);
}

void OperationModeNode::loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();
    LOG_DEBUG("〽 OperationalMode update rule\n");

    // Check time synchronization status
    static bool lastTimeSyncState = isTimeSyncValid();
    bool currentTimeSyncState = isTimeSyncValid();

    if (!currentTimeSyncState && lastTimeSyncState) {
      LOG_WARN("  ⚠ WARNING: NTP time sync failed! Using cached estimate.\n");
    } else if (currentTimeSyncState && !lastTimeSyncState) {
      LOG_INFO("  ✓ NTP time sync recovered.\n");
    }
    lastTimeSyncState = currentTimeSyncState;

    // Evaluate the active rule
    Rule *rule = getRule();
    if (rule != nullptr) {
      rule->loop();
    } else {
      LOG_ERROR("  ✖ no rule defined for mode: %s. Falling back to manual (source=rule-fallback:no-rule).\n", _mode.c_str());
      setMode(STATUS_MANU, "rule-fallback:no-rule");
    }
  }
}

bool OperationModeNode::handleHomeAssistantCommand(const char *property, const char *value) {
  LOG_DEBUG("  ◦ HA command -> property '%s' value = %s\n", property, value);
  bool retval = applyProperty(String(property), String(value));
  _lastMeasurement = 0;  // Trigger instant loop evaluation
  return retval;
}

bool OperationModeNode::applyProperty(const String &property, const String &value) {
  bool retval = false;

  if (property.equalsIgnoreCase("mode")) {
    LOG_INFO("  ✔ set operational mode: %s\n", value.c_str());
    retval = this->setMode(value, "ha:command");
  } else if (property.equalsIgnoreCase("hysteresis")) {
    LOG_INFO("  ✔ hysteresis: %s\n", value.c_str());
    float newValue;
    if (parseFloat(value, newValue, 0.0f, 10.0f)) {
      if (newValue != _hysteresis) {
        _hysteresis = newValue;
        saveState();
      }
      retval = true;
    } else {
      LOG_ERROR("  ✖ Invalid hysteresis value (must be 0-10): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("solar-min-temp")) {
    LOG_INFO("  ✔ solar min temp: %s\n", value.c_str());
    float newValue;
    if (parseFloat(value, newValue, 0.0f, 60.0f)) {
      if (newValue != _solarMinTemp) {
        _solarMinTemp = newValue;
        saveState();
      }
      retval = true;
    } else {
      LOG_ERROR("  ✖ Invalid solar min temp (must be 0-60°C): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("pool-max-temp")) {
    LOG_INFO("  ✔ pool max temp: %s\n", value.c_str());
    float newValue;
    if (parseFloat(value, newValue, 0.0f, 40.0f)) {
      if (newValue != _poolMaxTemp) {
        _poolMaxTemp = newValue;
        saveState();
      }
      retval = true;
    } else {
      LOG_ERROR("  ✖ Invalid pool max temp (must be 0-60°C): %s\n", value.c_str());
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
      LOG_ERROR("  ✖ Invalid start hour (must be 0-23): %s\n", value.c_str());
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
      LOG_ERROR("  ✖ Invalid start minutes (must be 0-59): %s\n", value.c_str());
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
      LOG_ERROR("  ✖ Invalid end hour (must be 0-23): %s\n", value.c_str());
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
      LOG_ERROR("  ✖ Invalid end minutes (must be 0-59): %s\n", value.c_str());
    }
  } else if (property.equalsIgnoreCase("timezone")) {
    int tzIndex = value.toInt();
    if (tzIndex >= 0 && tzIndex < getTzCount()) {
      setTimezoneIndex(tzIndex);
      retval = true;
    } else {
      LOG_ERROR("  ✖ Invalid timezone index: %d\n", tzIndex);
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

  LOG_INFO("✓ Operational mode loaded from persistent storage: %s (source=loadState)\n", _mode.c_str());

  _poolMaxTemp = StateManager::loadFloat("poolMaxTemp", 28.5f);
  _solarMinTemp = StateManager::loadFloat("solarMinTemp", 55.0f);
  _hysteresis = StateManager::loadFloat("hysteresis", 1.0f);

  _timerSetting.timerStartHour = StateManager::loadInt("timerStartH", 10);
  _timerSetting.timerStartMinutes = StateManager::loadInt("timerStartM", 30);
  _timerSetting.timerEndHour = StateManager::loadInt("timerEndH", 17);
  _timerSetting.timerEndMinutes = StateManager::loadInt("timerEndM", 30);

  LOG_INFO("✓ Operational mode state loaded from persistent storage.\n");
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
