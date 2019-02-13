/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>

extern "C" {

uint8_t temprature_sens_read();
}

class ESP32TemperatureNode : public HomieNode {
private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL = 60; // in seconds
  static const int MEASUREMENT_INTERVAL = 300;
  const char *cCaption = "• ESP32 Internal Temperature sensor:";
  const char *cIndent = "  ◦ ";

  const char *cStatus = "status";
  const char *cStatusName = "Status";
  const char *cTemperature = "temperature";
  const char *cTemperatureName = "Temperature";
  const char *cTemperatureUnit = "°C";

  bool _sensorFound = false;
  unsigned int _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  float temperature = NAN;

  void printCaption();

protected:
  void setup() override;
  void loop() override;
  void onReadyToOperate() override;


public:
  ESP32TemperatureNode(const char *id, const int measurementInterval = MEASUREMENT_INTERVAL);

  float getTemperature() const {
    return temperature;
  }

};
