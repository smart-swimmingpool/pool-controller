// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file RelayModuleNode.cpp
 * @brief Relay node implementation — state persistence via NVS and safe-mode
 *        override for boot-loop protection.
 */

#include "RelayModuleNode.hpp"
#include "Utils.hpp"
#include "DegradationManager.hpp"

RelayModuleNode::RelayModuleNode(const char *id, const char *name, const uint8_t pin, const bool activeLow, const int measurementInterval) {
  _id = id;
  _name = name;
  _pin = pin;
  _activeLow = activeLow;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = 0;
}

void RelayModuleNode::begin() {
  Serial.printf("• RelayModule Node '%s' initializing on PIN %d...\n", _id, _pin);

  pinMode(_pin, OUTPUT);

  // Load and restore relay state from persistent storage (NVS)
  preferences.begin(_id, false);
  _currentState = preferences.getBool("switch", false);
  preferences.end();

  // Apply polarity: active-LOW  → ON = LOW,  OFF = HIGH
  //                active-HIGH → ON = HIGH, OFF = LOW
  digitalWrite(_pin, _currentState == _activeLow ? LOW : HIGH);

  Serial.printf("  ◦ Relay restored to state: %s\n", _currentState ? "ON" : "OFF");
}

void RelayModuleNode::setSwitch(const bool state) {
  if (_currentState == state) {
    return;
  }

  // P8: In safe mode (boot-loop detected / CRITICAL degradation), never allow
  // a relay to switch ON. Only OFF transitions are permitted so relays default
  // to the safe OFF state.
  if (state && PoolController::DegradationManager::isSafe()) {
    Serial.println("  ⚠ SAFE MODE — ignoring relay ON request");
    return;
  }

  // Apply polarity: active-LOW  → ON = LOW,  OFF = HIGH
  //                active-HIGH → ON = HIGH, OFF = LOW
  digitalWrite(_pin, state == _activeLow ? LOW : HIGH);
  _currentState = state;

  // Persist relay state via Preferences (NVS)
  preferences.begin(_id, false);
  preferences.putBool("switch", state);
  preferences.end();

  Serial.printf("  ◦ Relay '%s' switched to: %s\n", _id, state ? "ON" : "OFF");
}

bool RelayModuleNode::getSwitch() {
  return _currentState;
}

void RelayModuleNode::loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();
    Serial.printf("〽 Relay '%s' status: %s\n", _id, getSwitch() ? "ON" : "OFF");
  }
}
