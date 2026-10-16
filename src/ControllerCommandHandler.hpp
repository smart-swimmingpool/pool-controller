// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerCommandHandler.hpp
 * @brief Single Core-1 mutation API for runtime controller commands.
 */

#pragma once

#include <cstddef>

#include "ControllerCommand.hpp"
#include "ControllerNodes.hpp"

namespace PoolController {

/** @brief Outcome of a command; adapters map it to their protocol (e.g. HTTP). */
enum class CommandStatus : std::uint8_t {
  APPLIED,         ///< Validated, persisted where needed and applied.
  INVALID,         ///< Malformed or out-of-range value (HTTP 400).
  REJECTED,        ///< Valid, but not allowed in the current state (HTTP 409).
  PERSIST_FAILED,  ///< Reading or writing NVS failed; nothing was changed (HTTP 500).
};

/** @brief Optional resolved owner state returned to synchronous adapters. */
struct ControllerCommandResult final {
  bool hasMode{false};
  OperationMode mode{OperationMode::AUTO};
  bool hasPumpState{false};
  bool pumpState{false};
};

/**
 * @brief Hands a command batch to the handler on the owning task.
 *
 * Adapters (Web, local UIs) receive it from the composition root instead of
 * reaching the handler or the nodes directly.
 */
using CommandDispatcher = CommandStatus (*)(const ControllerCommand *, std::size_t, ControllerCommandResult *);

/** @brief Dispatch one command; REJECTED while no dispatcher is installed. */
inline CommandStatus dispatchCommand(
  CommandDispatcher dispatcher, const ControllerCommand &command, ControllerCommandResult *result = nullptr) {
  return dispatcher == nullptr ? CommandStatus::REJECTED : dispatcher(&command, 1, result);
}

/**
 * @brief Applies validated runtime commands to the legacy controller model.
 *
 * This service is the transitional application boundary between protocol
 * adapters and the existing nodes/static configuration services. It is owned
 * by the composition root and must only be invoked from the Core-1 Arduino
 * loop task.
 *
 * A batch is all-or-nothing: every command is validated against a staged copy
 * of the settings first, then the settings are saved once, and only after a
 * successful save are the running nodes and services updated. A failure at
 * any step leaves both the persisted and the runtime state unchanged.
 */
class ControllerCommandHandler final {
public:
  using Dependencies = ControllerNodes;

  explicit ControllerCommandHandler(Dependencies dependencies) : dependencies_(dependencies) {}

  /**
   * @brief Apply a batch of commands atomically on the owning application task.
   *
   * FACTORY_RESET must be sent alone. `result` (optional) receives the
   * resolved mode and the last toggled pump state.
   */
  CommandStatus execute(const ControllerCommand *commands, std::size_t count, ControllerCommandResult *result = nullptr);
  CommandStatus execute(const ControllerCommand &command, ControllerCommandResult *result = nullptr) {
    return execute(&command, 1, result);
  }

  /** @brief Convenience wrapper: true if the command was applied. */
  bool handle(const ControllerCommand &command, ControllerCommandResult *result = nullptr) {
    return execute(command, result) == CommandStatus::APPLIED;
  }

private:
  struct Plan;

  CommandStatus stage(const ControllerCommand &command, Plan &plan) const;
  CommandStatus stageSensorMapping(const ControllerCommand &command, Plan &plan) const;
  CommandStatus stagePumpCommand(const ControllerCommand &command, Plan &plan) const;
  void applyRuntime(const ControllerCommand &command, ControllerCommandResult *result);
  void applyLoopInterval(long seconds);
  void applySensorMapping(const ControllerCommand &command, const Plan &plan);
  void togglePump(const ControllerCommand &command, ControllerCommandResult *result);
  static CommandStatus factoryReset();
  static const char *sourceName(CommandSource source);

  Dependencies dependencies_;
};

}  // namespace PoolController
