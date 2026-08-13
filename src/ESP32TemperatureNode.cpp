// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file ESP32TemperatureNode.cpp
 * @brief ESP32 internal chip temperature reader implementation.
 */

#include "ESP32TemperatureNode.hpp"
#include "Utils.hpp"
#include "LogCapture.hpp"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <driver/temperature_sensor.h>

namespace {
auto readInternalTemperatureCelsius(float &temperatureCelsius) -> bool {
  temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
  temperature_sensor_handle_t sensor = nullptr;

  esp_err_t err = temperature_sensor_install(&config, &sensor);
  if (err != ESP_OK) {
    LOG_ERROR("ESP32 internal temp sensor install failed: 0x%x (%s)\n", static_cast<unsigned>(err), esp_err_to_name(err));
    return false;
  }

  err = temperature_sensor_enable(sensor);
  if (err == ESP_OK) {
    err = temperature_sensor_get_celsius(sensor, &temperatureCelsius);
  }

  const esp_err_t disableErr = temperature_sensor_disable(sensor);
  if (disableErr != ESP_OK) {
    LOG_ERROR(
      "ESP32 internal temp sensor disable failed: 0x%x (%s)\n", static_cast<unsigned>(disableErr), esp_err_to_name(disableErr));
  }

  const esp_err_t uninstallErr = temperature_sensor_uninstall(sensor);
  if (uninstallErr != ESP_OK) {
    LOG_ERROR("ESP32 internal temp sensor uninstall failed: 0x%x (%s)\n", static_cast<unsigned>(uninstallErr),
      esp_err_to_name(uninstallErr));
  }

  if (err != ESP_OK) {
    LOG_ERROR("ESP32 internal temp sensor read failed: 0x%x (%s)\n", static_cast<unsigned>(err), esp_err_to_name(err));
    return false;
  }

  return true;
}
}  // namespace
#endif

ESP32TemperatureNode::ESP32TemperatureNode(const char *id, const char *name, const int measurementInterval) {
  _id = id;
  _name = name;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = millis();
  _temperature = NAN;
}

void ESP32TemperatureNode::begin() {
  LOG_INFO("• ESP32 Internal Temp sensor '%s' initialized.\n", _id);
}

void ESP32TemperatureNode::loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    float temperatureCelsius = NAN;
    if (readInternalTemperatureCelsius(temperatureCelsius)) {
      _temperature = temperatureCelsius;
    } else {
      _temperature = NAN;
    }
#else
    // Read internal temp of ESP32 (returns Fahrenheit)
    const uint8_t temp_farenheit = temprature_sens_read();

    // Convert to Celsius
    // Note: If the sensor reads 0 or is uncalibrated, it may return a value of 128
    // standard conversion is: C = (F - 32) / 1.8
    _temperature = (temp_farenheit - 32.0f) / 1.8f;
#endif

    LOG_DEBUG("〽 ESP32 internal temp: %f °C\n", _temperature);
  }
}
