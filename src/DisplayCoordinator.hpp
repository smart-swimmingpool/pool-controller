// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayCoordinator.hpp
 * @brief Serializes NORVI UI state between the control loop and DisplayTask.
 */

#pragma once

namespace PoolController {

class DisplayCoordinator {
public:
  /** Initialize the OLED, button callbacks, and state synchronization. */
  static void begin();

  /** Poll buttons and advance UI state without ever blocking the control loop. */
  static void loop();

  /** Render from DisplayTask while owning the UI state. */
  static void render();
};

}  // namespace PoolController
