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
  DallasTemperatureNode(const char* id, const char* name, const uint8_t pin,
                        const int measurementInterval = MEASUREMENT_INTERVAL);

  uint8_t       getPin() const { return _pin; }
  void          setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }
  float         getTemperature() const { return temperature; }

protected:
  void setup() override;
  void loop() override;
  void onReadyToOperate() override;

private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL         = 60;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char* cCaption = "• DallasTemperature sensor:";
  const char* cIndent  = "  ◦ ";

  const char* cTemperature     = "temperature";
  const char* cTemperatureName = "Temperature";
  const char* cTemperatureUnit = "°C";

  const char* cHomieNodeState      = "state";
  const char* cHomieNodeStateName  = "State";

  const char* cHomieNodeState_OK    = "OK";
  const char* cHomieNodeState_Error = "Error";

  bool _sensorFound = false;

  uint8_t       _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  float temperature = NAN;

  OneWire*           oneWire;
  DallasTemperature* sensor;
  uint8_t            numberOfDevices;  // Number of temperature devices found

  void   printCaption();
  String address2String(const DeviceAddress deviceAddress);
};
