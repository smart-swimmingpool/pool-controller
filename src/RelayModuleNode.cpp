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
}

/**
 *
 */
void RelayModuleNode::setState(const boolean state) {

  if (state) {
    relay->on();
    setProperty(cSwitch).send(cFlagOn);
  } else {
    relay->off();
    setProperty(cSwitch).send(cFlagOff);
  }

  preferences.begin(getId(), false);
  preferences.putBool(cSwitch, state);
  preferences.end();
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
    Homie.getLogger() << "〽 Sending Switch status: " << getId() << endl;

    const boolean state = getState();
    Homie.getLogger() << cIndent << "switch: " << state << endl;

    if (state) {
      setProperty(cSwitch).send(cFlagOn);
    } else {
      setProperty(cSwitch).send(cFlagOff);
    }

    _lastMeasurement = millis();
  }
}

/**
 *
 */
void RelayModuleNode::onReadyToOperate() {

  advertise(cSwitch).setName("Switch").setDatatype("boolean");
}

/**
 *
 */
void RelayModuleNode::setup() {
  relay = new RelayModule(_pin);

  preferences.begin(getId(), false);
  boolean storedSwitchValue = preferences.getBool(cSwitch, false);
  // Close the Preferences
  preferences.end();

  //restore from preferences
  if (storedSwitchValue) {
    relay->on();
  } else {
    relay->off();
  }
}
