// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerSnapshotPublisher.hpp
 * @brief Core-1 builder and publisher for immutable controller read models.
 */

#pragma once

#include <cstdint>

class DallasTemperatureNode;
class ESP32TemperatureNode;
class OperationModeNode;
class RelayModuleNode;

namespace PoolController {

class ControllerSnapshotStore;

/**
 * @brief Builds one coherent `SystemSnapshot` from Core-1-owned runtime state.
 *
 * The publisher is invoked only after the application loop has updated the
 * legacy runtime model. Adapters can then read a value copy from the snapshot
 * store instead of dereferencing mutable nodes and settings directly.
 */
class ControllerSnapshotPublisher final {
public:
  struct Dependencies final {
    OperationModeNode &operationMode;
    RelayModuleNode &poolPump;
    RelayModuleNode &solarPump;
    DallasTemperatureNode &solarTemperature;
    DallasTemperatureNode &poolTemperature;
    ESP32TemperatureNode &controllerTemperature;
    ControllerSnapshotStore &store;
  };

  explicit ControllerSnapshotPublisher(Dependencies dependencies) : dependencies_(dependencies) {}

  /** @brief Build and atomically publish the latest application read model. */
  void publish();

private:
  Dependencies dependencies_;
  std::uint32_t generation_{0};
};

}  // namespace PoolController
