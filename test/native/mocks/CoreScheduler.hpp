// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Native test double for CoreScheduler.
 * Captures SensorTask parameters so tests can assert the production values.
 */
class CoreScheduler {
public:
  static constexpr uint8_t TASK_PRIORITY_SENSOR = 2;
  static constexpr uint16_t TASK_STACK_SENSOR = 6 * 1024;

  static void begin();
  static void logStackWatermarks();

  static uint8_t sensorPriority;
  static uint16_t sensorStack;
};

}  // namespace PoolController
