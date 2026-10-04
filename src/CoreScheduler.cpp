// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.cpp
 * @brief Starts and monitors the dedicated Core-0 sensor task.
 */

#include "CoreScheduler.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "LogCapture.hpp"
#include "SensorTask.hpp"

namespace PoolController {

void CoreScheduler::begin() {
  // Core 0 = PRO_CPU_NUM (sensor I/O); Core 1 = APP_CPU_NUM (control loop).
  const BaseType_t core0 = PRO_CPU_NUM;

  if (!SensorTask::start(TASK_PRIORITY_SENSOR, TASK_STACK_SENSOR, core0)) {
    LOG_ERROR("✖ SensorTask could not be created (free heap %u B) — restarting\n", static_cast<unsigned>(ESP.getFreeHeap()));
    Serial.flush();
    // A persistent failure is caught by boot-loop detection and forces safe mode.
    ESP.restart();
  }
}

void CoreScheduler::logStackWatermarks() {
  static uint32_t lastLog = 0;
  if (millis() - lastLog < 60000) {
    return;
  }
  lastLog = millis();
  SensorTask::logStackWatermark();
}

}  // namespace PoolController
