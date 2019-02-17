/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>
#include <RelayModule.h>

class RelayModuleNode : public HomieNode {

public:
  RelayModuleNode(const char *id, const char* name, const int pin, const int measurementInterval = MEASUREMENT_INTERVAL);

  int getPin() const { return _pin; }
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }


protected:
  void setup() override;
  void loop() override;
  void onReadyToOperate() override;


private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL = 60; // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char *cCaption = "• Relay Module:";
  const char *cIndent = "  ◦ ";

  int _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;
  RelayModule* relay = NULL;
  
  void printCaption();
};
