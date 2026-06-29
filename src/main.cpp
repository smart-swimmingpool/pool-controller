// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file main.cpp
 * @brief Main entry point for the Pool Controller firmware.
 *
 * Arduino entry point: setup() initializes all subsystems, loop() runs
 * the control, monitoring, and network stack indefinitely.
 */

#include <Arduino.h>
#include "PoolController.hpp"

/** @brief Singleton context owning all controller subsystems. */
static PoolController::PoolControllerContext context{};

/**
 * @brief Arduino setup() — initializes serial and delegates to PoolControllerContext.
 *
 * Waits up to 3 seconds for a USB serial connection (non-blocking fallback
 * for headless operation), then calls context.setup() to initialize all
 * subsystems.
 */
auto setup() -> void {
  Serial.begin(SERIAL_SPEED);

  // Wait for serial port to connect. Needed for native USB port only.
  // F29: Avoid infinite blocking when USB is not connected (e.g. native USB boards running headless).
  const uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) {
    delay(10);
  }

  context.setup();
}

/**
 * @brief Arduino loop() — runs the control, monitoring, and network stack.
 *
 * Called continuously after setup(). Delegates to context.loop() which
 * handles watchdog feeding, memory checks, node updates, MQTT, and OTA.
 */
auto loop() -> void {
  context.loop();
}
