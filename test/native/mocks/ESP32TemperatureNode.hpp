#pragma once
#include "Arduino.h"

class ESP32TemperatureNode {
public:
  ESP32TemperatureNode(const char *id, const char *name) : _id(id), _name(name) {}
  void begin() {}
  void loop() {}
  float getTemperature() const { return _temperature; }
  void setTemperature(float t) { _temperature = t; }
  const char *getId() const { return _id; }
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
private:
  const char *_id;
  const char *_name;
  unsigned long _measurementInterval = 300;
  float _temperature = 32.5f;
};
