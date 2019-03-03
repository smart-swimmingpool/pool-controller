/**
 * Homie Node for Relay Module.
 *
 * Used lib:
 * https://github.com/YuriiSalimov/RelayModule
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
void RelayModuleNode::setSwitch(const boolean state) {

  if (state) {
    relay->on();
  } else {
    relay->off();
  }

  setProperty(cSwitch).send((state ? cFlagOn : cFlagOff));

  // persist value
#ifdef ESP32
  preferences.begin(getId(), false);
  preferences.putBool(cSwitch, state);
  preferences.end();
#elif defined(ESP8266)

#endif

  setProperty(cStatus).send("ok");

  Homie.getLogger() << cIndent << "Relay is " << (state ? cFlagOn : cFlagOff) << endl;
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
  Homie.getLogger() << cCaption << endl;
}

/**
 * Handles the received MQTT messages from Homie.
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
    setSwitch(flag);

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

    const boolean isOn = getSwitch();
    Homie.getLogger() << cIndent << "switch: " << (isOn ? cFlagOn : cFlagOff) << endl;

    setProperty(cSwitch).send((isOn ? cFlagOn : cFlagOff));
  }
}

/**
 *
 */
void RelayModuleNode::onReadyToOperate() {
  advertise(cSwitch).setName("Switch").setRetained(true).setDatatype("boolean");//.settable();
  advertise(cStatus).setName("Satus").setDatatype("string");
}

/**
 *
 */
void RelayModuleNode::setup() {
  printCaption();

  relay = new RelayModule(_pin);

#ifdef ESP32
  preferences.begin(getId(), false);
  boolean storedSwitchValue = preferences.getBool(cSwitch, false);
  // Close the Preferences
  preferences.end();
#elif defined(ESP8266)
  boolean storedSwitchValue = false;
#endif

  //restore from preferences
  if (storedSwitchValue) {
    relay->on();
  } else {
    relay->off();
  }
}
