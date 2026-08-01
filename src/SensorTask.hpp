// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorTask.hpp
 * @brief Core-0 task owning all DS18B20/OneWire and ESP32 internal temp reads.
 */

#pragma once

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace PoolController {

/**
 * @brief Runs the temperature measurement cycle exclusively on Core 0.
 *
 * Owns all Dallas/OneWire bus access (OneWire is not thread-safe — the
 * control loop never touches the buses anymore). Per period: begin
 * conversion, yield via vTaskDelay, read results, publish to SensorSlots.
 */
class SensorTask {
public:
  /** @brief Create and start the task pinned to the given core. */
  static void start(uint8_t priority, uint16_t stackBytes, BaseType_t core);

  /** @brief Log the task's stack high-water mark. */
  static void logStackWatermark();
};

}  // namespace PoolController
