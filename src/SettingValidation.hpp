// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file SettingValidation.hpp
 * @brief Single source of truth for parsing and range-checking numeric
 *        temperature settings received via MQTT.
 *
 * Used by MqttPublisher (Home Assistant commands and discovery limits) and
 * OperationModeNode so both accept the same syntax and the same ranges.
 * Header-only, allocation-free and hardware-independent for native tests.
 */

#pragma once

#include <cmath>
#include <cstdlib>

namespace PoolController {

struct FloatRange {
  float min;
  float max;
};

/// Valid ranges of the temperature settings (also published as HA limits).
namespace SettingLimits {
constexpr FloatRange kPoolMaxTemp{0.0f, 40.0f};
constexpr FloatRange kSolarMinTemp{0.0f, 100.0f};
constexpr FloatRange kHysteresis{0.0f, 10.0f};
constexpr FloatRange kTempCircThreshold{0.0f, 40.0f};
}  // namespace SettingLimits

/**
 * @brief Parse a plain decimal number: optional sign, digits, at most one
 *        decimal point, at least one digit.
 *
 * Rejects empty input, whitespace, exponents, "nan"/"inf" and trailing
 * characters. Arduino's String::toFloat() would turn such payloads into 0.0,
 * which lies inside most valid ranges.
 *
 * @param text NUL-terminated input
 */
inline bool parseDecimalFloat(const char *text, float &out) {
  if (text == nullptr) {
    return false;
  }
  const char *p = text;
  if (*p == '-' || *p == '+') {
    p++;
  }
  bool hasDigit = false;
  bool hasDot = false;
  for (; *p != '\0'; p++) {
    if (*p >= '0' && *p <= '9') {
      hasDigit = true;
    } else if (*p == '.' && !hasDot) {
      hasDot = true;
    } else {
      return false;
    }
  }
  if (!hasDigit) {
    return false;
  }
  float parsed = strtof(text, nullptr);
  if (!std::isfinite(parsed)) {
    return false;
  }
  out = parsed;
  return true;
}

/// Parse a plain decimal number and check it against @p range (inclusive).
inline bool parseFloatInRange(const char *text, FloatRange range, float &out) {
  float parsed = 0.0f;
  if (!parseDecimalFloat(text, parsed) || parsed < range.min || parsed > range.max) {
    return false;
  }
  out = parsed;
  return true;
}

}  // namespace PoolController
