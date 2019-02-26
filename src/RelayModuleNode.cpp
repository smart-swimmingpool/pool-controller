/**
 * Homie Node for Relay Module.
 *
 */
#include "RelayModuleNode.hpp"

RelayModuleNode::RelayModuleNode(const char* id, const char* name, const int pin, const int measurementInterval)
    : HomieNode(id, name, "switch") {
  _pin                 = pin;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = 0;

  relay = new RelayModule(_pin);
}

/**
 *
 */
void RelayModuleNode::setState(const boolean state) {

  if (state) {
    relay->on();
    setProperty("switch").send("on");
  } else {
    relay->off();
    setProperty("switch").send("off");
  }
}

/**
 *
 */
boolean RelayModuleNode::getState() {
  return relay->isOn();
}

/**
 *
 */
void RelayModuleNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}

/**
 *
 */
void RelayModuleNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    _lastMeasurement = millis();

    Homie.getLogger() << "〽 Sending Switch status: " << getId() << endl;

    const boolean state = getState();
    Homie.getLogger() << cIndent << "switch: " << state << endl;

    if (state) {
      setProperty("switch").send("on");
    } else {
      setProperty("switch").send("off");
    }
  }
}

/**
 *
 */
void RelayModuleNode::onReadyToOperate() {

  relayModuleSetting = new HomieSetting<boolean>(getId(), "stored switch configuration");
  relayModuleSetting->setDefaultValue(false);

  advertise("switch").setName("Switch").setDatatype("boolean");

  //restore from settings
  if (relayModuleSetting->get()) {
    relay->on();
  } else {
    relay->off();
  }
}

/**
 *
 */
void RelayModuleNode::setup() {
  printCaption();

  relay->on();
  relay->off();
}
