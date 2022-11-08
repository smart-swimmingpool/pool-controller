/**
 * Smart Swimming Pool - Pool Contoller
 *
 * Main entry of sketch.
 */

#include "PoolController.hpp"

static PoolController::PoolControllerContext context { };

/**
 * Setup of controller.
 */
#ifndef ESP32
extern "C"
#endif
auto setup() -> void {
  Serial.begin(SERIAL_SPEED);

  // Wait for serial port to connect. Needed for native USB port only
  while (!Serial);

  context.setup();
}

/**
 * Main loop of ESP.
 */
#ifndef ESP32
extern "C"
#endif
auto loop() -> void {
  context.loop();
}
