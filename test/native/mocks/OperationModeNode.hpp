#pragma once
#include "Arduino.h"
#include <string>
#include <functional>

#include "RuleAuto.hpp"
#include "RuleManu.hpp"
#include "RuleBoost.hpp"
#include "RuleTimer.hpp"

// TimerSetting is defined in src/Timer.hpp (pulled via Rule.hpp)

class OperationModeNode {
public:
  OperationModeNode() {}
  OperationModeNode(const char *id, const char *name, int) {}

  void begin() {}
  void loop() {}

  String getMode() const { return String(_mode.c_str()); }
  bool setMode(String mode) { return setMode(mode, "unspecified"); }
  bool setMode(String mode, const char *source) {
    _lastModeSource = source != nullptr ? source : "unspecified";
    _mode = mode.c_str();
    return true;
  }
  const char *getLastModeSource() const { return _lastModeSource.c_str(); }

  Rule *getRule() {
    if (_mode == "auto")
      return &_autoRule;
    if (_mode == "manu")
      return &_manuRule;
    if (_mode == "boost")
      return &_boostRule;
    if (_mode == "timer")
      return &_timerRule;
    return nullptr;
  }

  float getPoolMaxTemperature() const { return 28.0f; }
  float getSolarMinTemperature() const { return 35.0f; }
  float getTemperatureHysteresis() const { return 1.0f; }
  void setPoolMaxTemperature(float v) {}
  void setSolarMinTemperature(float v) {}
  void setTemperatureHysteresis(float v) {}

  TimerSetting getTimerSetting() const { return _timer; }
  void setTimerSetting(const TimerSetting &ts) { _timer = ts; }

  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }

  static constexpr const char *STATUS_AUTO = "auto";
  static constexpr const char *STATUS_MANU = "manu";
  static constexpr const char *STATUS_BOOST = "boost";
  static constexpr const char *STATUS_TIMER = "timer";

private:
  std::string _mode = "auto";
  std::string _lastModeSource = "";
  TimerSetting _timer;
  unsigned long _measurementInterval = 300;
  RuleAuto _autoRule;
  RuleManu _manuRule;
  RuleBoost _boostRule;
  RuleTimer _timerRule;
};
