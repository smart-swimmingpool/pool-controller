
#include "OperationStatusNode.hpp"

/**
 *
 */
OperationStatusNode::OperationStatusNode(const char* id, const char* name, const int measurementInterval)
    : HomieNode(id, name, "switch") {

  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = millis();
}

/**
 *
 */
void OperationStatusNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}

/**
 *
 */
void OperationStatusNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    Homie.getLogger() << "〽 Sending Temperature: " << getId() << endl;

    _lastMeasurement = millis();
  }
}

/**
 *
 */
void OperationStatusNode::onReadyToOperate() {
  //advertise(cTemperature).setName(cTemperatureName).setDatatype("float").setFormat("-50:100").setUnit(cTemperatureUnit);
}
