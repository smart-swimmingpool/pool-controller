// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file TemperatureReadingFilter.hpp
 * @brief Plausibility filter for DS18B20 readings.
 *
 * - Values outside the sensor range (-55…125 °C), NaN and the
 *   "disconnected" marker (-127 °C) are rejected.
 * - 85.0 °C is the DS18B20 power-on reset value, typically read after a
 *   supply glitch on long outdoor cables. A solar collector can really reach
 *   85 °C, so the value is only accepted when it continues a nearby previous
 *   reading or is confirmed by consecutive reads; otherwise the previous
 *   temperature is held.
 *
 * Header-only and hardware-independent for native unit tests.
 */

#pragma once

#include <cmath>
#include <cstdint>

namespace PoolController {

class TemperatureReadingFilter {
public:
  enum class Action : uint8_t {
    ACCEPT,  ///< Use the value
    HOLD,    ///< Transient power-on value — keep the previous temperature
    REJECT,  ///< Invalid reading — treat like a failed read
  };

  static constexpr float kMinValidC = -55.0f;
  static constexpr float kMaxValidC = 125.0f;
  static constexpr float kPowerOnResetC = 85.0f;
  static constexpr float kPowerOnContinuityC = 5.0f;  ///< 85 °C is plausible within this distance
  static constexpr uint8_t kPowerOnConfirmReads = 3;  ///< Consecutive 85 °C reads that confirm it

  Action apply(float raw) {
    if (std::isnan(raw) || raw < kMinValidC || raw > kMaxValidC) {
      powerOnReads_ = 0;
      return Action::REJECT;
    }
    if (raw == kPowerOnResetC) {
      bool continuesPrevious = !std::isnan(lastAccepted_) && std::fabs(lastAccepted_ - kPowerOnResetC) <= kPowerOnContinuityC;
      if (!continuesPrevious) {
        powerOnReads_++;
        if (powerOnReads_ < kPowerOnConfirmReads) {
          return std::isnan(lastAccepted_) ? Action::REJECT : Action::HOLD;
        }
      }
    }
    powerOnReads_ = 0;
    lastAccepted_ = raw;
    return Action::ACCEPT;
  }

  void reset() {
    lastAccepted_ = NAN;
    powerOnReads_ = 0;
  }

private:
  float lastAccepted_ = NAN;
  uint8_t powerOnReads_ = 0;
};

}  // namespace PoolController
