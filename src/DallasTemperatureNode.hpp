// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class DallasTemperatureNode {
public:
  DallasTemperatureNode(
    const char *id, const char *name, const uint8_t pin, const int measurementInterval = MEASUREMENT_INTERVAL);

  const char *getId() const { return _id; }
  uint8_t getPin() const { return _pin; }

  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

  float getTemperature() const { return _temperature; }
  bool isSensorFound() const { return _sensorFound; }

  void begin();
  void loop();

private:
  static const int MIN_INTERVAL = 10;  // in seconds (more granular loop support)
  static const int MEASUREMENT_INTERVAL = 300;
  static const int RECOVERY_INTERVAL = 5;  // seconds

  const char *_id;
  const char *_name;
  uint8_t _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;
  bool _sensorFound = false;

  float _temperature = NAN;

  OneWire oneWire;
  DallasTemperature sensor;
  uint8_t numberOfDevices;

  void address2String(const DeviceAddress deviceAddress, char *buffer, size_t size);
};
