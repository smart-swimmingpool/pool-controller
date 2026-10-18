#pragma once

#include "ControllerCommandHandler.hpp"

namespace PoolController {

class OlimexLocalUi {
public:
  static void begin();
  static void loop();
  static void requestRedraw();
  /** Route menu actions (mode, pump) through the command handler. */
  static void setCommandDispatcher(CommandDispatcher dispatcher);
};

}  // namespace PoolController
