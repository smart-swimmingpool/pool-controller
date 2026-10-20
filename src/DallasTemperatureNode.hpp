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

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#endif

#include "SensorSlots.hpp"
#include "TemperatureReadingFilter.hpp"

/**
 * @brief Reads temperature from a DS18B20 sensor on a OneWire bus.
 *
 * OneWire/DallasTemperature access is owned by SensorTask after startup.
 * Cross-core consumers only read SensorSlots or the small cached discovery
 * snapshot maintained by this class.
 */
class DallasTemperatureNode {
public:
  DallasTemperatureNode(PoolController::SensorId slot, const char *id, const char *name, const uint8_t pin,
    const int measurementInterval = MEASUREMENT_INTERVAL);

  DallasTemperatureNode(PoolController::SensorId slot, const char *id, const char *name, DallasTemperature *sharedSensor,
    uint8_t deviceIndex, const int measurementInterval = MEASUREMENT_INTERVAL);

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
  /** @brief Copy the selected ROM from the published cache (safe from any task). */
  void copySelectedAddress(DeviceAddress out) const;
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

  /** Minimum wait between beginMeasurement() and finishMeasurement() (12-bit DS18B20 conversion). */
  static constexpr uint32_t CONVERSION_DELAY_MS = 800;

  void begin();
  void beginMeasurement();
  void finishMeasurement();

private:
  static const int MIN_INTERVAL = 10;
  static const int MEASUREMENT_INTERVAL = 300;
  static const int RECOVERY_INTERVAL = 5;          // seconds
  static const uint8_t RESCAN_AFTER_FAILURES = 3;  // failed reads before the bus is rescanned
  static constexpr uint8_t MAX_DETECTED_DEVICES = 20;

  const PoolController::SensorId slot_;
  const char *_id;
  const char *_name;
  uint8_t _pin = 0;
  std::atomic<unsigned long> _measurementInterval{MEASUREMENT_INTERVAL};
  bool _sensorFound = false;
  float _temperature = NAN;
  PoolController::TemperatureReadingFilter filter_;  ///< Plausibility filter (85 °C power-on, range)
  uint8_t consecutiveFailures_ = 0;                  ///< Failed reads since the last valid one

  OneWire oneWire;
  DallasTemperature sensor;

  std::atomic<bool> hasFilter_{false};
  std::atomic<bool> mappingDirty_{false};
  DeviceAddress filterAddr_{};

  DallasTemperature *sharedSensor_ = nullptr;
  /** The bus this node reads: the shared NORVI bus or its own dedicated one. */
  DallasTemperature *bus() { return sharedSensor_ ? sharedSensor_ : &sensor; }
  uint8_t deviceIndex_ = 0;
  bool isBusMaster_ = false;
  DeviceAddress deviceAddress_{};
  uint8_t numberOfDevices = 0;

  // Cross-core metadata cache. The lock only protects short memory copies;
  // OneWire operations are always performed without holding it.
  // On ESP32 a portMUX critical section is used instead of a yield()-spinning
  // atomic_flag: on single-core targets (ESP32-C6) the higher-priority
  // SensorTask could otherwise spin forever on a lock held by the preempted
  // loop task, because yield() never hands the CPU to a lower priority.
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  mutable portMUX_TYPE cacheMux_ = portMUX_INITIALIZER_UNLOCKED;
#else
  mutable std::atomic_flag cacheLock_ = ATOMIC_FLAG_INIT;
#endif
  DeviceAddress cachedSelectedAddress_{};
  DeviceAddress detectedAddresses_[MAX_DETECTED_DEVICES]{};
  float detectedTemperatures_[MAX_DETECTED_DEVICES]{};
  uint8_t detectedCount_ = 0;

  bool resolveFilter();
  void applyPendingAddressFilter();
  void refreshDetectedSnapshot(bool includeTemperatures);
  void publishSelectedAddress();

  /** @brief Apply the plausibility filter to a raw reading and update the
   *  temperature and sensor health.
   *  @return true when repeated failures call for a bus rescan. */
  bool handleReading(float raw);

  /** @brief Address this node must find on the bus: the configured filter,
   *  else the previously resolved device. @return false if neither exists. */
  bool targetAddress(DeviceAddress target) const;

  /** @brief Rescan the bus and find this node's sensor again by address
   *  (e.g. after the sensor was disconnected or replaced). */
  void rescanBus();

  void lockCache() const;
  void unlockCache() const;

  PoolController::SensorId slotId() const { return slot_; }

  /** @brief Format a DeviceAddress as a hex string. */
  void address2String(const DeviceAddress deviceAddress, char *buffer, size_t size) const;
};
