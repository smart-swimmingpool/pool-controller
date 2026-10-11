// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file MqttRuntimeCommandAdapter.hpp
 * @brief MQTT protocol adapter for typed Core-1 controller commands.
 */

#pragma once

#include <AsyncMqttClient.h>
#include <cstddef>

#include "ControllerCommand.hpp"
#include "MqttCommandQueue.hpp"

namespace PoolController {

class ControllerCommandHandler;

/**
 * @brief Moves MQTT callback data onto the loop task and translates runtime
 * mutations into allocation-free `ControllerCommand` values.
 *
 * Commands outside the runtime-controller scope (for example OTA and NTP
 * server provisioning) are delegated to the existing MQTT implementation on
 * the loop task until their dedicated service boundaries are extracted.
 */
class MqttRuntimeCommandAdapter final {
public:
  explicit MqttRuntimeCommandAdapter(ControllerCommandHandler &handler) : handler_(handler) {}

  /** @brief Replace the MQTT callback with this adapter after MqttPublisher::begin(). */
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

  void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, std::size_t len, std::size_t index,
    std::size_t total);
  ParseResult parseRuntimeCommand(const MqttCommandQueue::Message &message, ControllerCommand &command) const;

  ControllerCommandHandler &handler_;
  MqttCommandQueue rawQueue_;
};

}  // namespace PoolController
