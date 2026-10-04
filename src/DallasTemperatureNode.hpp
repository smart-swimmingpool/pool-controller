// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DallasTemperatureNode.hpp
 * @brief DS18B20 temperature sensor node — reads pool and solar temperatures.
 */

#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <atomic>

#include "SensorSlots.hpp"

/**
 * @brief Reads temperature from a DS18B20 sensor on a OneWire bus.
 *
 * OneWire/DallasTemperature access is owned by SensorTask after startup.
 * Cross-core consumers only read SensorSlots or the small cached discovery
 * snapshot maintained by this class.
 */
class DallasTemperatureNode {
public:
  DallasTemperatureNode(
    const char *id, const char *name, const uint8_t pin, const int measurementInterval = MEASUREMENT_INTERVAL);

  DallasTemperatureNode(const char *id, const char *name, DallasTemperature *sharedSensor, uint8_t deviceIndex,
    const int measurementInterval = MEASUREMENT_INTERVAL);

  const char *getId() const { return _id; }
  uint8_t getPin() const { return _pin; }

  void setMeasurementInterval(unsigned long interval) { _measurementInterval.store(interval, std::memory_order_relaxed); }
  unsigned long getMeasurementInterval() const { return _measurementInterval.load(std::memory_order_relaxed); }

  /** @brief Measurement interval including the 5 s recovery cadence. */
  unsigned long getEffectiveMeasurementInterval() const {
    return PoolController::SensorSlots::isFound(slotId()) ? getMeasurementInterval() : RECOVERY_INTERVAL;
  }

  float getTemperature() const { return PoolController::SensorSlots::read(slotId()); }
  bool isSensorFound() const { return PoolController::SensorSlots::isFound(slotId()); }

  /** @brief Get number of devices from the thread-safe discovery snapshot. */
  uint8_t getDeviceCount() const;
  const uint8_t *getDeviceAddress() const { return deviceAddress_; }
  void getDeviceAddressString(char *buffer, size_t size) const;
  bool getDetectedDeviceAddress(uint8_t index, DeviceAddress addr) const;
  float getDetectedDeviceTemperature(uint8_t index) const;

  /**
   * @brief Queue a preferred address for application by SensorTask.
   *
   * Before SensorTask starts, begin() resolves the queued filter during boot.
   * At runtime this method never touches OneWire; the next measurement cycle
   * applies the pending mapping on the sensor task.
   */
  void setAddressFilter(const DeviceAddress addr);
  void clearAddressFilter();
  bool hasAddressFilter() const { return hasFilter_.load(std::memory_order_acquire); }

  void begin();
  void loop();
  void beginMeasurement();
  void finishMeasurement();

private:
  static const int MIN_INTERVAL = 10;
  static const int MEASUREMENT_INTERVAL = 300;
  static const int RECOVERY_INTERVAL = 5;
  static constexpr uint8_t MAX_DETECTED_DEVICES = 20;
  static constexpr uint32_t SYNC_CONVERSION_DELAY_MS = 800;

  const char *_id;
  const char *_name;
  uint8_t _pin = 0;
  std::atomic<unsigned long> _measurementInterval{MEASUREMENT_INTERVAL};
  unsigned long _lastMeasurement;
  bool _sensorFound = false;
  float _temperature = NAN;

  OneWire oneWire;
  DallasTemperature sensor;

  std::atomic<bool> hasFilter_{false};
  std::atomic<bool> mappingDirty_{false};
  DeviceAddress filterAddr_{};

  DallasTemperature *sharedSensor_ = nullptr;
  uint8_t deviceIndex_ = 0;
  bool isBusMaster_ = false;
  DeviceAddress deviceAddress_{};
  uint8_t numberOfDevices = 0;

  // Cross-core metadata cache. The lock only protects short memory copies;
  // OneWire operations are always performed without holding it.
  mutable std::atomic_flag cacheLock_ = ATOMIC_FLAG_INIT;
  DeviceAddress cachedSelectedAddress_{};
  DeviceAddress detectedAddresses_[MAX_DETECTED_DEVICES]{};
  float detectedTemperatures_[MAX_DETECTED_DEVICES]{};
  uint8_t detectedCount_ = 0;

  bool resolveFilter();
  void applyPendingAddressFilter();
  void refreshDetectedSnapshot(bool includeTemperatures);
  void publishSelectedAddress();

  void lockCache() const;
  void unlockCache() const;

  PoolController::SensorId slotId() const;
  void address2String(const DeviceAddress deviceAddress, char *buffer, size_t size) const;
};
