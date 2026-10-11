#pragma once
#include "Arduino.h"
#include "DallasTemperature.h"
#include <cstring>

class DallasTemperatureNode {
public:
  DallasTemperatureNode(const char *id, const char *name, uint8_t pin, int interval = 300)
    : _id(id), _name(name), _pin(pin), _measurementInterval(interval) {}
  DallasTemperatureNode(const char *id, const char *name, DallasTemperature*, uint8_t, int = 300)
    : _id(id), _name(name), _pin(0), _measurementInterval(300) {}

  void begin() {}
  void loop() {}

  float getTemperature() const { return _temperature; }
  void setTemperature(float t) { _temperature = t; }
  bool isSensorFound() const { return _sensorFound; }
  void setSensorFound(bool f) { _sensorFound = f; }
  uint8_t getDeviceCount() const { return 0; }
  const char *getId() const { return _id; }
  uint8_t getPin() const { return _pin; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }

  void setAddressFilter(const DeviceAddress address) {
    std::memcpy(_filterAddress, address, sizeof(_filterAddress));
    _hasFilter = true;
  }
  void clearAddressFilter() {
    std::memset(_filterAddress, 0, sizeof(_filterAddress));
    _hasFilter = false;
  }
  bool hasAddressFilter() const { return _hasFilter; }
  const uint8_t *getDeviceAddress() const { return _filterAddress; }

  void getDeviceAddressString(char *buf, size_t size) const {
    if (!_hasFilter) {
      snprintf(buf, size, "NONE");
      return;
    }
    snprintf(buf, size, "%02X%02X%02X%02X%02X%02X%02X%02X", _filterAddress[0], _filterAddress[1], _filterAddress[2],
      _filterAddress[3], _filterAddress[4], _filterAddress[5], _filterAddress[6], _filterAddress[7]);
  }
  bool getDetectedDeviceAddress(uint8_t, DeviceAddress) const { return false; }
  float getDetectedDeviceTemperature(uint8_t) const { return 0.0f; }

private:
  const char *_id;
  const char *_name;
  uint8_t _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement = 0;
  bool _sensorFound = false;
  float _temperature = 25.0f;
  bool _hasFilter = false;
  DeviceAddress _filterAddress{};
};
