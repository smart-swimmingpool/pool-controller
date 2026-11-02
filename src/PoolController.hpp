// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file PoolController.hpp
 * @brief Core controller context — owns all subsystems and drives the main loop.
 */

#pragma once

namespace PoolController {

/**
 * @brief Singleton context that initializes and runs all controller subsystems.
 *
 * Owns temperature sensor nodes, relay nodes, rule engine, network stack,
 * web portal, MQTT publisher, OTA updater, and system monitoring.
 * Uses RAII — constructor builds the context, setup() initializes hardware,
 * loop() runs the control cycle.
 *
 * When built with `NORVI_AE01_R`, the context also owns the NORVI-specific
 * OLED display and front-panel button handler.
 */
struct PoolControllerContext final {
  PoolControllerContext();
  // no copy
  PoolControllerContext(const PoolControllerContext &) = delete;
  // no move
  PoolControllerContext(PoolControllerContext &&) = delete;
  // no copy
  auto operator=(const PoolControllerContext &) -> PoolControllerContext & = delete;
  // no move
  auto operator=(PoolControllerContext &&) -> PoolControllerContext & = delete;
  ~PoolControllerContext();

  /**
   * @brief Startup the controller.
   * Calls begin() on all subsystems in dependency order (Preferences → SystemMonitor
   * → DegradationManager → ConfigManager → Network → WebPortal → MQTT → nodes).
   * @note Call from the Arduino setup() function exactly once.
   */
  auto setup() -> void;

  /**
   * @brief Run the main control loop iteration.
   * Feeds watchdog, checks memory, runs managers (network, web, OTA), updates
   * relay/operation-mode nodes, evaluates rules, and publishes MQTT states
   * periodically. Temperature sensor reads run in SensorTask on Core 0
   * (started from setup() via CoreScheduler) and publish into SensorSlots.
   * @note Call from the Arduino loop() function indefinitely.
   */
  auto loop() -> void;

private:
  /**
   * @brief Initialize hardware nodes and rule engine.
   * Validates pin configuration, sets measurement intervals, initializes NTP,
   * configures time degradation limits, creates rule instances.
   */
  auto initializeController() -> void;

  bool bootLoopDetected_ = false;
};

}  // namespace PoolController
