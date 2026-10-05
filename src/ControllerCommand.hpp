// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerCommand.hpp
 * @brief Fixed-size application command contract for MQTT/Web/local UI adapters.
 */

#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "OperationMode.hpp"
#include "NtpServerValue.hpp"

namespace PoolController {

/** @brief Origin of a command for diagnostics and audit logging. */
enum class CommandSource : std::uint8_t {
  INTERNAL,
  MQTT,
  WEB,
  LOCAL_UI,
};

/** @brief Logical DS18B20 role independent of physical bus topology. */
enum class SensorRole : std::uint8_t {
  SOLAR,
  POOL,
};

/** @brief Existing toggle use cases; applied atomically by the state owner. */
enum class PumpToggleModePolicy : std::uint8_t {
  REQUIRE_MANUAL,  ///< Web: reject outside manual mode.
  KEEP_MODE,       ///< NORVI menu: toggle without changing the current mode.
  ENTER_MANUAL,    ///< Olimex UI: enter manual mode, then toggle the live relay.
};

/** @brief Commands that may mutate runtime controller state or controller settings. */
enum class ControllerCommandType : std::uint8_t {
  SET_MODE,
  SET_POOL_MAX_TEMPERATURE,
  SET_SOLAR_MIN_TEMPERATURE,
  SET_TEMPERATURE_HYSTERESIS,
  SET_TEMPERATURE_CIRCULATION_THRESHOLD,
  SET_TEMPERATURE_CIRCULATION_FACTOR,
  SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME,
  SET_TIMER_START,
  SET_TIMER_END,
  SET_LOOP_INTERVAL,
  SET_TIMEZONE,
  SET_TIME_LOSS_GREEN_HOURS,
  SET_TIME_LOSS_RED_HOURS,
  SET_BUTTON_1_MIN,
  SET_BUTTON_1_MAX,
  SET_BUTTON_2_MIN,
  SET_BUTTON_2_MAX,
  SET_BUTTON_3_MIN,
  SET_BUTTON_3_MAX,
  SET_BUTTON_NO_PRESS,
  SET_SENSOR_MAPPING,
  CLEAR_SENSOR_MAPPING,
  SET_POOL_PUMP_MANUAL,
  SET_SOLAR_PUMP_MANUAL,
  FACTORY_RESET,
  SET_NTP_SERVER,
  TOGGLE_POOL_PUMP,
  TOGGLE_SOLAR_PUMP,
  CYCLE_MODE,
};

/**
 * @brief Fixed-size command payload suitable for a bounded FreeRTOS queue.
 *
 * Only the fields relevant for `type` are interpreted. Decimal settings use
 * `value`, integer settings use `integerValue`, and timer start/end commands
 * use `hour`/`minute`. Values are validated by the Core-1 application handler
 * before being applied. The intentionally flat layout avoids heap allocations
 * and makes commands cheap to copy between callback/task boundaries.
 * `SET_NTP_SERVER` uses `ntpServer`; the handler rejects invalid text before
 * persisting it and asking the time service to apply the change.
 * Toggle/cycle commands carry relative intent, ignoring `enabled`/`mode`.
 * They must be evaluated in FIFO order against live owner state, never against
 * an adapter snapshot. Toggles use `toggleModePolicy` to preserve the existing
 * Web/NORVI/Olimex mode behavior as one action. Safety checks apply at execution.
 *
 * Provisioning/authentication data such as WiFi/MQTT credentials and passwords
 * is deliberately not transported through this general controller command
 * queue; those secrets remain owned by dedicated configuration/auth services.
 */
struct ControllerCommand final {
  ControllerCommandType type{ControllerCommandType::SET_MODE};
  CommandSource source{CommandSource::INTERNAL};

  OperationMode mode{OperationMode::AUTO};
  float value{0.0F};
  std::int32_t integerValue{0};
  bool enabled{false};

  std::uint8_t hour{0};
  std::uint8_t minute{0};

  SensorRole sensorRole{SensorRole::SOLAR};
  std::array<std::uint8_t, 8> sensorAddress{};
  NtpServerValue ntpServer{};
  PumpToggleModePolicy toggleModePolicy{PumpToggleModePolicy::REQUIRE_MANUAL};
};

static_assert(std::is_trivially_copyable<ControllerCommand>::value,
  "ControllerCommand must remain trivially copyable for fixed-size task queues");

}  // namespace PoolController
