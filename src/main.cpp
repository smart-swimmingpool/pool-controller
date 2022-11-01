/**
 * Smart Swimming Pool - Pool Contoller
 *
 * Main entry of sketch.
 */

#include "PoolController.hpp"

using namespace PoolController;

static PoolControllerContext context { };

/**
 * Setup of controller.
 */
extern "C" auto setup() -> void
{
  Serial.begin(SERIAL_SPEED);

  // Wait for serial port to connect. Needed for native USB port only
  while (!Serial);

  context.setup();
}

/**
 * Main loop of ESP.
 */
extern "C" auto loop() -> void
{
  context.loop();
}
