// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>
#include <vector>
#include <memory>

#include "DallasTemperatureNode.hpp"
#include "Rule.hpp"
#include "Timer.hpp"
#include "TimeClientHelper.hpp"

class OperationModeNode {
public:
  OperationModeNode(const char *id, const char *name, const int measurementInterval = MEASUREMENT_INTERVAL);
  ~OperationModeNode() = default;

  void setMeasurementInterval(uint32_t interval) { _measurementInterval = interval; }
  uint32_t getMeasurementInterval() const { return _measurementInterval; }

  bool setMode(String mode);
  String getMode() const { return _mode; }

  void addRule(Rule *rule);
  Rule *getRule();

  void setPoolTemperatureNode(DallasTemperatureNode *node) { _currentPoolTempNode = node; }
  void setSolarTemperatureNode(DallasTemperatureNode *node) { _currentSolarTempNode = node; }

  void setPoolMaxTemperature(float temp) {
    _poolMaxTemp = temp;
    if (!_suppressPersist)
      saveState();
  }
  float getPoolMaxTemperature() const { return _poolMaxTemp; }

  void setSolarMinTemperature(float temp) {
    _solarMinTemp = temp;
    if (!_suppressPersist)
      saveState();
  }
  float getSolarMinTemperature() const { return _solarMinTemp; }

  void setTemperatureHysteresis(float temp) {
    _hysteresis = temp;
    if (!_suppressPersist)
      saveState();
  }
  float getTemperatureHysteresis() const { return _hysteresis; }

  void setTimerSetting(TimerSetting setting) {
    _timerSetting = setting;
    if (!_suppressPersist)
      saveState();
  }
  TimerSetting getTimerSetting() const { return _timerSetting; }

  void loadState();
  void saveState();
  bool handleHomeAssistantCommand(const char *property, const char *value);
  bool applyProperty(const String &property, const String &value);

  static void suppressPersist(bool suppress) { _suppressPersist = suppress; }

  static constexpr const char *STATUS_AUTO = "auto";
  static constexpr const char *STATUS_MANU = "manu";
  static constexpr const char *STATUS_BOOST = "boost";
  static constexpr const char *STATUS_TIMER = "timer";

  void begin();
  void loop();

private:
  static bool _suppressPersist;

  static const int MIN_INTERVAL = 10;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char *_id;
  const char *_name;

  String _mode = STATUS_AUTO;
  float _poolMaxTemp = 28.5f;
  float _solarMinTemp = 55.0f;
  float _hysteresis = 1.0f;

  // Use unique_ptr to manage rules automatically and safely (F19 Fix!)
  std::vector<std::unique_ptr<Rule>> _ruleVec;

  DallasTemperatureNode *_currentPoolTempNode = nullptr;
  DallasTemperatureNode *_currentSolarTempNode = nullptr;

  TimerSetting _timerSetting;

  uint32_t _measurementInterval;
  uint32_t _lastMeasurement;
};
