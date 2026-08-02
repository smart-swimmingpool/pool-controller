// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayTask.hpp
 * @brief Core-0 task rendering the NORVI OLED display (NORVI_AE01_R only).
 */

#pragma once

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace PoolController {

/**
 * @brief Renders the OLED display on Core 0.
 *
 * The control loop advances the display state machine and requests renders;
 * this task owns the I2C SSD1306 work so a hung display can never stall
 * the control loop. NORVI_AE01_R only.
 */
class DisplayTask {
public:
  /** @brief Create and start the task pinned to the given core. */
  static void start(uint8_t priority, uint16_t stackBytes, BaseType_t core);

  /** @brief Request a render on the next task tick. */
  static void requestRender();

  /** @brief Log the task's stack high-water mark. */
  static void logStackWatermark();
};

}  // namespace PoolController
