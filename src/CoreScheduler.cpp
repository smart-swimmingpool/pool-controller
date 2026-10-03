// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.cpp
 * @brief Core-0 sensor task launcher and Core-1 telemetry service.
 */

#include "CoreScheduler.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "LogCapture.hpp"
#include "MqttPublisher.hpp"
#include "OtaUpdater.hpp"
#include "SensorTask.hpp"
#include "TelemetryQueue.hpp"

namespace PoolController {

void CoreScheduler::begin() {
  // Core 0 = PRO_CPU_NUM (I/O core); Core 1 = APP_CPU_NUM (control loop).
  const BaseType_t core0 = PRO_CPU_NUM;

  if (!SensorTask::start(TASK_PRIORITY_SENSOR, TASK_STACK_SENSOR, core0)) {
    LOG_ERROR("✖ SensorTask could not be created (free heap %u B) — restarting\n", static_cast<unsigned>(ESP.getFreeHeap()));
    Serial.flush();
    // A persistent failure is caught by boot-loop detection and forces safe mode.
    ESP.restart();
  }
}

void CoreScheduler::logStackWatermarks() {
  // This method is invoked from PoolController::loop() on Core 1. Drain the
  // request queue here so MqttPublisher never races the mutable control model.
  PublishRequestKind kind;
  while (TelemetryQueue::instance().dequeue(kind)) {
    // Preserve the previous OTA behavior: requests are drained but not sent
    // while the updater owns the network path.
    if (OtaUpdater::isUpdateInProgress()) {
      continue;
    }

    if (kind == PublishRequestKind::DISCOVERY) {
      MqttPublisher::publishDiscovery();
    } else {
      MqttPublisher::publishStates();
    }
  }

  static uint32_t lastLog = 0;
  if (millis() - lastLog < 60000) {
    return;
  }
  lastLog = millis();
  SensorTask::logStackWatermark();
}

}  // namespace PoolController
