// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file ESP32TemperatureNode.hpp
 * @brief Internal ESP32 chip temperature sensor node.
 */

#pragma once

#include <Arduino.h>

extern "C" {
uint8_t temprature_sens_read();
}

/**
 * @brief Reads the ESP32 internal chip temperature sensor.
 *
 * Provides the on-die temperature of the ESP32 microcontroller.
 * Useful for monitoring enclosure temperature and detecting overheating.
 * Uses the ESP32 internal temperature sensor (temprature_sens_read).
 */
class ESP32TemperatureNode {
public:
  ESP32TemperatureNode(const char *id, const char *name, const int measurementInterval = MEASUREMENT_INTERVAL);

  float getTemperature() const { return _temperature; }
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

  void begin();
  void loop();

private:
  static const int MIN_INTERVAL = 10;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char *_id;
  const char *_name;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  float _temperature = NAN;
};
