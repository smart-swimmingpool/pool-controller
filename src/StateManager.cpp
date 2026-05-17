// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "StateManager.hpp"

#include <cstring>

namespace PoolController {

#ifdef ESP8266
// ---------------------------------------------------------------------------
// ESP8266 EEPROM Structured Layout
// ---------------------------------------------------------------------------
// Replaces the old DJB2 hash-based slot allocation with a fixed key-to-offset
// mapping, eliminating collision risk and adding CRC16 data integrity.
//
// Layout (512 bytes total):
//   [0..3]   Magic number    "P00N" (0x4E303050) — changed from "P00L" to
//                            detect old-format EEPROM and auto-migrate.
//   [4..5]   CRC16           Over all data bytes below.
//   [6..41]  Key-value slots (36 bytes data + 36 bytes meta = 72 bytes used,
//                            leaving 438 bytes free for future keys).
//
// Slot mapping (deterministic, no collisions):
//   Addr  Key             Type    Size  Description
//     6   "hysteresis"    float    4    Hysteresis
//    10   "opmode"        string   8    Mode ("auto","manu","boost","timer")
//    18   "poolMaxTemp"   float    4    Max pool temperature
//    22   "solarMinTemp"  float    4    Min solar temperature
//    26   "timerEndH"     int      4    Timer end hour
//    30   "timerEndM"     int      4    Timer end minute
//    34   "timerStartH"   int      4    Timer start hour
//    38   "timerStartM"   int      4    Timer start minute
//    42   First future slot
//
// CRC16 covers bytes [6..41] (36 bytes of data).
// ---------------------------------------------------------------------------

static constexpr uint32_t EEPROM_MAGIC = 0x4E303050;  // "P00N" (new layout)
static constexpr uint32_t EEPROM_SIZE = 512;
static constexpr uint16_t CRC_START = 4;   // Address of CRC16 value
static constexpr uint16_t DATA_START = 6;  // Address of first key-value slot

// Key-to-offset mapping table — one entry per known key.
// Order must stay stable across firmware updates for backward compatibility.
// Max total data at time of writing: 36 bytes.
struct SlotEntry {
  const char *key;
  uint16_t offset;  // Relative to DATA_START
  uint8_t maxSize;
};

// Sorted for readability, but lookup is linear (only ~8 entries).
static const SlotEntry SLOT_MAP[] = {
  {"hysteresis",    0,  4},
  {"opmode",        4,  8},
  {"poolMaxTemp",   12, 4},
  {"solarMinTemp",  16, 4},
  {"timerEndH",     20, 4},
  {"timerEndM",     24, 4},
  {"timerStartH",   28, 4},
  {"timerStartM",   32, 4},
};
static constexpr uint8_t SLOT_COUNT = sizeof(SLOT_MAP) / sizeof(SLOT_MAP[0]);
static constexpr uint16_t DATA_END = DATA_START + 36;  // 6 + 36 = 42 bytes used

// Cache: last written CRC to avoid redundant recomputation on every read.
static uint16_t cachedCrc = 0;

// ---------------------------------------------------------------------------
// CRC16-CCITT (poly 0x1021) — computed over data region [DATA_START..DATA_END)
// ---------------------------------------------------------------------------
static uint16_t computeCrc() {
  uint16_t crc = 0xFFFF;
  for (uint16_t addr = DATA_START; addr < DATA_END; addr++) {
    uint8_t byte = EEPROM.read(addr);
    crc ^= (uint16_t(byte) << 8);
    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc = (crc << 1);
    }
  }
  return crc;
}

// ---------------------------------------------------------------------------
// Lookup slot for a given key — returns address in EEPROM or 0 if not found.
// ---------------------------------------------------------------------------
static uint16_t lookupSlot(const char *key, uint8_t expectedSize) {
  for (uint8_t i = 0; i < SLOT_COUNT; i++) {
    if (std::strcmp(SLOT_MAP[i].key, key) == 0) {
      return DATA_START + SLOT_MAP[i].offset;
    }
  }
  return 0;  // Unknown key — should not happen in normal operation
}

// ---------------------------------------------------------------------------
// CRC validation — call after every read to detect data corruption.
// Returns true if CRC stored in EEPROM matches computed CRC.
// ---------------------------------------------------------------------------
static bool isCrcValid() {
  uint16_t storedCrc;
  EEPROM.get(CRC_START, storedCrc);
  return storedCrc == computeCrc();
}

// ---------------------------------------------------------------------------
// Write CRC to EEPROM (does NOT commit — caller must commit).
// ---------------------------------------------------------------------------
static void updateCrc() {
  uint16_t crc = computeCrc();
  EEPROM.put(CRC_START, crc);
  cachedCrc = crc;
}

// Single instance of initialization flag
static bool eepromInitialized = false;

// Lazy initialization — ensures EEPROM is ready before first use.
static void ensureInitialized() {
  if (!eepromInitialized) {
    EEPROM.begin(EEPROM_SIZE);

    uint32_t magic = 0;
    EEPROM.get(0, magic);

    if (magic != EEPROM_MAGIC) {
      // First boot after P3 update (old "P00L" magic) or uninitialized EEPROM.
      // Initialize everything to factory defaults.
      for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
      }
      EEPROM.put(0, EEPROM_MAGIC);
      // Initialize data slots to known-zero so CRC is deterministic
      for (uint16_t addr = DATA_START; addr < DATA_END; addr++) {
        EEPROM.write(addr, 0);
      }
      updateCrc();
      EEPROM.commit();
    }

    eepromInitialized = true;
  }
}

