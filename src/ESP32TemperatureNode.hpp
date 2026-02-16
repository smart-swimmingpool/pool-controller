/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>

#ifdef ESP32
extern "C" {

uint8_t temprature_sens_read();
}
#endif

class ESP32TemperatureNode : public HomieNode {

public:
  ESP32TemperatureNode(const char* id, const char* name, const int measurementInterval = MEASUREMENT_INTERVAL);

  float         getTemperature() const { return temperature; }
  void          setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

protected:
  void loop() override;
  void onReadyToOperate() override;

private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL         = 60;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;
  const char*      cCaption             = "• ESP32 Internal Temperature sensor:";
  const char*      cIndent              = "  ◦ ";

  const char* cTemperature     = "temperature";
  const char* cTemperatureName = "Temperature";
  const char* cTemperatureUnit = "°C";

  const char* cHomieNodeState     = "state";
  const char* cHomieNodeStateName = "State";

  const char* cHomieNodeState_OK    = "OK";
  const char* cHomieNodeState_Error = "Error";

  bool          _sensorFound = false;
  unsigned int  _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  float temperature = NAN;

  void printCaption();
};
