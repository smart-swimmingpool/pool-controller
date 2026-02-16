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

// ESP8266 EEPROM Memory Map
// Address 0-3: Magic number for validation (0xP00L)
// Address 4-511: Key-value storage
#ifdef ESP8266
static const uint32_t EEPROM_MAGIC      = 0x50304F4C;  // "P00L"
static const int      EEPROM_MAGIC_ADDR = 0;
static const int      EEPROM_DATA_START = 4;

// Simple hash function for key to EEPROM address mapping
static uint16_t hashKey(const char* key) {
  uint16_t hash = 0;
  while (*key) {
    hash = hash * 31 + *key;
    key++;
  }
  // Map to EEPROM space (4-511), reserve 32 bytes per key
  return EEPROM_DATA_START + ((hash % 15) * 32);
}
#endif

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
    // Check magic number, initialize if needed
    uint32_t magic = 0;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);
    if (magic != EEPROM_MAGIC) {
      // First time or corrupted - initialize
      EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
      EEPROM.commit();
    }
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
    uint16_t addr = hashKey(key);
    // Store length (1 byte) + string data (max 30 bytes)
    uint8_t len = min(value.length(), 30);
    EEPROM.write(addr, len);
    for (uint8_t i = 0; i < len; i++) {
      EEPROM.write(addr + 1 + i, value[i]);
    }
    return EEPROM.commit();
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
    uint16_t addr = hashKey(key);
    uint8_t  len  = EEPROM.read(addr);
    if (len == 0 || len > 30) {
      return defaultValue;
    }
    String value = "";
    value.reserve(len);
    for (uint8_t i = 0; i < len; i++) {
      value += (char)EEPROM.read(addr + 1 + i);
    }
    return value;
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
    uint16_t addr = hashKey(key);
    EEPROM.put(addr, value);
    return EEPROM.commit();
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
    uint16_t addr = hashKey(key);
    float    value;
    EEPROM.get(addr, value);
    // Validate: if NaN or unreasonable, use default
    if (isnan(value) || value < -1000.0 || value > 1000.0) {
      return defaultValue;
    }
    return value;
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
    uint16_t addr = hashKey(key);
    EEPROM.put(addr, value);
    return EEPROM.commit();
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
    uint16_t addr = hashKey(key);
    int      value;
    EEPROM.get(addr, value);
    // Validate: if unreasonable, use default
    if (value < -10000 || value > 10000) {
      return defaultValue;
    }
    return value;
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
    uint16_t addr = hashKey(key);
    EEPROM.write(addr, value ? 1 : 0);
    return EEPROM.commit();
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
    uint16_t addr  = hashKey(key);
    uint8_t  value = EEPROM.read(addr);
    if (value > 1) {
      return defaultValue;  // Uninitialized
    }
    return value == 1;
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
