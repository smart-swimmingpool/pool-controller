/**
 * Smart Swimming Pool - Pool Contoller
 *
 * Main entry of sketch.
 */

#include "PoolController.hpp"

using namespace PoolController;

static PoolControllerContext context { };

/**
 * Startup of controller.
 */
auto setup() -> void {
  context.startup();
}

/**
 * Main loop of ESP.
 */
auto loop() -> void {
  context.loop();
}
