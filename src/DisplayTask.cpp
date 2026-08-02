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

#include "NorviOledDisplay.hpp"

namespace PoolController {

namespace {
TaskHandle_t displayTaskHandle = nullptr;
volatile bool renderRequested = false;
}  // namespace

void displayTaskFunc(void *) {
  for (;;) {
    if (renderRequested || (millis() % 2000 < 50)) {
      renderRequested = false;
      NorviOledDisplay::render();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void DisplayTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  xTaskCreatePinnedToCore(displayTaskFunc, "display", stackBytes, nullptr, priority, &displayTaskHandle, core);
}

void DisplayTask::requestRender() { renderRequested = true; }

void DisplayTask::logStackWatermark() {
  if (displayTaskHandle != nullptr) {
    Serial.printf("  DisplayTask stack high-water: %u B\n",
      static_cast<unsigned>(uxTaskGetStackHighWaterMark(displayTaskHandle)));
  }
}

}  // namespace PoolController
