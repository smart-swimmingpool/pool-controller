// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * Homie Node for Relay Module.
 *
 * Used lib:
 * https://github.com/YuriiSalimov/RelayModule
 *
 * ESP8266 support was removed in v3.2.0.
 */
#include "RelayModuleNode.hpp"
#include "Utils.hpp"
#include "MqttInterface.hpp"
#include "DegradationManager.hpp"

RelayModuleNode::RelayModuleNode(const char *id, const char *name, const uint8_t pin, const int measurementInterval)
    : HomieNode(id, name, "switch") {
  _pin = pin;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = 0;

  setRunLoopDisconnected(true);
}

/**
 *
 */
void RelayModuleNode::setSwitch(const boolean state) {
  // Check if state actually changes to avoid unnecessary NVS writes
  boolean currentState = relay->isOn();
  if (currentState == state) {
    return;
  }

  // P8: In safe mode (boot-loop detected / CRITICAL degradation), never allow
  // a relay to switch ON. Only OFF transitions are permitted so relays default
  // to the safe OFF state.
  if (state && PoolController::DegradationManager::isSafe()) {
    Homie.getLogger() << cIndent << F("SAFE MODE — ignoring relay ON request") << endl;
    return;
  }

  if (state) {
    relay->on();
  } else {
    relay->off();
  }

  if (Homie.isConnected()) {
    PoolController::MqttInterface::publishSwitchState(*this, cSwitch, getId(), state);
    PoolController::MqttInterface::publishHomieProperty(*this, cHomieNodeState, cHomieNodeState_OK);
  }

  // Persist relay state via Preferences (NVS)
  preferences.begin(getId(), false);
  preferences.putBool(cSwitch, state);
  preferences.end();

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
bool RelayModuleNode::handleInput(const HomieRange &range, const String &property, const String &value) {
  printCaption();

  Homie.getLogger() << cIndent << F("〽 handleInput -> property '") << property << F("' value=") << value << endl;
  bool retval;

  if (value != cFlagOn && value != cFlagOff) {
    Homie.getLogger() << F("invalid value for property '") << property << F("' value=") << value << endl;

    if (Homie.isConnected()) {
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
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    if (Homie.isConnected()) {
      const boolean isOn = getSwitch();
      Homie.getLogger() << F("〽 Sending Switch status: ") << getId() << F("switch: ") << (isOn ? cFlagOn : cFlagOff) << endl;

      PoolController::MqttInterface::publishSwitchState(*this, cSwitch, getId(), isOn);
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

  // Load and restore relay state from persistent storage (NVS)
  preferences.begin(getId(), false);
  boolean storedSwitchValue = preferences.getBool(cSwitch, false);
  preferences.end();

  if (storedSwitchValue) {
    relay->on();
  } else {
    relay->off();
  }
}
