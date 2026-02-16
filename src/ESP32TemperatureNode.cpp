// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * Homie Node for internal temperature sensor of ESP32.
 *
 */

#include "ESP32TemperatureNode.hpp"
#include "Utils.hpp"
#include "MqttInterface.hpp"

/**
 * @param id
 */
ESP32TemperatureNode::ESP32TemperatureNode(const char* id, const char* name, const int measurementInterval)
    : HomieNode(id, name, "temperature") {
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = millis();

  setRunLoopDisconnected(true);

  setRunLoopDisconnected(true);
}

/**
 *
 */
void ESP32TemperatureNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}

/**
 *
 */
void ESP32TemperatureNode::loop() {
#ifdef ESP32
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();

    Homie.getLogger() << F("〽 Sending Temperature: ") << getId() << endl;

    // internal temp of ESP
    const uint8_t temp_farenheit = temprature_sens_read();
    const double  temp           = (temp_farenheit - 32) / 1.8;

    Homie.getLogger() << cIndent << F("Temperature = ") << temp << cTemperatureUnit << endl;
    if (Homie.isConnected()) {
      // Optimize memory: avoid String allocation
      char buffer[16];
      Utils::floatToString(temp, buffer, sizeof(buffer));

      PoolController::MqttInterface::publishSensorState(
          *this, cTemperature, getId(), buffer);
      PoolController::MqttInterface::publishHomieProperty(
          *this, cHomieNodeState, cHomieNodeState_OK);
    }
  }
#endif
}

/**
 *
 */
void ESP32TemperatureNode::onReadyToOperate() {
  advertise(cTemperature).setName(cTemperatureName).setDatatype("float").setFormat("-50:100").setUnit(cTemperatureUnit);
  advertise(cHomieNodeState).setName(cHomieNodeStateName);
}
