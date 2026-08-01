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
#include "SystemMonitor.hpp"

namespace PoolController {

// Referenced from PoolController.cpp (namespace scope globals).
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;

namespace {
TaskHandle_t sensorTaskHandle = nullptr;
uint32_t lastSolarReadingMs = 0;
uint32_t lastControllerReadingMs = 0;
constexpr uint32_t CONVERSION_DELAY_MS = 800;  // 12-bit DS18B20 conversion
}  // namespace

void sensorTaskFunc(void *) {
  for (;;) {
    const uint32_t now = millis();

    // Solar (master on shared NORVI bus) drives the shared conversion.
    const unsigned long solarInterval = solarTemperatureNode.getMeasurementInterval();
    if (now - lastSolarReadingMs >= solarInterval * 1000UL) {
      lastSolarReadingMs = now;
      Serial.println("〽 SensorTask: reading Dallas sensors");
      solarTemperatureNode.beginMeasurement();
      // Yield while the conversion runs — never block the control loop.
      vTaskDelay(pdMS_TO_TICKS(CONVERSION_DELAY_MS));
      // Feed from the task context so long I/O waits can't starve the WDT.
      SystemMonitor::feedWatchdogFromTask();
      solarTemperatureNode.finishMeasurement();
      poolTemperatureNode.finishMeasurement();
    }

    // ESP32 internal temperature on its own interval.
    const unsigned long ctrlInterval = ctrlTemperatureNode.getMeasurementInterval();
    if (now - lastControllerReadingMs >= ctrlInterval * 1000UL) {
      lastControllerReadingMs = now;
      ctrlTemperatureNode.loop();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void SensorTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  xTaskCreatePinnedToCore(sensorTaskFunc, "sensor", stackBytes, nullptr, priority, &sensorTaskHandle, core);
}

void SensorTask::logStackWatermark() {
  if (sensorTaskHandle != nullptr) {
    Serial.printf("  SensorTask stack high-water: %u B\n",
      static_cast<unsigned>(uxTaskGetStackHighWaterMark(sensorTaskHandle)));
  }
}

}  // namespace PoolController
