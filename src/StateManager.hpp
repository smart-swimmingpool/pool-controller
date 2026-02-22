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
static const int      EEPROM_SLOT_COUNT = 16;  // Number of 32-byte slots
static const int      EEPROM_SLOT_SIZE  = 32;  // Bytes per slot
static bool           eepromInitialized = false;

// Hash function using prime modulo for better distribution
static uint16_t hashKey(const char* key) {
  uint32_t hash = 5381;  // DJB2 hash initial value
  while (*key) {
    hash = ((hash << 5) + hash) + static_cast<uint8_t>(*key);  // hash * 33 + c
    key++;
  }
  // Use prime number 17 for better distribution
  return EEPROM_DATA_START + ((hash % 17) % EEPROM_SLOT_COUNT) * EEPROM_SLOT_SIZE;
}

// Lazy initialization - ensures EEPROM is ready before first use
static void ensureInitialized() {
  if (!eepromInitialized) {
    EEPROM.begin(512);
    // Check magic number
    uint32_t magic = 0;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);
    if (magic != EEPROM_MAGIC) {
      // First time or corrupted - initialize entire EEPROM
      EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
      // Clear data region to avoid garbage
      for (int i = EEPROM_DATA_START; i < 512; i++) {
        EEPROM.write(i, 0xFF);  // 0xFF indicates unused/invalid
      }
      EEPROM.commit();
    }
    eepromInitialized = true;
  }
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
    ensureInitialized();  // Use lazy init for safety
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
    ensureInitialized();  // Lazy init
    uint16_t addr = hashKey(key);
    // Store length (1 byte) + string data (max 30 bytes)
    uint8_t len = min(value.length(), 30U);
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
    ensureInitialized();  // Lazy init
    uint16_t addr = hashKey(key);
    uint8_t  len  = EEPROM.read(addr);
    if (len == 0 || len == 0xFF || len > 30) {
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
    ensureInitialized();  // Lazy init
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
    ensureInitialized();  // Lazy init
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
    ensureInitialized();  // Lazy init
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
    ensureInitialized();  // Lazy init
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
    ensureInitialized();  // Lazy init
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
    ensureInitialized();  // Lazy init
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
    ensureInitialized();  // Lazy init
    // Clear EEPROM
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
#endif
  }
};

}  // namespace PoolController
