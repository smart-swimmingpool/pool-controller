// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.hpp
 * @brief Static launcher for the dedicated Core-0 sensor task.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Creates and tracks the dedicated sensor task pinned to Core 0.
 *
 * Mutable application state, MQTT serialization and OLED rendering remain on
 * the Arduino control-loop task on Core 1. Sensor I/O is the only production
 * workload moved to Core 0 and publishes readings through SensorSlots.
 */
class CoreScheduler {
public:
  static constexpr uint8_t TASK_PRIORITY_SENSOR = 2;
  static constexpr uint16_t TASK_STACK_SENSOR = 6 * 1024;

  /** @brief Create the Core-0 sensor task. Call once after initializeController(). */
  static void begin();

  /** @brief Periodically log the SensorTask stack high-water mark. */
  static void logStackWatermarks();
};

}  // namespace PoolController
