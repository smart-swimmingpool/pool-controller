/**
 * Homie Node for RCSwitches (433MHz).
 *
 */
#include "RCSwitchNode.hpp"

/**
 *
 */
RCSwitchNode::RCSwitchNode(const char* id, const char* name, const int pin, const char* group, const char* device,
                           const int measurementInterval)
    : HomieNode(id, name, "switch") {

  _pin                 = pin;
  _group               = group;
  _device              = device;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = 0;
}

/**
 *
 */
void RCSwitchNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}

/**
 *
 */
void RCSwitchNode::setState(const boolean state) {

  if (state) {
    rcSwitch->switchOn(_group, _device);
    setProperty(cSwitch).send(cFlagOn);
  } else {
    rcSwitch->switchOff(_group, _device);
    setProperty(cSwitch).send(cFlagOff);
  }
  _state = state;
  preferences.begin(getId(), false);
  preferences.putBool(cSwitch, _state);
  preferences.end();
}

/**
 *
 */
void RCSwitchNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    Homie.getLogger() << "〽 Sending Switch status: " << getId() << endl;
    Homie.getLogger() << cIndent << "switch: " << _state << endl;

    if (_state) {
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
void RCSwitchNode::onReadyToOperate() {

  advertise(cSwitch).setName("Switch").setDatatype("boolean");
}

/**
 *
 */
void RCSwitchNode::setup() {
  //RS Switches via 433MHz
  rcSwitch = new RCSwitch();

  rcSwitch->enableTransmit(_pin);
  rcSwitch->setRepeatTransmit(10);
  rcSwitch->setPulseLength(350);

  preferences.begin(getId(), false);
  boolean storedSwitchValue = preferences.getBool(cSwitch, false);
  // Close the Preferences
  preferences.end();

  //restore from preferences
  if (storedSwitchValue) {
    rcSwitch->switchOn(_group, _device);
  } else {
    rcSwitch->switchOff(_group, _device);
  }
}
