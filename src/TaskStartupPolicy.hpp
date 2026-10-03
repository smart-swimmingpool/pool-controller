// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file TaskStartupPolicy.hpp
 * @brief What to do when a Core-0 task could not be created.
 *
 * xTaskCreatePinnedToCore() only fails when the heap cannot hold the task
 * stack. Sensor reads and MQTT publishing no longer run in the control loop,
 * so without SensorTask the rules would work on stale temperatures and
 * without PublishTask Home Assistant would get no telemetry. These are
 * critical: the controller restarts. A persistent failure ends in the
 * boot-loop detection (3 consecutive boots), which forces safe mode with all
 * relays off. The display task is not needed for control — the controller
 * continues without OLED rendering.
 *
 * Header-only and hardware-independent for native tests.
 */

#pragma once

#include <cstdint>

namespace PoolController {

enum class TaskStartupAction : uint8_t {
  CONTINUE,           ///< All tasks running
  CONTINUE_DEGRADED,  ///< Non-critical task missing (display)
  RESTART,            ///< Critical task missing (sensor or publish)
};

inline TaskStartupAction decideTaskStartupAction(bool sensorStarted, bool publishStarted, bool displayStarted) {
  if (!sensorStarted || !publishStarted) {
    return TaskStartupAction::RESTART;
  }
  if (!displayStarted) {
    return TaskStartupAction::CONTINUE_DEGRADED;
  }
  return TaskStartupAction::CONTINUE;
}

}  // namespace PoolController
