// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file PublishTask.cpp
 * @brief MQTT publish task draining the telemetry queue.
 */

#include "PublishTask.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "TelemetryQueue.hpp"
#include "MqttPublisher.hpp"
#include "OtaUpdater.hpp"

namespace PoolController {

namespace {
TaskHandle_t publishTaskHandle = nullptr;
}  // namespace

void publishTaskFunc(void *) {
  for (;;) {
    PublishRequestKind kind;
    while (TelemetryQueue::instance().dequeue(kind)) {
      // Pause during OTA updates, but keep draining to avoid queue buildup.
      if (!OtaUpdater::isUpdateInProgress()) {
        if (kind == PublishRequestKind::DISCOVERY) {
          MqttPublisher::publishDiscovery();
        } else {
          MqttPublisher::publishStates();
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void PublishTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  xTaskCreatePinnedToCore(publishTaskFunc, "publish", stackBytes, nullptr, priority, &publishTaskHandle, core);
}

void PublishTask::logStackWatermark() {
  if (publishTaskHandle != nullptr) {
    Serial.printf(
      "  PublishTask stack high-water: %u B\n", static_cast<unsigned>(uxTaskGetStackHighWaterMark(publishTaskHandle)));
  }
}

}  // namespace PoolController
