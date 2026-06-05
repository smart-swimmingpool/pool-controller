// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * Smart Swimming Pool - Pool Controller
 *
 * Main entry of sketch.
 */

#include <Arduino.h>
#include "PoolController.hpp"

static PoolController::PoolControllerContext context{};

/**
 * Setup of controller.
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
 * Main loop of ESP.
 */
auto loop() -> void {
  context.loop();
}
