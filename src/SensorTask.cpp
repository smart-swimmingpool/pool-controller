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
#include "LogCapture.hpp"

namespace PoolController {

// Referenced from PoolController.cpp (namespace scope globals).
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;

namespace {
TaskHandle_t sensorTaskHandle = nullptr;
uint32_t lastDallasReadingMs = 0;
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
    const unsigned long dallasInterval = selectDallasCycleInterval(solarInterval, poolInterval);

    if (now - lastDallasReadingMs >= dallasInterval * 1000UL) {
      lastDallasReadingMs = now;
      LOG_DEBUG("〽 SensorTask: reading Dallas sensors\n");
      // DallasTemperature is configured with waitForConversion=false in
      // DallasTemperatureNode::begin(), so the conversion starts immediately
      // and this task yields while the sensors convert.
      runDallasMeasurementCycle(solarTemperatureNode, poolTemperatureNode, [watchdogRegistered] {
        vTaskDelay(pdMS_TO_TICKS(DallasTemperatureNode::CONVERSION_DELAY_MS));
        if (watchdogRegistered) {
          SystemMonitor::feedWatchdog();
        }
      });
    }

    // ESP32 internal temperature: the node enforces its own interval.
    ctrlTemperatureNode.loop();

    if (watchdogRegistered) {
      SystemMonitor::feedWatchdog();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

bool SensorTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  return xTaskCreatePinnedToCore(sensorTaskFunc, "sensor", stackBytes, nullptr, priority, &sensorTaskHandle, core) == pdPASS;
}

void SensorTask::logStackWatermark() {
  if (sensorTaskHandle != nullptr) {
    LOG_DEBUG("  SensorTask stack high-water: %u B\n", static_cast<unsigned>(uxTaskGetStackHighWaterMark(sensorTaskHandle)));
  }
}

}  // namespace PoolController
