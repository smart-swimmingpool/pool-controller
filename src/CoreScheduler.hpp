// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.hpp
 * @brief Static launcher for the Core-0 I/O tasks.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Creates and tracks the dedicated I/O tasks pinned to Core 0.
 *
 * All tasks are created once in begin() with fixed stacks and priorities
 * (no dynamic task creation after setup). Priorities only matter within a
 * core: the I/O tasks sit below the WiFi-stack tasks and yield via
 * vTaskDelay at their scheduling period.
 */
class CoreScheduler {
public:
  static constexpr uint8_t TASK_PRIORITY_SENSOR = 2;
  static constexpr uint8_t TASK_PRIORITY_PUBLISH = 1;
  static constexpr uint8_t TASK_PRIORITY_DISPLAY = 1;
  static constexpr uint16_t TASK_STACK_SENSOR = 6 * 1024;
  static constexpr uint16_t TASK_STACK_PUBLISH = 4 * 1024;
  static constexpr uint16_t TASK_STACK_DISPLAY = 3 * 1024;

  /** @brief Create all Core-0 I/O tasks. Call once from setup(), after initializeController(). */
  static void begin();

  /** @brief Log stack high-water marks of all tasks (call periodically from loop()). */
  static void logStackWatermarks();
};

}  // namespace PoolController
