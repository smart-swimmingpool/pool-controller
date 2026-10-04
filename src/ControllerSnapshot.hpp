// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerSnapshot.hpp
 * @brief Immutable read-model contracts for adapters and cross-task publication.
 */

#pragma once

#include <cstdint>
#include <type_traits>

#include "OperationMode.hpp"

namespace PoolController {

/** @brief One coherent sensor value with validity information. */
struct SensorReadingSnapshot final {
  float value{0.0F};
  bool valid{false};
};

/**
 * @brief Temperatures produced by one acquisition generation.
 *
 * `generation` lets consumers detect a new complete measurement cycle without
 * comparing individual fields. `measuredAtMs` is the acquisition timestamp.
 */
struct SensorSnapshot final {
  SensorReadingSnapshot pool{};
  SensorReadingSnapshot solar{};
  SensorReadingSnapshot controller{};
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

/** @brief Safety/degradation state projected without exposing manager internals. */
struct HealthSnapshot final {
  bool safeMode{false};
  bool timeValid{false};
  bool poolSensorValid{false};
  bool solarSensorValid{false};
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
