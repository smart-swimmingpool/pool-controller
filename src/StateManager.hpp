// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file StateManager.hpp
 * @brief Controller state persistence via ESP32 Preferences (NVS).
 */

#pragma once

/**
 * State Manager for persisting controller state
 *
 * Handles saving and restoring controller state across reboots and power failures.
 * Uses ESP32 Preferences for persistent storage.
 *
 * ESP8266 support was removed in v3.2.0.
 */

#include <Arduino.h>
#include <Preferences.h>

namespace PoolController {

/**
 * State Manager for persistent storage
 */
class StateManager {
public:
  /** Initialize state manager */
  static void begin();

  /** Save a string value */
  static bool saveString(const char *key, const String &value);

  /** Load a string value */
  static String loadString(const char *key, const String &defaultValue);

  /** Save a float value */
  static bool saveFloat(const char *key, float value);

  /** Load a float value */
  static float loadFloat(const char *key, float defaultValue);

  /** Save an int value */
  static bool saveInt(const char *key, int value);

  /** Load an int value */
  static int loadInt(const char *key, int defaultValue);

  /** Save a boolean value */
  static bool saveBool(const char *key, bool value);

  /** Load a boolean value */
  static bool loadBool(const char *key, bool defaultValue);

  /** Clear all stored values */
  static void clear();
};

}  // namespace PoolController
