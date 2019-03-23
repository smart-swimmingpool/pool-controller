/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>
#include <Vector.h>

#include "Rule.hpp"

class OperationModeNode : public HomieNode {

public:
  OperationModeNode(const char* id, const char* name, const int measurementInterval = MEASUREMENT_INTERVAL);

  void          setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }
  bool          setMode(String mode);
  String        getMode();
  void          addRule(Rule* rule);
  Rule*         getRule();

  enum MODE { AUTO, MANU, BOOST };
  const char* STATUS_AUTO  = "auto";
  const char* STATUS_MANU  = "manu";
  const char* STATUS_BOOST = "boost";

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
  const char* cModeName = "Mode";

  const char* cPoolMaxTemp     = "pool-max-temp";
  const char* cPoolMaxTempName = "Max. Pool Temperature";

  const char* cSolarMinTemp     = "solar-min-temp";
  const char* cSolarMinTempName = "Min. Solar Temperature";

  String        _mode;
  float         _poolMaxTemp;
  float         _solarMinTemp;
  Vector<Rule*> _ruleVec;

  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  void  printCaption();
  char* string2char(String comand);
};
