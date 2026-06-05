// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "ESP32TemperatureNode.hpp"
#include "Utils.hpp"

ESP32TemperatureNode::ESP32TemperatureNode(const char *id, const char *name, const int measurementInterval) {
  _id = id;
  _name = name;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = millis();
  _temperature = NAN;
}

void ESP32TemperatureNode::begin() {
  Serial.printf("• ESP32 Internal Temp sensor '%s' initialized.\n", _id);
}

void ESP32TemperatureNode::loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();

    // Read internal temp of ESP32 (returns Fahrenheit)
    const uint8_t temp_farenheit = temprature_sens_read();

    // Convert to Celsius
    // Note: If the sensor reads 0 or is uncalibrated, it may return a value of 128
    // standard conversion is: C = (F - 32) / 1.8
    _temperature = (temp_farenheit - 32.0f) / 1.8f;

    Serial.printf("〽 ESP32 internal temp: %f °C\n", _temperature);
  }
}
