/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>
#include <OneWire.h>
#include <DallasTemperature.h>

class DallasTemperatureNode : public HomieNode {

public:
  DallasTemperatureNode(const char *id,
             const int pin,
             const int measurementInterval = MEASUREMENT_INTERVAL);

  int getPin() const { return _pin; }
  int getMeasurementInterval() const { return _measurementInterval; }
  float getTemperature() const { return temperature; }

protected:
  void setup() override;
  void loop() override;
  void onReadyToOperate() override;

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

  int _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  float temperature = NAN;

  DallasTemperature sensor;
  int numberOfDevices; // Number of temperature devices found

  void printCaption();
};
