// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "StateManager.hpp"

namespace PoolController {

#ifdef ESP8266
// ESP8266 EEPROM helpers - implementation in .cpp to avoid multiple definitions

// ESP8266 EEPROM Memory Map constants
static const uint32_t EEPROM_MAGIC = 0x50304F4C;  // "P00L"
static const int EEPROM_MAGIC_ADDR = 0;
static const int EEPROM_DATA_START = 4;
static const int EEPROM_SLOT_COUNT = 15;  // Number of 32-byte slots (fits in 512-byte EEPROM)
static const int EEPROM_SLOT_SIZE = 32;   // Bytes per slot

// Single instance of initialization flag
static bool eepromInitialized = false;

// Hash function using DJB2 for distribution
static uint16_t hashKey(const char *key) {
  uint32_t hash = 5381;  // DJB2 hash initial value
  while (*key) {
    hash = ((hash << 5) + hash) + static_cast<uint8_t>(*key);  // hash * 33 + c
    key++;
  }
  // Map hash uniformly into available EEPROM slots
  return EEPROM_DATA_START + (hash % EEPROM_SLOT_COUNT) * EEPROM_SLOT_SIZE;
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

// StateManager implementation

void StateManager::begin() {
#ifdef ESP8266
  ensureInitialized();  // Use lazy init for safety
#endif
}

bool StateManager::saveString(const char *key, const String &value) {
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
#else
  return false;
#endif
}

String StateManager::loadString(const char *key, const String &defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);  // read-only
  String value = prefs.getString(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();  // Lazy init
  uint16_t addr = hashKey(key);
  uint8_t len = EEPROM.read(addr);
  if (len == 0 || len == 0xFF || len > 30) {
    return defaultValue;
  }
  String value = "";
  value.reserve(len);
  for (uint8_t i = 0; i < len; i++) {
    value += (char)EEPROM.read(addr + 1 + i);
  }
  return value;
#else
  return defaultValue;
#endif
}

bool StateManager::saveFloat(const char *key, float value) {
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
#else
  return false;
#endif
}

float StateManager::loadFloat(const char *key, float defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);  // read-only
  float value = prefs.getFloat(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();  // Lazy init
  uint16_t addr = hashKey(key);
  float value;
  EEPROM.get(addr, value);
  // Validate: if NaN or unreasonable, use default
  if (isnan(value) || value < -1000.0 || value > 1000.0) {
    return defaultValue;
  }
  return value;
#else
  return defaultValue;
#endif
}

bool StateManager::saveInt(const char *key, int value) {
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
#else
  return false;
#endif
}

int StateManager::loadInt(const char *key, int defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);  // read-only
  int value = prefs.getInt(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();  // Lazy init
  uint16_t addr = hashKey(key);
  int value;
  EEPROM.get(addr, value);
  // Validate: if unreasonable, use default
  if (value < -10000 || value > 10000) {
    return defaultValue;
  }
  return value;
#else
  return defaultValue;
#endif
}

bool StateManager::saveBool(const char *key, bool value) {
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
#else
  return false;
#endif
}

bool StateManager::loadBool(const char *key, bool defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);  // read-only
  bool value = prefs.getBool(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();  // Lazy init
  uint16_t addr = hashKey(key);
  uint8_t value = EEPROM.read(addr);
  if (value > 1) {
    return defaultValue;  // Uninitialized
  }
  return value == 1;
#else
  return defaultValue;
#endif
}

void StateManager::clear() {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", false);
  prefs.clear();
  prefs.end();
#elif defined(ESP8266)
  ensureInitialized();  // Lazy init
  // Clear EEPROM to uninitialized state (0xFF)
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0xFF);
  }
  // Restore magic number so future operations know EEPROM is initialized
  EEPROM.put(0, EEPROM_MAGIC);
  EEPROM.commit();
#endif
}

}  // namespace PoolController
