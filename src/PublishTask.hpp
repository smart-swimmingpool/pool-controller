// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file PublishTask.hpp
 * @brief Core-0 task that serializes and publishes MQTT telemetry.
 */

#pragma once

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace PoolController {

/**
 * @brief Drains the telemetry queue and performs MQTT serialization on Core 0.
 *
 * The control loop only enqueues publish requests (non-blocking); the heavy
 * JSON/HA-discovery serialization and the AsyncMqttClient::publish() calls
 * run here. AsyncMqttClient::publish() is non-blocking from the library side.
 */
class PublishTask {
public:
  /** @brief Create and start the task pinned to the given core. */
  static void start(uint8_t priority, uint16_t stackBytes, BaseType_t core);

  /** @brief Log the task's stack high-water mark. */
  static void logStackWatermark();
};

}  // namespace PoolController