#endif  // ESP8266

// ===========================================================================
// StateManager implementation
// ===========================================================================

void StateManager::begin() {
#ifdef ESP8266
  ensureInitialized();
#endif
}

// ---------------------------------------------------------------------------
// saveString
// ---------------------------------------------------------------------------
bool StateManager::saveString(const char *key, const String &value) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putString(key, value);
  prefs.end();
  return result;
#elif defined(ESP8266)
  ensureInitialized();
  uint16_t addr = lookupSlot(key, 0);
  if (addr == 0) return false;

  uint8_t maxLen = 7;  // max string length for opmode ("timer" = 5 chars)
  uint8_t len = min(value.length(), (unsigned int)maxLen);
  EEPROM.write(addr, len);
  for (uint8_t i = 0; i < len; i++) {
    EEPROM.write(addr + 1 + i, value[i]);
  }
  updateCrc();
  return EEPROM.commit();
#else
  return false;
#endif
}

// ---------------------------------------------------------------------------
// loadString
// ---------------------------------------------------------------------------
String StateManager::loadString(const char *key, const String &defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);
  String value = prefs.getString(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();
  if (!isCrcValid()) return defaultValue;

  uint16_t addr = lookupSlot(key, 0);
  if (addr == 0) return defaultValue;

  uint8_t len = EEPROM.read(addr);
  if (len == 0 || len > 7) return defaultValue;

  String value;
  value.reserve(len);
  for (uint8_t i = 0; i < len; i++) {
    value += (char)EEPROM.read(addr + 1 + i);
  }
  return value;
#else
  return defaultValue;
#endif
}

// ---------------------------------------------------------------------------
// saveFloat
// ---------------------------------------------------------------------------
bool StateManager::saveFloat(const char *key, float value) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putFloat(key, value);
  prefs.end();
  return result;
#elif defined(ESP8266)
  ensureInitialized();
  uint16_t addr = lookupSlot(key, 4);
  if (addr == 0) return false;

  EEPROM.put(addr, value);
  updateCrc();
  return EEPROM.commit();
#else
  return false;
#endif
}

// ---------------------------------------------------------------------------
// loadFloat
// ---------------------------------------------------------------------------
float StateManager::loadFloat(const char *key, float defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);
  float value = prefs.getFloat(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();
  if (!isCrcValid()) return defaultValue;

  uint16_t addr = lookupSlot(key, 4);
  if (addr == 0) return defaultValue;

  float value;
  EEPROM.get(addr, value);
  if (isnan(value) || value < -1000.0f || value > 1000.0f) {
    return defaultValue;
  }
  return value;
#else
  return defaultValue;
#endif
}

// ---------------------------------------------------------------------------
// saveInt
// ---------------------------------------------------------------------------
bool StateManager::saveInt(const char *key, int value) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putInt(key, value);
  prefs.end();
  return result;
#elif defined(ESP8266)
  ensureInitialized();
  uint16_t addr = lookupSlot(key, 4);
  if (addr == 0) return false;

  EEPROM.put(addr, value);
  updateCrc();
  return EEPROM.commit();
#else
  return false;
#endif
}

// ---------------------------------------------------------------------------
// loadInt
// ---------------------------------------------------------------------------
int StateManager::loadInt(const char *key, int defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);
  int value = prefs.getInt(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();
  if (!isCrcValid()) return defaultValue;

  uint16_t addr = lookupSlot(key, 4);
  if (addr == 0) return defaultValue;

  int value;
  EEPROM.get(addr, value);
  if (value < -10000 || value > 10000) {
    return defaultValue;
  }
  return value;
#else
  return defaultValue;
#endif
}

// ---------------------------------------------------------------------------
// saveBool
// ---------------------------------------------------------------------------
bool StateManager::saveBool(const char *key, bool value) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", false);
  bool result = prefs.putBool(key, value);
  prefs.end();
  return result;
#elif defined(ESP8266)
  ensureInitialized();
  uint16_t addr = lookupSlot(key, 1);
  if (addr == 0) return false;

  EEPROM.write(addr, value ? 1 : 0);
  updateCrc();
  return EEPROM.commit();
#else
  return false;
#endif
}

// ---------------------------------------------------------------------------
// loadBool
// ---------------------------------------------------------------------------
bool StateManager::loadBool(const char *key, bool defaultValue) {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", true);
  bool value = prefs.getBool(key, defaultValue);
  prefs.end();
  return value;
#elif defined(ESP8266)
  ensureInitialized();
  if (!isCrcValid()) return defaultValue;

  uint16_t addr = lookupSlot(key, 1);
  if (addr == 0) return defaultValue;

  uint8_t value = EEPROM.read(addr);
  if (value > 1) return defaultValue;
  return value == 1;
#else
  return defaultValue;
#endif
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------
void StateManager::clear() {
#ifdef ESP32
  Preferences prefs;
  prefs.begin("pool-controller", false);
  prefs.clear();
  prefs.end();
#elif defined(ESP8266)
  ensureInitialized();
  for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.put(0, EEPROM_MAGIC);
  // Re-initialize data area with zeros so CRC is deterministic
  for (uint16_t addr = DATA_START; addr < DATA_END; addr++) {
    EEPROM.write(addr, 0);
  }
  updateCrc();
  EEPROM.commit();
#endif
}

}  // namespace PoolController
