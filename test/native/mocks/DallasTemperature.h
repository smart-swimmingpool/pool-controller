#pragma once

#include <stdint.h>
#include <cmath>
#include "OneWire.h"

#ifndef DEVICE_DISCONNECTED_C
#define DEVICE_DISCONNECTED_C -127.0f
#endif

class DallasTemperature {
public:
  DallasTemperature() {}
  explicit DallasTemperature(void *) {}

  void begin() {}
  void requestTemperatures() {}
  float getTempCByIndex(int) { return 25.0f; }
  float getTempC(const DeviceAddress &) { return 25.0f; }
  int getDeviceCount() { return 0; }
  bool getAddress(uint8_t *addr, int idx) { return false; }
  bool setResolution(const DeviceAddress &, int) { return true; }
  bool isConnected(const DeviceAddress &) { return false; }
  bool isValid(const DeviceAddress &) { return true; }
  void setOneWireBus(void *) {}
  void setOneWire(OneWire *) {}
  bool isParasitePowerMode(void) { return false; }
};
