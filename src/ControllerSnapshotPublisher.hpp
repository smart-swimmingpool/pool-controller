// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerSnapshotPublisher.hpp
 * @brief Core-1 builder and publisher for immutable controller read models.
 */

#pragma once

#include <array>
#include <cstdint>

#include "ControllerNodes.hpp"
#include "ControllerSnapshot.hpp"

namespace PoolController {

class ControllerSnapshotStore;

/**
 * @brief Builds one coherent `SystemSnapshot` from Core-1-owned runtime state.
 *
 * The publisher is invoked only after the application loop has updated the
 * legacy runtime model. Adapters can then read a value copy from the snapshot
 * store instead of dereferencing mutable nodes and settings directly.
 *
 * Not wired into loop() yet: building the snapshot every cycle costs time and
 * RAM, so it is added together with its first reader.
 */
class ControllerSnapshotPublisher final {
public:
  ControllerSnapshotPublisher(ControllerNodes nodes, ControllerSnapshotStore &store) : nodes_(nodes), store_(store) {}

  /** @brief Build and atomically publish the latest application read model. */
  void publish();

private:
  ControllerNodes nodes_;
  ControllerSnapshotStore &store_;
  std::array<DetectedSensorSnapshot, kMaxDetectedSensors> detectedInventory_{};
  std::uint8_t detectedInventoryCount_{0};
  std::uint32_t lastDetectedInventoryRefreshMs_{0};
  std::uint32_t generation_{0};
  bool detectedInventoryInitialized_{false};
};

}  // namespace PoolController
