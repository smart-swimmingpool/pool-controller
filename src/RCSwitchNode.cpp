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
 * Set the state of the switch and sent property message.
 */
void RCSwitchNode::setState(const boolean state) {

  if (state) {
    rcSwitch->switchOn(_group, _device);
  } else {
    rcSwitch->switchOff(_group, _device);
  }

  setProperty(cSwitch).send((state ? cFlagOn : cFlagOff));

  _state = state;

  //store state
  preferences.begin(getId(), false);
  preferences.putBool(cSwitch, _state);
  preferences.end();

  setProperty(cStatus).send("ok");

  Homie.getLogger() << cIndent << "RCSwitch is " << (state ? cFlagOn : cFlagOff) << endl;
}

/**
 * Handle update by Homie message.
 */
bool RCSwitchNode::handleInput(const HomieRange& range, const String& property, const String& value) {

  printCaption();

  Homie.getLogger() << cIndent << "〽 handleInput -> property '" << property << "' value=" << value << endl;

  bool retval;

  if (value != cFlagOn && value != cFlagOff) {

    Homie.getLogger() << "reveived invalid value for property [" + property + "]: " + value << endl;
    setProperty(cStatus).send("reveived invalid value for property [" + property + "]: " + value);

    retval = false;

  } else {

    const bool flag = (value == cFlagOn);
    setState(flag);

    retval = true;
  }

  Homie.getLogger() << "〽 handleInput <-" << retval << endl;
  return retval;
}

/**
 *
 */
void RCSwitchNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    Homie.getLogger() << "〽 Sending Switch status: " << getId() << endl;
    Homie.getLogger() << cIndent << "switch: " << _state << endl;

    setProperty(cSwitch).send((_state ? cFlagOn : cFlagOff));

    _lastMeasurement = millis();
  }
}

/**
 *
 */
void RCSwitchNode::onReadyToOperate() {

  advertise(cSwitch).setName("Switch").setDatatype("boolean").settable();
  advertise(cStatus).setName("Satus").setDatatype("string");
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

  printCaption();
  Homie.getLogger() << cIndent << "RCSwitch Pin: " << _pin << endl;

  preferences.begin(getId(), false);
  boolean storedSwitchValue = preferences.getBool(cSwitch, false);
  // Close the Preferences
  preferences.end();
  Homie.getLogger() << cIndent << "Restore status: " << storedSwitchValue << endl;

  //restore from preferences
  setState(storedSwitchValue);
}
