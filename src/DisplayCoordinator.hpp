// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayCoordinator.hpp
 * @brief Owns NORVI button, UI-state and OLED rendering on Core 1.
 */

#pragma once

namespace PoolController {

class DisplayCoordinator {
public:
  /** Initialize the OLED and front-panel button callbacks. */
  static void begin();

  /** Poll buttons, apply UI events and render on the control-loop task. */
  static void loop();
};

}  // namespace PoolController
