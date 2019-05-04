/**
 * Homie Node for RCSwitches (433MHz).
 *
 */
#include "RCSwitchNode.hpp"

/**
 *
 */
RCSwitchNode::RCSwitchNode(const char* id, const char* name, const uint8_t pin, const char* group, const char* device,
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

  if(Homie.isConnected()) {
    setProperty(cSwitch).send((state ? cFlagOn : cFlagOff));
  }

  _state = state;

  //store state

#ifdef ESP32
  preferences.begin(getId(), false);
  preferences.putBool(cSwitch, _state);
  preferences.end();
#elif defined(ESP8266)

#endif

  if(Homie.isConnected()) {
    setProperty(cHomieNodeState).send(cHomieNodeState_OK);
  }
  Homie.getLogger() << cIndent << F("RCSwitch is ") << (state ? cFlagOn : cFlagOff) << endl;
}

/**
 * Handle update by Homie message.
 */
bool RCSwitchNode::handleInput(const HomieRange& range, const String& property, const String& value) {

  printCaption();

  Homie.getLogger() << cIndent << F("〽 handleInput -> property '") << property << F("' value=") << value << endl;

  bool retval;

  if (value != cFlagOn && value != cFlagOff) {

    Homie.getLogger() << F("reveived invalid value for property [") << property << F("]: ") << value << endl;
    if(Homie.isConnected()) {
      setProperty(cHomieNodeState).send(cHomieNodeState_Error);
    }

    retval = false;

  } else {

    const bool flag = (value == cFlagOn);
    setState(flag);

    retval = true;
  }

  Homie.getLogger() << F("〽 handleInput <-") << retval << endl;
  return retval;
}

/**
 *
 */
void RCSwitchNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    _lastMeasurement = millis();

    Homie.getLogger() << F("〽 Sending Switch status: ") << getId() << endl;
    Homie.getLogger() << cIndent << F("switch: ") << _state << endl;

    if(Homie.isConnected()) {
      setProperty(cSwitch).send((_state ? cFlagOn : cFlagOff));
    }
  }
}

/**
 *
 */
void RCSwitchNode::onReadyToOperate() {

  advertise(cSwitch).setName(cSwitchName).setDatatype("boolean").settable();
  advertise(cHomieNodeState).setName(cHomieNodeStateName).setDatatype("string");
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
  Homie.getLogger() << cIndent << F("RCSwitch Pin: ") << _pin << endl;

#ifdef ESP32
  preferences.begin(getId(), false);
  boolean storedSwitchValue = preferences.getBool(cSwitch, false);
  // Close the Preferences
  preferences.end();
#elif defined(ESP8266)
  boolean storedSwitchValue = false;
#endif

  Homie.getLogger() << cIndent << F("Restore status: ") << storedSwitchValue << endl;

  //restore from preferences
  setState(storedSwitchValue);
}
