// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayTask.cpp
 * @brief OLED render task (NORVI_AE01_R only).
 */

#include "DisplayTask.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>

#include "DisplayCoordinator.hpp"
#include "SystemMonitor.hpp"

namespace PoolController {

namespace {
TaskHandle_t displayTaskHandle = nullptr;
std::atomic<bool> renderRequested{false};
constexpr uint32_t FALLBACK_RENDER_INTERVAL_MS = 2000;
}  // namespace

void displayTaskFunc(void *) {
  const bool watchdogRegistered = SystemMonitor::registerCurrentTaskWithWatchdog();
  uint32_t lastRenderMs = millis();

  for (;;) {
    const uint32_t now = millis();
    const bool requested = renderRequested.exchange(false, std::memory_order_acq_rel);
    if (requested || now - lastRenderMs >= FALLBACK_RENDER_INTERVAL_MS) {
      lastRenderMs = now;
      DisplayCoordinator::render();
    }
    if (watchdogRegistered) {
      SystemMonitor::feedWatchdogFromTask();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

bool DisplayTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  return xTaskCreatePinnedToCore(displayTaskFunc, "display", stackBytes, nullptr, priority, &displayTaskHandle, core) == pdPASS;
}

void DisplayTask::requestRender() {
  renderRequested.store(true, std::memory_order_release);
}

void DisplayTask::logStackWatermark() {
  if (displayTaskHandle != nullptr) {
    Serial.printf(
      "  DisplayTask stack high-water: %u B\n", static_cast<unsigned>(uxTaskGetStackHighWaterMark(displayTaskHandle)));
  }
}

}  // namespace PoolController
