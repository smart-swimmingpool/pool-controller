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
  context.setup();
}

/**
 * Main loop of ESP.
 */
extern "C" auto loop() -> void
{
  context.loop();
}
