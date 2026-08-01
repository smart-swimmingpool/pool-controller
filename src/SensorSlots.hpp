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
 * @brief Fixed, lock-free slots for sensor values.
 *
 * Single writer (SensorTask on Core 0), multiple readers (control loop,
 * display). Uses `volatile` word-sized fields: on ESP32 aligned 32-bit
 * reads/writes are atomic, so readers may see one-cycle-stale but never
 * torn values — acceptable for temperature telemetry.
 */
class SensorSlots {
public:
  /** @brief Reset all slots to NaN / not-found (tests only). */
  static void reset();

  /** @brief Writer: publish a new value. */
  static void write(SensorId id, float value, bool found);

  /** @brief Reader: get the latest value (°C, NAN if unknown). */
  static float read(SensorId id);

  /** @brief Reader: check whether the sensor is currently found. */
  static bool isFound(SensorId id);

private:
  struct Slot {
    volatile float value;
    volatile bool found;
  };
  static Slot slots_[static_cast<uint8_t>(SensorId::COUNT)];
};

}  // namespace PoolController
