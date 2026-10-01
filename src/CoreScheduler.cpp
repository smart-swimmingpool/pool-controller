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

#include "LogCapture.hpp"
#include "PublishTask.hpp"
#include "SensorTask.hpp"
#include "TaskStartupPolicy.hpp"
#ifdef NORVI_AE01_R
#include "DisplayTask.hpp"
#endif

namespace PoolController {

void CoreScheduler::begin() {
  // Core 0 = PRO_CPU_NUM (I/O core); Core 1 = APP_CPU_NUM (control loop).
  const BaseType_t core0 = PRO_CPU_NUM;

  const bool sensorStarted = SensorTask::start(TASK_PRIORITY_SENSOR, TASK_STACK_SENSOR, core0);
  const bool publishStarted = PublishTask::start(TASK_PRIORITY_PUBLISH, TASK_STACK_PUBLISH, core0);
#ifdef NORVI_AE01_R
  const bool displayStarted = DisplayTask::start(TASK_PRIORITY_DISPLAY, TASK_STACK_DISPLAY, core0);
#else
  const bool displayStarted = true;  // no display task on this board
#endif

  switch (decideTaskStartupAction(sensorStarted, publishStarted, displayStarted)) {
  case TaskStartupAction::CONTINUE:
    break;
  case TaskStartupAction::CONTINUE_DEGRADED:
    LOG_ERROR("✖ DisplayTask could not be created (free heap %u B) — continuing without OLED rendering\n",
      static_cast<unsigned>(ESP.getFreeHeap()));
    break;
  case TaskStartupAction::RESTART:
    LOG_ERROR("✖ Critical task could not be created (sensor: %s, publish: %s, free heap %u B) — restarting\n",
      sensorStarted ? "ok" : "FAILED", publishStarted ? "ok" : "FAILED", static_cast<unsigned>(ESP.getFreeHeap()));
    Serial.flush();
    // A persistent failure is caught by the boot-loop detection (safe mode).
    ESP.restart();
    break;
  }
}

void CoreScheduler::logStackWatermarks() {
  static uint32_t lastLog = 0;
  if (millis() - lastLog < 60000) {
    return;
  }
  lastLog = millis();
  SensorTask::logStackWatermark();
  PublishTask::logStackWatermark();
#ifdef NORVI_AE01_R
  DisplayTask::logStackWatermark();
#endif
}

}  // namespace PoolController
