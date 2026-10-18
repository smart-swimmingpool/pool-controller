// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayCoordinator.hpp
 * @brief Owns NORVI button, UI-state and OLED rendering on Core 1.
 */

#pragma once

#include "ControllerCommandHandler.hpp"

namespace PoolController {

class DisplayCoordinator {
public:
  /** Initialize the OLED and front-panel button callbacks. */
  static void begin();

  /** Poll buttons, apply UI events and render on the control-loop task. */
  static void loop();

  /** Route menu actions (mode, pump, sensor mapping) through the command handler. */
  static void setCommandDispatcher(CommandDispatcher dispatcher);
};

}  // namespace PoolController
