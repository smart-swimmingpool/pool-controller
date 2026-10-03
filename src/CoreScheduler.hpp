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
 * Stateful MQTT serialization and OLED rendering intentionally stay on the
 * Arduino control-loop task on Core 1. They traverse mutable controller state
 * and therefore must not run concurrently with the single-writer control loop.
 * Sensor I/O is isolated on Core 0 and publishes readings through SensorSlots.
 */
class CoreScheduler {
public:
  static constexpr uint8_t TASK_PRIORITY_SENSOR = 2;
  static constexpr uint8_t TASK_PRIORITY_PUBLISH = 1;
  static constexpr uint8_t TASK_PRIORITY_DISPLAY = 1;
  static constexpr uint16_t TASK_STACK_SENSOR = 6 * 1024;
  static constexpr uint16_t TASK_STACK_PUBLISH = 4 * 1024;
  static constexpr uint16_t TASK_STACK_DISPLAY = 3 * 1024;

  /** @brief Create the Core-0 sensor task. Call once after initializeController(). */
  static void begin();

  /**
   * @brief Service queued telemetry on Core 1 and periodically log worker stack watermarks.
   *
   * PoolController::loop() calls this once per iteration, so MQTT publishing
   * remains serialized with all other mutable control state.
   */
  static void logStackWatermarks();
};

}  // namespace PoolController
