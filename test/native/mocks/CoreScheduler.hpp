// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Native test double for CoreScheduler.
 * Captures begin() parameters so tests can assert the planned values.
 */
class CoreScheduler {
public:
  static constexpr uint8_t TASK_PRIORITY_SENSOR = 2;
  static constexpr uint8_t TASK_PRIORITY_PUBLISH = 1;
  static constexpr uint8_t TASK_PRIORITY_DISPLAY = 1;
  static constexpr uint16_t TASK_STACK_SENSOR = 6 * 1024;
  static constexpr uint16_t TASK_STACK_PUBLISH = 4 * 1024;
  static constexpr uint16_t TASK_STACK_DISPLAY = 3 * 1024;

  static void begin();
  static void logStackWatermarks();

  static uint8_t sensorPriority;
  static uint16_t sensorStack;
};

}  // namespace PoolController
