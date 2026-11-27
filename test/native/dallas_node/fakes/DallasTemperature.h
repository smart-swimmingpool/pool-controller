// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DallasTemperature.h
 * @brief Programmable DS18B20 bus fake for the DallasTemperatureNode tests.
 *
 * Tests place devices (address + next reading) on the bus and remove them to
 * simulate a probe dropping off. Shadows the inert mock in test/native/mocks.
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "OneWire.h"

#ifndef DEVICE_DISCONNECTED_C
#define DEVICE_DISCONNECTED_C -127.0f
#endif

class DallasTemperature {
public:
  struct Device {
    DeviceAddress address;
    float reading;
  };

  DallasTemperature() {}
  explicit DallasTemperature(OneWire *) {}

  // ── Test control ──
  void addDevice(const DeviceAddress address, float reading) {
    Device device{};
    std::memcpy(device.address, address, sizeof(DeviceAddress));
    device.reading = reading;
    devices_.push_back(device);
  }
  void removeDevice(const DeviceAddress address) {
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
      if (std::memcmp(it->address, address, sizeof(DeviceAddress)) == 0) {
        devices_.erase(it);
        return;
      }
    }
  }
  void setReading(const DeviceAddress address, float reading) {
    for (auto &device : devices_) {
      if (std::memcmp(device.address, address, sizeof(DeviceAddress)) == 0) {
        device.reading = reading;
      }
    }
  }
  int beginCalls() const { return beginCalls_; }

  // ── DallasTemperature API used by the node ──
  void begin() { ++beginCalls_; }
  void setOneWire(OneWire *) {}
  void setWaitForConversion(bool) {}
  void requestTemperatures() {}
  bool isParasitePowerMode() { return false; }
  uint8_t getDeviceCount() { return static_cast<uint8_t>(devices_.size()); }
  bool getAddress(uint8_t *address, uint8_t index) {
    if (index >= devices_.size()) {
      return false;
    }
    std::memcpy(address, devices_[index].address, sizeof(DeviceAddress));
    return true;
  }
  float getTempC(const uint8_t *address) {
    for (const auto &device : devices_) {
      if (std::memcmp(device.address, address, sizeof(DeviceAddress)) == 0) {
        return device.reading;
      }
    }
    return DEVICE_DISCONNECTED_C;
  }

private:
  std::vector<Device> devices_;
  int beginCalls_ = 0;
};
