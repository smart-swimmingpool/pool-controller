// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerSnapshot.hpp
 * @brief Immutable read-model contracts for adapters and cross-task publication.
 */

#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "OperationMode.hpp"

namespace PoolController {

/** @brief One coherent sensor value with validity information. */
struct SensorReadingSnapshot final {
  float value{0.0F};
  bool valid{false};
};

/** @brief Logical sensor mapping identity independent of the physical Dallas bus topology. */
struct SensorMappingSnapshot final {
  std::array<std::uint8_t, 8> address{};
  bool configured{false};
  bool found{false};
};

/**
 * @brief Temperatures and role mappings produced by one acquisition generation.
 *
 * `generation` lets consumers detect a new complete measurement cycle without
 * comparing individual fields. `measuredAtMs` is the acquisition timestamp.
 */
struct SensorSnapshot final {
  SensorReadingSnapshot pool{};
  SensorReadingSnapshot solar{};
  SensorReadingSnapshot controller{};
  SensorMappingSnapshot poolMapping{};
  SensorMappingSnapshot solarMapping{};
  std::uint32_t measuredAtMs{0};
  std::uint32_t generation{0};
};

/** @brief Network state projected for UI/MQTT/Web consumers. */
struct NetworkSnapshot final {
  bool wifiConnected{false};
  bool mqttConnected{false};
  bool apMode{false};
  std::int16_t wifiRssi{0};
};

/** @brief Three-state time quality used by the existing Web status model. */
enum class TimeDegradationState : std::uint8_t {
  GREEN = 0,
  YELLOW = 1,
  RED = 2,
};

/** @brief Safety/degradation state projected without exposing manager internals. */
struct HealthSnapshot final {
  bool safeMode{false};
  bool timeValid{false};
  TimeDegradationState timeDegradation{TimeDegradationState::RED};
  bool poolSensorValid{false};
  bool solarSensorValid{false};
};

/** @brief Temperature-based circulation configuration exposed to adapters. */
struct CirculationSnapshot final {
  float threshold{0.0F};
  std::uint16_t factorMinutesPerDegree{0};
  std::uint16_t maxRuntimeMinutes{0};
};

/** @brief Timer configuration and derived runtime state exposed to adapters. */
struct TimerSnapshot final {
  std::uint8_t startHour{0};
  std::uint8_t startMinute{0};
  std::uint8_t endHour{0};
  std::uint8_t endMinute{0};
  std::uint16_t effectiveRuntimeMinutes{0};
  std::uint16_t circulationExtensionMinutes{0};
  std::uint16_t activeEndMinutes{0};
};

/** @brief Runtime controller settings needed by status/config projections. */
struct ControllerSettingsSnapshot final {
  std::uint32_t loopInterval{0};
  std::int16_t timezoneIndex{0};
  std::uint16_t timeLossGreenHours{0};
  std::uint16_t timeLossRedHours{0};
  std::uint16_t button1Min{0};
  std::uint16_t button1Max{0};
  std::uint16_t button2Min{0};
  std::uint16_t button2Max{0};
  std::uint16_t button3Min{0};
  std::uint16_t button3Max{0};
  std::uint16_t buttonNoPress{0};
};

/**
 * @brief Read-only application projection consumed by outbound adapters.
 *
 * Future MQTT/Web/display code should consume this structure rather than
 * dereferencing global nodes or mutable manager singletons directly.
 */
struct SystemSnapshot final {
  SensorSnapshot sensors{};
  NetworkSnapshot network{};
  HealthSnapshot health{};
  CirculationSnapshot circulation{};
  TimerSnapshot timer{};
  ControllerSettingsSnapshot settings{};

  OperationMode mode{OperationMode::AUTO};
  bool poolPumpOn{false};
  bool solarPumpOn{false};

  float poolMaxTemperature{0.0F};
  float solarMinTemperature{0.0F};
  float temperatureHysteresis{0.0F};

  std::uint32_t uptimeMs{0};
  std::uint32_t freeHeapBytes{0};
};

static_assert(std::is_trivially_copyable<SensorSnapshot>::value,
  "SensorSnapshot must remain trivially copyable for bounded snapshot transport");
static_assert(std::is_trivially_copyable<SystemSnapshot>::value,
  "SystemSnapshot must remain trivially copyable for bounded snapshot transport");

}  // namespace PoolController
