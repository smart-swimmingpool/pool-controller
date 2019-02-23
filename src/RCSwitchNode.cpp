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
    setProperty("switch").send("on");
  } else {
    rcSwitch->switchOff(_group, _device);
    setProperty("switch").send("off");
  }
  _state = state;
}

/**
 *
 */
void RCSwitchNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    Homie.getLogger() << "〽 Sending Switch status: " << getId() << endl;
    Homie.getLogger() << cIndent << "switch: " << _state << endl;

    if (_state) {
      setProperty("switch").send("on");
    } else {
      setProperty("switch").send("off");
    }

    _lastMeasurement = millis();
  }
}

/**
 *
 */
void RCSwitchNode::onReadyToOperate() {

  advertise("switch").setName("Switch").setDatatype("boolean");

  setState(false);
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
}
