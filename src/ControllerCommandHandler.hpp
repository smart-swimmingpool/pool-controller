// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerCommandHandler.hpp
 * @brief Single Core-1 mutation API for runtime controller commands.
 */

#pragma once

#include "ControllerCommand.hpp"

class DallasTemperatureNode;
class ESP32TemperatureNode;
class OperationModeNode;
class RelayModuleNode;

namespace PoolController {

/** @brief Optional resolved owner state returned to synchronous adapters. */
struct ControllerCommandResult final {
  bool hasMode{false};
  OperationMode mode{OperationMode::AUTO};
  bool hasPumpState{false};
  bool pumpState{false};
};

/**
 * @brief Applies validated runtime commands to the legacy controller model.
 *
 * This service is the transitional application boundary between protocol
 * adapters and the existing nodes/static configuration services. It is owned
 * by the composition root and must only be invoked from the Core-1 Arduino
 * loop task.
 */
class ControllerCommandHandler final {
public:
  /** @brief Explicit legacy runtime dependencies supplied by the composition root. */
  struct Dependencies final {
    OperationModeNode &operationMode;
    RelayModuleNode &poolPump;
    RelayModuleNode &solarPump;
    DallasTemperatureNode &solarTemperature;
    DallasTemperatureNode &poolTemperature;
    ESP32TemperatureNode &controllerTemperature;
  };

  explicit ControllerCommandHandler(Dependencies dependencies) : dependencies_(dependencies) {}

  /**
   * @brief Apply one command on the owning application task.
   * @return true if the command was accepted and applied, false if invalid.
   */
  bool handle(const ControllerCommand &command) { return handle(command, nullptr); }
  bool handle(const ControllerCommand &command, ControllerCommandResult *result);

private:
  static const char *sourceName(CommandSource source);
  bool saveSettings() const;
  bool applySensorMapping(const ControllerCommand &command, bool clear);

  Dependencies dependencies_;
};

}  // namespace PoolController
