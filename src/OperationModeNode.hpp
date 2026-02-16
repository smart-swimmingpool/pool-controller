// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>
#include <Vector.h>

#include "DallasTemperatureNode.hpp"
#include "Rule.hpp"
#include "Timer.hpp"
#include "TimeClientHelper.hpp"

class OperationModeNode : public HomieNode {
public:
  OperationModeNode(const char* id, const char* name, const int measurementInterval = MEASUREMENT_INTERVAL);
  ~OperationModeNode() {
    // This could cause use after free - to bad it is designed that way
    // Delete ruleset on deletion of this object
    for (int i = 0; i < _ruleVec.Size(); i++)
      delete _ruleVec[i];
  }

  void     setMeasurementInterval(uint32_t interval) { _measurementInterval = interval; }
  uint32_t getMeasurementInterval() const { return _measurementInterval; }
  bool     setMode(String mode);
  String   getMode();
  void     addRule(Rule* rule);
  Rule*    getRule();

  void setPoolTemperatureNode(DallasTemperatureNode* node) { _currentPoolTempNode = node; }
  void setSolarTemperatureNode(DallasTemperatureNode* node) { _currentSolarTempNode = node; }

  void setPoolMaxTemperature(float temp) {
    _poolMaxTemp = temp;
    saveState();
  }
  float getPoolMaxTemperature() { return _poolMaxTemp; }

  void setSolarMinTemperature(float temp) {
    _solarMinTemp = temp;
    saveState();
  }
  float getSolarMinTemperature() { return _solarMinTemp; }

  void setTemperatureHysteresis(float temp) {
    _hysteresis = temp;
    saveState();
  }
  float getTemperatureHysteresis() { return _hysteresis; }

  void setTimerSetting(TimerSetting setting) {
    _timerSetting = setting;
    saveState();
  }
  TimerSetting getTimerSetting() { return _timerSetting; }

  void loadState();
  void saveState();
  bool handleHomeAssistantCommand(const char* property, const char* value);

  enum MODE { AUTO, MANU, BOOST };
  const char* STATUS_AUTO  = "auto";
  const char* STATUS_MANU  = "manu";
  const char* STATUS_BOOST = "boost";
  const char* STATUS_TIMER = "timer";

protected:
  void setup() override;
  void loop() override;
  bool handleInput(const HomieRange& range, const String& property, const String& value) override;

private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL         = 60;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;
  const char*      cCaption             = "• Operation Status:";
  const char*      cIndent              = "  ◦ ";

  const char* cMode     = "mode";
  const char* cModeName = "Operation Mode";

  const char* cPoolMaxTemp     = "pool-max-temp";
  const char* cPoolMaxTempName = "Max. Pool Temperature";

  const char* cSolarMinTemp     = "solar-min-temp";
  const char* cSolarMinTempName = "Min. Solar Temperature";

  const char* cHysteresis     = "hysteresis";
  const char* cHysteresisName = "Hysterese";

  const char* cTimerStartHour = "timer-start-h";
  const char* cTimerStartMin  = "timer-start-min";

  const char* cTimerEndHour = "timer-end-h";
  const char* cTimerEndMin  = "timer-end-min";

  const char* cTimezone     = "timezone";
  const char* cTimezoneName = "Timezone";
  const char* cTimezoneInfo = "timezone-info";
  const char* cTimezoneInfoName = "Timezone Info";

  const char* cHomieNodeState     = "state";
  const char* cHomieNodeStateName = "State";

  const char* cHomieNodeState_OK    = "OK";
  const char* cHomieNodeState_Error = "Error";

  String        _mode = STATUS_AUTO;
  float         _poolMaxTemp;
  float         _solarMinTemp;
  float         _hysteresis;
  Vector<Rule*> _ruleVec;

  DallasTemperatureNode* _currentPoolTempNode;
  DallasTemperatureNode* _currentSolarTempNode;

  TimerSetting _timerSetting;

  uint32_t _measurementInterval;
  uint32_t _lastMeasurement;

  bool applyProperty(const String& property, const String& value);
  void printCaption();
};
