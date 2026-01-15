// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

/**
 * State Manager for persisting controller state
 *
 * Handles saving and restoring controller state across reboots and power failures.
 * Uses ESP32 Preferences on ESP32 and EEPROM emulation on ESP8266.
 */

#include <Arduino.h>

#ifdef ESP32
#include <Preferences.h>
#elif defined(ESP8266)
#include <EEPROM.h>
#endif

namespace PoolController {

/**
 * State Manager for persistent storage
 */
class StateManager {
public:
  /**
     * Initialize state manager
     */
  static void begin() {
#ifdef ESP8266
    EEPROM.begin(512);  // Allocate 512 bytes for EEPROM emulation
#endif
  }

  /**
     * Save a string value
     */
  static bool saveString(const char* key, const String& value) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putString(key, value);
    prefs.end();
    return result;
#elif defined(ESP8266)
    // For ESP8266, use simpler approach - store in fixed location
    // This is a simplified implementation
    return false;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Load a string value
     */
  static String loadString(const char* key, const String& defaultValue) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", true);  // read-only
    String value = prefs.getString(key, defaultValue);
    prefs.end();
    return value;
#elif defined(ESP8266)
    return defaultValue;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Save a float value
     */
  static bool saveFloat(const char* key, float value) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putFloat(key, value);
    prefs.end();
    return result;
#elif defined(ESP8266)
    return false;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Load a float value
     */
  static float loadFloat(const char* key, float defaultValue) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", true);  // read-only
    float value = prefs.getFloat(key, defaultValue);
    prefs.end();
    return value;
#elif defined(ESP8266)
    return defaultValue;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Save an int value
     */
  static bool saveInt(const char* key, int value) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putInt(key, value);
    prefs.end();
    return result;
#elif defined(ESP8266)
    return false;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Load an int value
     */
  static int loadInt(const char* key, int defaultValue) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", true);  // read-only
    int value = prefs.getInt(key, defaultValue);
    prefs.end();
    return value;
#elif defined(ESP8266)
    return defaultValue;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Save a boolean value
     */
  static bool saveBool(const char* key, bool value) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", false);
    bool result = prefs.putBool(key, value);
    prefs.end();
    return result;
#elif defined(ESP8266)
    return false;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Load a boolean value
     */
  static bool loadBool(const char* key, bool defaultValue) {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", true);  // read-only
    bool value = prefs.getBool(key, defaultValue);
    prefs.end();
    return value;
#elif defined(ESP8266)
    return defaultValue;  // Not implemented for ESP8266 yet
#endif
  }

  /**
     * Clear all stored values
     */
  static void clear() {
#ifdef ESP32
    Preferences prefs;
    prefs.begin("pool-controller", false);
    prefs.clear();
    prefs.end();
#elif defined(ESP8266)
    // Clear EEPROM
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
#endif
  }
};

}  // namespace PoolController
