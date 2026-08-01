// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.cpp
 * @brief Task creation for the Core-0 I/O tasks.
 */

#include "CoreScheduler.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "SensorTask.hpp"
#include "PublishTask.hpp"

namespace PoolController {

void CoreScheduler::begin() {
  // Core 0 = PRO_CPU_NUM (I/O core); Core 1 = APP_CPU_NUM (control loop).
  const BaseType_t core0 = PRO_CPU_NUM;

  SensorTask::start(TASK_PRIORITY_SENSOR, TASK_STACK_SENSOR, core0);
  PublishTask::start(TASK_PRIORITY_PUBLISH, TASK_STACK_PUBLISH, core0);
}

void CoreScheduler::logStackWatermarks() {
  static uint32_t lastLog = 0;
  if (millis() - lastLog < 60000) {
    return;
  }
  lastLog = millis();
  SensorTask::logStackWatermark();
  PublishTask::logStackWatermark();
}

}  // namespace PoolController
