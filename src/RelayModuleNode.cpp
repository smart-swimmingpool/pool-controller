/**
 * Homie Node for Relay Module.
 *
 * Used lib:
 * https://github.com/YuriiSalimov/RelayModule
 */
#include "RelayModuleNode.hpp"
#include <Preferences.h>

RelayModuleNode::RelayModuleNode(const char* id, const char* name, const uint8_t pin, const int measurementInterval)
    : HomieNode(id, name, "switch") {
  _pin                 = pin;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = 0;
}

/**
 *
 */
void RelayModuleNode::setSwitch(const boolean state) {

  if (state) {
    relay->on();
  } else {
    relay->off();
  }

  if(Homie.isConnected()){
    setProperty(cSwitch).send((state ? cFlagOn : cFlagOff));
    setProperty(cHomieNodeState).send(cHomieNodeState_OK);
  }
  // persist value
  {
    Preferences prefs;
    prefs.begin(getId(), false);
    prefs.putBool(cSwitch, state);
    prefs.end();
  }

  Homie.getLogger() << cIndent << F("Relay is ") << (state ? cFlagOn : cFlagOff) << endl;
}

/**
 *
 */
boolean RelayModuleNode::getSwitch() {
  return relay->isOn();
}

/**
 *
 */
void RelayModuleNode::printCaption() {
  Homie.getLogger() << cCaption << F(" pin[") << _pin << F("]:") << endl;
}

/**
 * Handles the received MQTT messages from Homie.
 *
 */
bool RelayModuleNode::handleInput(const HomieRange& range, const String& property, const String& value) {
  printCaption();

  Homie.getLogger() << cIndent << F("〽 handleInput -> property '") << property << F("' value=") << value << endl;
  bool retval;

  if (value != cFlagOn && value != cFlagOff) {
    Homie.getLogger() << F("invalid value for property '") << property << F("' value=") << value << endl;

    if(Homie.isConnected()) {
      setProperty(cHomieNodeState).send(cHomieNodeState_Error);
    }
    retval = false;
  } else {
    const bool flag = (value == cFlagOn);
    setSwitch(flag);

    retval = true;
  }

  Homie.getLogger() << F("〽 handleInput <-") << retval << endl;
  return retval;
}

/**
 *
 */
void RelayModuleNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {

    if (Homie.isConnected()) {

      const boolean isOn = getSwitch();
      Homie.getLogger() << F("〽 Sending Switch status: ") << getId() << F("switch: ") << (isOn ? cFlagOn : cFlagOff) << endl;

      if(Homie.isConnected()) {
        setProperty(cSwitch).send((isOn ? cFlagOn : cFlagOff));
      }
    }

    _lastMeasurement = millis();
  }
}

/**
 *
 */
void RelayModuleNode::setup() {
  printCaption();

  advertise(cSwitch).setName(cSwitchName).setDatatype("boolean").settable();
  advertise(cHomieNodeState).setName(cHomieNodeStateName).setDatatype("string");

  relay = new RelayModule(_pin);

  {
    Preferences prefs;
    prefs.begin(getId(), false);
    boolean storedSwitchValue = prefs.getBool(cSwitch, false);
    // Close the Preferences
    prefs.end();

  //restore from preferences
  if (storedSwitchValue) {
    relay->on();
  } else {
    relay->off();
  }
}
