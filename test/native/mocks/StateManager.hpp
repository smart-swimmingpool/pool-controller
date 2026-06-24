#pragma once
#include "Arduino.h"
#include <Preferences.h>

namespace PoolController {

class StateManager {
public:
  static void begin() {}

  static bool saveString(const char *key, const String &value) {
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putString(key, value);
    prefs.end();
    return result;
  }

  static String loadString(const char *key, const String &defaultValue) {
    Preferences prefs;
    prefs.begin("pool-controller", true);
    String value = prefs.getString(key, defaultValue);
    prefs.end();
    return value;
  }

  static bool saveFloat(const char *key, float value) {
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putFloat(key, value);
    prefs.end();
    return result;
  }

  static float loadFloat(const char *key, float defaultValue) {
    Preferences prefs;
    prefs.begin("pool-controller", true);
    float value = prefs.getFloat(key, defaultValue);
    prefs.end();
    if (isnan(value) || value < -1000.0f || value > 1000.0f) return defaultValue;
    return value;
  }

  static bool saveInt(const char *key, int value) {
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putInt(key, value);
    prefs.end();
    return result;
  }

  static int loadInt(const char *key, int defaultValue) {
    Preferences prefs;
    prefs.begin("pool-controller", true);
    int value = prefs.getInt(key, defaultValue);
    prefs.end();
    if (value < -10000 || value > 10000) return defaultValue;
    return value;
  }

  static bool saveBool(const char *key, bool value) {
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putBool(key, value);
    prefs.end();
    return result;
  }

  static bool loadBool(const char *key, bool defaultValue) {
    Preferences prefs;
    prefs.begin("pool-controller", true);
    bool value = prefs.getBool(key, defaultValue);
    prefs.end();
    return value;
  }

  static void clear() {
    Preferences prefs;
    prefs.begin("pool-controller", false);
    prefs.clear();
    prefs.end();
  }
};

} // namespace PoolController
