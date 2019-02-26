/**
 * Homie Node for internal temperature sensor of ESP32.
 *
 */

#include "ESP32TemperatureNode.hpp"

/**
 *
 */
ESP32TemperatureNode::ESP32TemperatureNode(const char* id, const char* name, const int measurementInterval)
    : HomieNode(id, name, "temperature") {

  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = millis();
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
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    Homie.getLogger() << "〽 Sending Temperature: " << getId() << endl;

    //internal temp of ESP
    uint8_t      temp_farenheit = temprature_sens_read();
    const double temp           = (temp_farenheit - 32) / 1.8;
    Homie.getLogger() << cIndent << "Temperature = " << temp << cTemperatureUnit << endl;
    setProperty(cTemperature).send(String(temp, 2));

    _lastMeasurement = millis();
  }
}

/**
 *
 */
void ESP32TemperatureNode::onReadyToOperate() {
  advertise(cTemperature).setName(cTemperatureName).setDatatype("float").setFormat("-50:100").setUnit(cTemperatureUnit);
}