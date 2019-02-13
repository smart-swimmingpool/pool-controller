/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>
#include <OneWire.h>
#include <DallasTemperature.h>

class DallasTemperatureNode : public HomieNode {
private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL = 60; // in seconds
  static const int MEASUREMENT_INTERVAL = 300;
  const char *cCaption = "• DallasTemperature sensor:";
  const char *cIndent = "  ◦ ";

  const char *cStatus = "status";
  const char *cStatusName = "Status";
  const char *cTemperature = "temperature";
  const char *cTemperatureName = "Temperature";
  const char *cTemperatureUnit = "°C";

  bool _sensorFound = false;
  unsigned int _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  float temperature = NAN;

  DallasTemperature sensor;

  void printCaption();

protected:
  void setup() override;
  void loop() override;
  void onReadyToOperate() override;

public:
  DallasTemperatureNode(const char *id,
             const int pin,
             const int measurementInterval = MEASUREMENT_INTERVAL);

  float getTemperature() const { return temperature; }

};
