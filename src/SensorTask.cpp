// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorTask.cpp
 * @brief DS18B20 + internal temperature measurement task.
 */

#include "SensorTask.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "SensorCycle.hpp"
#include "SystemMonitor.hpp"

namespace PoolController {

// Referenced from PoolController.cpp (namespace scope globals).
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;

namespace {
TaskHandle_t sensorTaskHandle = nullptr;
uint32_t lastDallasReadingMs = 0;
uint32_t lastControllerReadingMs = 0;
constexpr uint32_t CONVERSION_DELAY_MS = 800;  // >= 12-bit DS18B20 conversion
}  // namespace

void sensorTaskFunc(void *) {
  const bool watchdogRegistered = SystemMonitor::registerCurrentTaskWithWatchdog();

  for (;;) {
    const uint32_t now = millis();

    // Both Dallas nodes are measured in one cycle. Preserve the old recovery
    // behavior by using the shorter effective interval whenever either node is
    // missing/invalid (RECOVERY_INTERVAL = 5 s).
    const unsigned long solarInterval = solarTemperatureNode.getEffectiveMeasurementInterval();
    const unsigned long poolInterval = poolTemperatureNode.getEffectiveMeasurementInterval();
    const unsigned long dallasInterval = (solarInterval < poolInterval) ? solarInterval : poolInterval;

    if (now - lastDallasReadingMs >= dallasInterval * 1000UL) {
      lastDallasReadingMs = now;
      Serial.println("〽 SensorTask: reading Dallas sensors");
      // DallasTemperature is configured with waitForConversion=false in
      // DallasTemperatureNode::begin(), so the conversion starts immediately
      // and this task yields while the sensors convert.
      runDallasMeasurementCycle(solarTemperatureNode, poolTemperatureNode, [watchdogRegistered] {
        vTaskDelay(pdMS_TO_TICKS(CONVERSION_DELAY_MS));
        if (watchdogRegistered) {
          SystemMonitor::feedWatchdogFromTask();
        }
      });
    }

    // ESP32 internal temperature on its own interval.
    const unsigned long ctrlInterval = ctrlTemperatureNode.getMeasurementInterval();
    if (now - lastControllerReadingMs >= ctrlInterval * 1000UL) {
      lastControllerReadingMs = now;
      ctrlTemperatureNode.loop();
    }

    if (watchdogRegistered) {
      SystemMonitor::feedWatchdogFromTask();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

bool SensorTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  return xTaskCreatePinnedToCore(sensorTaskFunc, "sensor", stackBytes, nullptr, priority, &sensorTaskHandle, core) == pdPASS;
}

void SensorTask::logStackWatermark() {
  if (sensorTaskHandle != nullptr) {
    Serial.printf("  SensorTask stack high-water: %u B\n", static_cast<unsigned>(uxTaskGetStackHighWaterMark(sensorTaskHandle)));
  }
}

}  // namespace PoolController
