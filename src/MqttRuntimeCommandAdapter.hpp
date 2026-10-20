// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file MqttRuntimeCommandAdapter.hpp
 * @brief MQTT protocol adapter for typed Core-1 controller commands.
 */

#pragma once

#include <cstddef>

#include "ControllerCommand.hpp"
#include "MqttCommandQueue.hpp"

namespace PoolController {

class ControllerCommandHandler;

/**
 * @brief Moves MQTT callback data onto the loop task and translates runtime
 * mutations into allocation-free `ControllerCommand` values.
 *
 * Commands outside the runtime-controller scope (the firmware update) are
 * delegated to MqttPublisher::handleMqttMessage() on the loop task.
 */
class MqttRuntimeCommandAdapter final {
public:
  explicit MqttRuntimeCommandAdapter(ControllerCommandHandler &handler) : handler_(handler) {}

  /** @brief Register this adapter as the MQTT callback; call before MQTT connects. */
  void begin();

  /** @brief Drain a bounded number of queued messages on the Core-1 loop task. */
  void processPendingCommands();

  static constexpr std::size_t kMaxCommandsPerLoop = 2;

private:
  enum class ParseResult : std::uint8_t {
    NOT_RUNTIME,
    VALID,
    INVALID,
  };

  void onMqttMessage(const char *topic, const char *payload, std::size_t len, std::size_t index, std::size_t total);
  ParseResult parseRuntimeCommand(const MqttCommandQueue::Message &message, ControllerCommand &command) const;

  ControllerCommandHandler &handler_;
  MqttCommandQueue rawQueue_;
};

}  // namespace PoolController
