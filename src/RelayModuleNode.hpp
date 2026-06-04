// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>
#include <RelayModule.h>
#include <Preferences.h>
#include <memory>

class RelayModuleNode {
public:
  RelayModuleNode(const char *id, const char *name, const uint8_t pin, const int measurementInterval = MEASUREMENT_INTERVAL);
  ~RelayModuleNode() = default;

  const char* getId() const { return _id; }
  uint8_t getPin() const { return _pin; }
  
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }
  
  void setSwitch(const bool state);
  bool getSwitch();

  void begin();
  void loop();

private:
  static const int MIN_INTERVAL = 10;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char *_id;
  const char *_name;
  uint8_t _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  std::unique_ptr<RelayModule> relay;
  Preferences preferences;
};
