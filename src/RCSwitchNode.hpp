/**
 * Homie Node for RCSwitches (433MHz sender).
 *
 */

#pragma once

#include <Homie.hpp>
#include <RCSwitch.h>
#include <Preferences.h>

class RCSwitchNode : public HomieNode {

public:
  RCSwitchNode(const char* id, const char* name, const int pin, const char* group, const char* device,
               const int measurementInterval = MEASUREMENT_INTERVAL);

  void          setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }
  void          setState(const boolean state);
  boolean       getState() const { return _state; };

protected:
  void setup() override;
  void loop() override;
  void onReadyToOperate() override;

private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL         = 60;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char* cCaption = "• RC 433MHz switch:";
  const char* cIndent  = "  ◦ ";

  const char* cSwitch  = "switch";
  const char* cFlagOn  = "on";
  const char* cFlagOff = "off";

  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  int         _pin;
  const char* _group;
  const char* _device;
  RCSwitch*   rcSwitch = NULL;
  boolean     _state;

  Preferences preferences;

  void printCaption();
};
