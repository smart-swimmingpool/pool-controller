// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file DallasTemperatureNode.hpp
 * @brief DS18B20 temperature sensor node — reads pool and solar temperatures.
 */

#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/**
 * @brief Reads temperature from a DS18B20 sensor on a OneWire bus.
 *
 * Supports automatic recovery: if the sensor disappears, it uses a shorter
 * polling interval (RECOVERY_INTERVAL) and rescans the bus each cycle.
 * Reports sensor status to the DegradationManager for system health tracking.
 */
class DallasTemperatureNode {
public:
  /**
   * @brief Construct a DallasTemperature node.
   * @param id  Unique node identifier (e.g. "solar-temp").
   * @param name  Human-readable name (e.g. "Solar Temperature").
   * @param pin  GPIO pin for the OneWire data line.
   * @param measurementInterval  Minimum interval between sensor reads (seconds).
   */
  DallasTemperatureNode(
    const char *id, const char *name, const uint8_t pin, const int measurementInterval = MEASUREMENT_INTERVAL);

  /** @brief Get the node identifier. */
  const char *getId() const { return _id; }
  /** @brief Get the OneWire GPIO pin number. */
  uint8_t getPin() const { return _pin; }

  /** @brief Set the measurement interval (seconds). */
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  /** @brief Get the measurement interval (seconds). */
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

  /** @brief Get the last successfully read temperature. @return Temperature in °C, or NAN if no valid read. */
  float getTemperature() const { return _temperature; }
  /** @brief Check if a sensor was found on the bus. @return true if at least one device is present. */
  bool isSensorFound() const { return _sensorFound; }

  /** @brief Initialize the OneWire bus and discover connected sensors. */
  void begin();
  /** @brief Read temperature periodically (respects measurementInterval). */
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

  /** @brief Format a DeviceAddress as a hex string. */
  void address2String(const DeviceAddress deviceAddress, char *buffer, size_t size);
};
