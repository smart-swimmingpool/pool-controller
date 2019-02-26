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
  } else {
    relay->off();
  }

  setProperty(cSwitch).send((state ? cFlagOn : cFlagOff));

  // persist value
  preferences.begin(getId(), false);
  preferences.putBool(cSwitch, state);
  preferences.end();

  setProperty(cStatus).send("ok");

  Homie.getLogger() << cIndent << "Relay is " << (state ? cFlagOn : cFlagOff) << endl;
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
bool RelayModuleNode::handleInput(const HomieRange& range, const String& property, const String& value) {

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
void RelayModuleNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    _lastMeasurement = millis();

    Homie.getLogger() << "〽 Sending Switch status: " << getId() << endl;

    const boolean state = getState();
    Homie.getLogger() << cIndent << "switch: " << (state ? cFlagOn : cFlagOff) << endl;

    setProperty(cSwitch).send((state ? cFlagOn : cFlagOff));
  }
}

/**
 *
 */
void RelayModuleNode::onReadyToOperate() {
  advertise(cSwitch).setName("Switch").setRetained(true).setDatatype("boolean").settable();
  advertise(cStatus).setName("Satus").setDatatype("string");
}

/**
 *
 */
void RelayModuleNode::setup() {
  printCaption();

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
