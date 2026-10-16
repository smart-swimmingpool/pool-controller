// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorSlots.hpp
 * @brief Lock-free temperature slots shared between SensorTask and readers.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/** @brief Identifies a temperature sensor slot. */
enum class SensorId : uint8_t {
  SOLAR = 0,       ///< Solar DS18B20
  POOL = 1,        ///< Pool DS18B20
  CONTROLLER = 2,  ///< ESP32 internal temperature
  COUNT = 3        ///< Sentinel
};

/**
 * @brief Fixed slots for sensor values shared across tasks.
 *
 * Single writer (SensorTask on Core 0), multiple readers (control loop,
 * display). Value and found flag are written and read together under a
 * short critical section (portMUX on ESP32, std::mutex in native tests), so
 * readers always see a consistent snapshot of one measurement. `volatile`
 * would give neither inter-core ordering nor a consistent pair.
 */
class SensorSlots {
public:
  /** @brief A consistent value/found pair of one measurement. */
  struct Reading {
    float value;  ///< °C, NAN if unknown
    bool found;   ///< Sensor present and reading valid
  };

  /** @brief Reset all slots to NaN / not-found (tests only). */
  static void reset();

  /** @brief Writer: publish a new value. */
  static void write(SensorId id, float value, bool found);

  /** @brief Reader: value and found flag of the same measurement. */
  static Reading snapshot(SensorId id);

  /** @brief Reader: get the latest value (°C, NAN if unknown). */
  static float read(SensorId id) { return snapshot(id).value; }

  /** @brief Reader: check whether the sensor is currently found. */
  static bool isFound(SensorId id) { return snapshot(id).found; }

private:
  static Reading slots_[static_cast<uint8_t>(SensorId::COUNT)];
};

}  // namespace PoolController
