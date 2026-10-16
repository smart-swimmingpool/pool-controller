// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file ESP32TemperatureNode.hpp
 * @brief Internal ESP32 chip temperature sensor node.
 */

#pragma once

#include <Arduino.h>
#include <atomic>
#include <esp_idf_version.h>

#include "SensorSlots.hpp"

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
extern "C" {
uint8_t temprature_sens_read();
}
#endif

/**
 * @brief Reads the ESP32 internal chip temperature sensor.
 *
 * Provides the on-die temperature of the ESP32 microcontroller.
 * Useful for monitoring enclosure temperature and detecting overheating.
 * The measured value is published through SensorSlots so readers on the
 * control core never race with SensorTask on Core 0.
 */
class ESP32TemperatureNode {
public:
  ESP32TemperatureNode(const char *id, const char *name, const int measurementInterval = MEASUREMENT_INTERVAL);

  float getTemperature() const { return PoolController::SensorSlots::read(PoolController::SensorId::CONTROLLER); }
  void setMeasurementInterval(unsigned long interval) { _measurementInterval.store(interval, std::memory_order_relaxed); }
  unsigned long getMeasurementInterval() const { return _measurementInterval.load(std::memory_order_relaxed); }

  void begin();
  void loop();

private:
  static const int MIN_INTERVAL = 10;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char *_id;
  const char *_name;
  std::atomic<unsigned long> _measurementInterval{MEASUREMENT_INTERVAL};
  unsigned long _lastMeasurement;

  // Written only by SensorTask. Cross-task readers use SensorSlots.
  float _temperature = NAN;
};
