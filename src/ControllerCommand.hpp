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

/** @brief Commands that may mutate application state. */
enum class ControllerCommandType : std::uint8_t {
  SET_MODE,
  SET_POOL_MAX_TEMPERATURE,
  SET_SOLAR_MIN_TEMPERATURE,
  SET_TEMPERATURE_HYSTERESIS,
  SET_TEMPERATURE_CIRCULATION_THRESHOLD,
  SET_TEMPERATURE_CIRCULATION_FACTOR,
  SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME,
  SET_TIMER,
  SET_SENSOR_MAPPING,
  CLEAR_SENSOR_MAPPING,
  SET_POOL_PUMP_MANUAL,
  SET_SOLAR_PUMP_MANUAL,
  FACTORY_RESET,
};

/**
 * @brief Fixed-size command payload suitable for a bounded FreeRTOS queue.
 *
 * Only the fields relevant for `type` are interpreted. Numeric controller
 * settings use `value`; integer-only settings are validated by the application
 * handler before being applied. The intentionally flat layout avoids heap
 * allocations and makes commands cheap to copy between callback/task boundaries.
 */
struct ControllerCommand final {
  ControllerCommandType type{ControllerCommandType::SET_MODE};
  CommandSource source{CommandSource::INTERNAL};

  OperationMode mode{OperationMode::AUTO};
  float value{0.0F};
  bool enabled{false};

  std::uint8_t startHour{0};
  std::uint8_t startMinute{0};
  std::uint8_t endHour{0};
  std::uint8_t endMinute{0};

  SensorRole sensorRole{SensorRole::SOLAR};
  std::array<std::uint8_t, 8> sensorAddress{};
};

static_assert(std::is_trivially_copyable<ControllerCommand>::value,
  "ControllerCommand must remain trivially copyable for fixed-size task queues");

}  // namespace PoolController
