// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.cpp
 * @brief Starts and monitors the sensor task (Core 0; the only core on ESP32-C6).
 */

#include "CoreScheduler.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "LogCapture.hpp"
#include "SensorTask.hpp"

namespace PoolController {

void CoreScheduler::begin() {
  // Dual-core ESP32 (esp32dev, NORVI): SensorTask on PRO_CPU (Core 0), the
  // Arduino loop on APP_CPU (Core 1). The ESP32-C6 has a single application
  // core, so both tasks share it and SensorTask (priority 2) preempts the
  // loop (priority 1) while it runs; see docs/multicore-architecture.md.
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
