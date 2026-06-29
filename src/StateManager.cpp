// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file StateManager.cpp
 * @brief Controller state persistence — save/restore operation mode and
 *        temperature thresholds via ESP32 Preferences (NVS).
 */

#include "StateManager.hpp"

namespace PoolController {

void StateManager::begin() {
  // ESP32: Preferences is initialized per-operation
}

bool StateManager::saveString(const char *key, const String &value) {
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putString(key, value);
  prefs.end();
  return result;
}

String StateManager::loadString(const char *key, const String &defaultValue) {
  Preferences prefs;
  prefs.begin("pool-controller", true);
  String value = prefs.getString(key, defaultValue);
  prefs.end();
  return value;
}

bool StateManager::saveFloat(const char *key, float value) {
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putFloat(key, value);
  prefs.end();
  return result;
}

float StateManager::loadFloat(const char *key, float defaultValue) {
  Preferences prefs;
  prefs.begin("pool-controller", true);
  float value = prefs.getFloat(key, defaultValue);
  prefs.end();
  if (isnan(value) || value < -1000.0f || value > 1000.0f) {
    return defaultValue;
  }
  return value;
}

bool StateManager::saveInt(const char *key, int value) {
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putInt(key, value);
  prefs.end();
  return result;
}

int StateManager::loadInt(const char *key, int defaultValue) {
  Preferences prefs;
  prefs.begin("pool-controller", true);
  int value = prefs.getInt(key, defaultValue);
  prefs.end();
  if (value < -10000 || value > 10000) {
    return defaultValue;
  }
  return value;
}

bool StateManager::saveBool(const char *key, bool value) {
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putBool(key, value);
  prefs.end();
  return result;
}

bool StateManager::loadBool(const char *key, bool defaultValue) {
  Preferences prefs;
  prefs.begin("pool-controller", true);
  bool value = prefs.getBool(key, defaultValue);
  prefs.end();
  return value;
}

void StateManager::clear() {
  Preferences prefs;
  prefs.begin("pool-controller", false);
  prefs.clear();
  prefs.end();
}

}  // namespace PoolController
