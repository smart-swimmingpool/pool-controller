// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file OperationMode.hpp
 * @brief Typed operation-mode contract used by domain/application boundaries.
 */

#pragma once

#include <cstdint>
#include <cstring>

namespace PoolController {

/**
 * @brief Type-safe operation mode used internally by the controller.
 *
 * External protocols keep their historic wire values (notably "manu") and
 * convert only at the adapter boundary.
 */
enum class OperationMode : std::uint8_t {
  AUTO,
  MANUAL,
  BOOST,
  TIMER,
};

/** @brief Stable external representation used by MQTT, Web API and NVS. */
constexpr const char *toString(OperationMode mode) noexcept {
  switch (mode) {
    case OperationMode::AUTO:
      return "auto";
    case OperationMode::MANUAL:
      return "manu";
    case OperationMode::BOOST:
      return "boost";
    case OperationMode::TIMER:
      return "timer";
  }
  return "auto";
}

/**
 * @brief Parse an external operation-mode value without allocating memory.
 * @return true when @p value contains a known mode.
 */
inline bool tryParseOperationMode(const char *value, OperationMode &out) noexcept {
  if (value == nullptr) {
    return false;
  }
  if (std::strcmp(value, "auto") == 0) {
    out = OperationMode::AUTO;
    return true;
  }
  if (std::strcmp(value, "manu") == 0) {
    out = OperationMode::MANUAL;
    return true;
  }
  if (std::strcmp(value, "boost") == 0) {
    out = OperationMode::BOOST;
    return true;
  }
  if (std::strcmp(value, "timer") == 0) {
    out = OperationMode::TIMER;
    return true;
  }
  return false;
}

}  // namespace PoolController
