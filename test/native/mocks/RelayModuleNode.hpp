#pragma once
#include "Arduino.h"

// NOTE: The member layout matches the production RelayModuleNode class
// up to _currentState so that stubs.cpp's setSwitch/getSwitch (compiled
// with production-layout offsets) write/read the correct memory location
// when called on mock-sized objects.

class RelayModuleNode {
public:
  RelayModuleNode(const char *id, const char *name)
    : _id(id), _name(name), _pin(0) {}
  RelayModuleNode(const char *id, const char *name, uint8_t, bool = true)
    : _id(id), _name(name), _pin(0) {}

  void begin() {}
  void loop() {}

  bool getSwitch() const { return _currentState; }
  void setSwitch(bool s) { _currentState = s; }
  const char *getId() const { return _id; }
  const char *getName() const { return _name; }
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

private:
  const char *_id;                    // offset  0
  const char *_name;                  // offset  8
  uint8_t _pin = 0;                   // offset 16
  unsigned long _measurementInterval = 300; // offset 24 (after 7 bytes padding)
  unsigned long _lastMeasurement = 0;       // offset 32
  bool _currentState = false;         // offset 40 — matches production offset
  // (Preferences would follow at offset 48 in production, not needed here)
};

static_assert(sizeof(RelayModuleNode) >= 41,
  "RelayModuleNode mock too small — _currentState must be at offset 40");
