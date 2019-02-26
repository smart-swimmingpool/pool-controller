/**
 * Homie Node for Relays.
 *
 */

#pragma once

#include <Homie.hpp>
#include <RelayModule.h>
#include <Preferences.h>

class RelayModuleNode : public HomieNode {

public:
  RelayModuleNode(const char* id, const char* name, const int pin, const int measurementInterval = MEASUREMENT_INTERVAL);

  int           getPin() const { return _pin; }
  void          setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }
  void          setState(const boolean state);
  boolean       getState();

protected:
  void         setup() override;
  void         loop() override;
  void         onReadyToOperate() override;
  bool handleInput(const HomieRange& range, const String& property, const String& value) override;

private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL         = 60;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char* cCaption = "• Relay Module:";
  const char* cIndent  = "  ◦ ";

  const char* cSwitch  = "switch";
  const char* cFlagOn  = "on";
  const char* cFlagOff = "off";
  const char* cStatus  = "status";

  int           _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;
  RelayModule*  relay = NULL;

  Preferences preferences;

  void printCaption();
};
