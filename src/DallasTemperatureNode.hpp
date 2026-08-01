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

#include "SensorSlots.hpp"

/**
 * @brief Reads temperature from a DS18B20 sensor on a OneWire bus.
 *
 * Supports automatic recovery: if the sensor disappears, it uses a shorter
 * polling interval (RECOVERY_INTERVAL) and rescans the bus each cycle.
 * Reports sensor status to the DegradationManager for system health tracking.
 *
 * ### Shared Bus Mode (NORVI AE01-R)
 *
 * When constructed with the shared-bus constructor, two nodes can share a
 * single OneWire bus. The "master" node (deviceIndex == 0) drives
 * requestTemperatures() while the "slave" (deviceIndex > 0) reads its
 * specific device address from the already-completed conversion.
 *
 * @note Shared bus mode is only enabled when `NORVI_AE01_R` is defined.
 */
class DallasTemperatureNode {
public:
  /**
   * @brief Construct a DallasTemperature node (dedicated bus).
   * @param id  Unique node identifier (e.g. "solar-temp").
   * @param name  Human-readable name (e.g. "Solar Temperature").
   * @param pin  GPIO pin for the OneWire data line.
   * @param measurementInterval  Minimum interval between sensor reads (seconds).
   */
  DallasTemperatureNode(
    const char *id, const char *name, const uint8_t pin, const int measurementInterval = MEASUREMENT_INTERVAL);

  /**
   * @brief Construct a DallasTemperature node on a shared OneWire bus.
   * @param id  Unique node identifier (e.g. "solar-temp").
   * @param name  Human-readable name (e.g. "Solar Temperature").
   * @param sharedSensor  Pointer to an externally owned DallasTemperature instance.
   * @param deviceIndex  Index of this node's sensor on the shared bus (0 = master, 1 = slave).
   * @param measurementInterval  Minimum interval between sensor reads (seconds).
   *
   * The master (index 0) calls requestTemperatures(). All nodes read their
   * respective device address from the conversion result. Requires a prior
   * call to sharedSensor->begin() before calling node.begin().
   */
  DallasTemperatureNode(const char *id, const char *name, DallasTemperature *sharedSensor, uint8_t deviceIndex,
    const int measurementInterval = MEASUREMENT_INTERVAL);

  /** @brief Get the node identifier. */
  const char *getId() const { return _id; }
  /** @brief Get the OneWire GPIO pin number. */
  uint8_t getPin() const { return _pin; }

  /** @brief Set the measurement interval (seconds). */
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  /** @brief Get the measurement interval (seconds). */
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

  /** @brief Get the last successfully read temperature. @return Temperature in °C, or NAN if no valid read. */
  float getTemperature() const { return PoolController::SensorSlots::read(slotId()); }
  /** @brief Check if a sensor was found on the bus. @return true if at least one device is present. */
  bool isSensorFound() const { return PoolController::SensorSlots::isFound(slotId()); }

  /** @brief Get number of devices detected on this node's bus. */
  uint8_t getDeviceCount() const { return numberOfDevices; }
  /** @brief Get the 8-byte ROM address of the device this node reads from. */
  const uint8_t *getDeviceAddress() const { return deviceAddress_; }
  /** @brief Format the node's device address into a hex string (17 chars). */
  void getDeviceAddressString(char *buffer, size_t size) const;
  /** @brief Get the address of a detected device by index. @return true if index is valid. */
  bool getDetectedDeviceAddress(uint8_t index, DeviceAddress addr) const;
  /** @brief Get temperature for any device on this node's bus by index. */
  float getDetectedDeviceTemperature(uint8_t index) const;

  /**
   * @brief Set a preferred device address filter.
   *
   * During begin(), the node scans the bus and finds the device whose ROM
   * address matches \p addr. If found, that device is used regardless of
   * deviceIndex. If not found, a warning is printed and deviceIndex is
   * used as fallback.
   */
  void setAddressFilter(const DeviceAddress addr);
  /** @brief Clear the address filter — reverts to deviceIndex-based selection. */
  void clearAddressFilter();
  /** @brief Check if an address filter is configured. */
  bool hasAddressFilter() const { return hasFilter_; }

  /** @brief Initialize the OneWire bus and discover connected sensors. */
  void begin();
  /** @brief Read temperature periodically (respects measurementInterval). */
  void loop();

  /**
   * @brief Start a temperature conversion (non-blocking on Core 0).
   *
   * In shared-bus mode only the master (deviceIndex 0) issues
   * requestTemperatures(); slaves just return. In dedicated mode the
   * node starts its own conversion.
   * @note Call from SensorTask; the result must be read later via
   *       finishMeasurement() after the conversion time has elapsed.
   */
  void beginMeasurement();

  /**
   * @brief Read the conversion result and publish it to SensorSlots.
   *
   * Reads the temperature from the bus, updates the internal state, reports
   * sensor status to DegradationManager, and writes the value into the
   * thread-safe SensorSlots for cross-task consumers.
   * @note Call from SensorTask after beginMeasurement() + conversion delay.
   */
  void finishMeasurement();

private:
  static const int MIN_INTERVAL = 10;  // in seconds (more granular loop support)
  static const int MEASUREMENT_INTERVAL = 300;
  static const int RECOVERY_INTERVAL = 5;  // seconds

  const char *_id;
  const char *_name;
  uint8_t _pin = 0;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;
  bool _sensorFound = false;

  float _temperature = NAN;

  // Own OneWire bus (dedicated mode) or nullptr (shared mode)
  OneWire oneWire;
  DallasTemperature sensor;

  // Address filter (persistent mapping across reboots)
  bool hasFilter_ = false;      ///< Whether an address filter is configured
  DeviceAddress filterAddr_{};  ///< Address to match during begin()

  // Shared bus members (only used in shared mode)
  DallasTemperature *sharedSensor_ = nullptr;  ///< External shared sensor (shared mode only)
  uint8_t deviceIndex_ = 0;                    ///< Device index on the shared bus
  bool isBusMaster_ = false;                   ///< True if this node drives requestTemperatures()
  DeviceAddress deviceAddress_{};              ///< Cached device address for shared bus reads

  uint8_t numberOfDevices;

  /** @brief Scan the bus for the current filter and update deviceAddress_.
   *  Called by begin(), setAddressFilter(), and clearAddressFilter().
   *  @return true if the device was found, false if fallback was used. */
  bool resolveFilter();

  /** @brief Map this node to its SensorSlots id. */
  PoolController::SensorId slotId() const;

  /** @brief Format a DeviceAddress as a hex string. */
  void address2String(const DeviceAddress deviceAddress, char *buffer, size_t size) const;
};
