// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file RelayModuleNode.cpp
 * @brief Relay node implementation — state persistence via NVS and safe-mode
 *        override for boot-loop protection.
 */

#include "RelayModuleNode.hpp"
#include "Utils.hpp"
#include "DegradationManager.hpp"

RelayModuleNode::RelayModuleNode(const char *id, const char *name, const uint8_t pin, const int measurementInterval) {
  _id = id;
  _name = name;
  _pin = pin;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = 0;
}

void RelayModuleNode::begin() {
  Serial.printf("• RelayModule Node '%s' initializing on PIN %d...\n", _id, _pin);

  // Use direct new allocation as make_unique is not working
  relay = std::unique_ptr<RelayModule>(new RelayModule(_pin));

  // Load and restore relay state from persistent storage (NVS)
  preferences.begin(_id, false);
  bool storedSwitchValue = preferences.getBool("switch", false);
  preferences.end();

  if (storedSwitchValue) {
    relay->on();
  } else {
    relay->off();
  }
  Serial.printf("  ◦ Relay restored to state: %s\n", storedSwitchValue ? "ON" : "OFF");
}

void RelayModuleNode::setSwitch(const bool state) {
  if (!relay)
    return;

  bool currentState = relay->isOn();
  if (currentState == state) {
    return;
  }

  // P8: In safe mode (boot-loop detected / CRITICAL degradation), never allow
  // a relay to switch ON. Only OFF transitions are permitted so relays default
  // to the safe OFF state.
  if (state && PoolController::DegradationManager::isSafe()) {
    Serial.println("  ⚠ SAFE MODE — ignoring relay ON request");
    return;
  }

  if (state) {
    relay->on();
  } else {
    relay->off();
  }

  // Persist relay state via Preferences (NVS)
  preferences.begin(_id, false);
  preferences.putBool("switch", state);
  preferences.end();

  Serial.printf("  ◦ Relay '%s' switched to: %s\n", _id, state ? "ON" : "OFF");
}

bool RelayModuleNode::getSwitch() {
  if (!relay)
    return false;
  return relay->isOn();
}

void RelayModuleNode::loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();
    Serial.printf("〽 Relay '%s' status: %s\n", _id, getSwitch() ? "ON" : "OFF");
  }
}
