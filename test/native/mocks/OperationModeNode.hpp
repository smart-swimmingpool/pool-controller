#pragma once
#include "Arduino.h"
#include <functional>
#include <string>

#include "OperationMode.hpp"
#include "RuleAuto.hpp"
#include "RuleManu.hpp"
#include "RuleBoost.hpp"
#include "RuleTimer.hpp"

// TimerSetting is defined in src/Timer.hpp (pulled via Rule.hpp)

class OperationModeNode {
public:
  OperationModeNode() {}
  OperationModeNode(const char *id, const char *name, int = 300) {}

  void begin() {}
  void loop() {}

  String getMode() const { return String(PoolController::toString(_mode)); }
  const char *getModeCStr() const { return PoolController::toString(_mode); }
  PoolController::OperationMode getTypedMode() const { return _mode; }

  bool setMode(String mode) { return setMode(mode, "unspecified"); }
  bool setMode(String mode, const char *source) {
    PoolController::OperationMode parsed{};
    if (!PoolController::tryParseOperationMode(mode.c_str(), parsed)) return false;
    return setMode(parsed, source);
  }
  bool setMode(PoolController::OperationMode mode) { return setMode(mode, "unspecified"); }
  bool setMode(PoolController::OperationMode mode, const char *source) {
    _lastModeSource = source != nullptr ? source : "unspecified";
    _mode = mode;
    return true;
  }
  const char *getLastModeSource() const { return _lastModeSource.c_str(); }

  Rule *getRule() {
    if (_mode == PoolController::OperationMode::AUTO)
      return &_autoRule;
    if (_mode == PoolController::OperationMode::MANUAL)
      return &_manuRule;
    if (_mode == PoolController::OperationMode::BOOST)
      return &_boostRule;
    if (_mode == PoolController::OperationMode::TIMER)
      return &_timerRule;
    return nullptr;
  }

  float getPoolMaxTemperature() const { return _poolMaxTemperature; }
  float getSolarMinTemperature() const { return _solarMinTemperature; }
  float getTemperatureHysteresis() const { return _temperatureHysteresis; }
  void setPoolMaxTemperature(float value) { _poolMaxTemperature = value; }
  void setSolarMinTemperature(float value) { _solarMinTemperature = value; }
  void setTemperatureHysteresis(float value) { _temperatureHysteresis = value; }

  TimerSetting getTimerSetting() const { return _timer; }
  void setTimerSetting(const TimerSetting &ts) { _timer = ts; }

  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

  static constexpr const char *STATUS_AUTO = "auto";
  static constexpr const char *STATUS_MANU = "manu";
  static constexpr const char *STATUS_BOOST = "boost";
  static constexpr const char *STATUS_TIMER = "timer";

private:
  PoolController::OperationMode _mode{PoolController::OperationMode::AUTO};
  std::string _lastModeSource = "";
  float _poolMaxTemperature = 28.0f;
  float _solarMinTemperature = 35.0f;
  float _temperatureHysteresis = 1.0f;
  TimerSetting _timer;
  unsigned long _measurementInterval = 300;
  RuleAuto _autoRule;
  RuleManu _manuRule;
  RuleBoost _boostRule;
  RuleTimer _timerRule;
};
