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
 *   reading. Repetition never confirms it: a recurring brownout delivers the
 *   same power-on value every time and must not turn into a hot collector
 *   that starts the solar pump.
 * - A non-continuing 85 °C holds the previous temperature for at most
 *   kMaxPowerOnHolds reads. After that, or without a valid previous value,
 *   it yields no temperature (UNCONFIRMED) and forgets the previous value.
 *   UNCONFIRMED is not a sensor failure, so it does not count towards a bus
 *   rescan.
 * - A rejected reading interrupts the valid sequence: continuity is not
 *   carried across a sensor failure, because a reconnect is exactly when the
 *   power-on value appears. HOLD is therefore only returned while the
 *   previous temperature is still valid.
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
    ACCEPT,       ///< Use the value
    HOLD,         ///< Transient power-on value — keep the previous temperature
    REJECT,       ///< Invalid reading — treat like a failed read
    UNCONFIRMED,  ///< 85 °C without a valid previous value, not yet confirmed — no value, but no failure either
  };

  static constexpr float kMinValidC = -55.0f;
  static constexpr float kMaxValidC = 125.0f;
  static constexpr float kPowerOnResetC = 85.0f;
  static constexpr float kPowerOnContinuityC = 5.0f;  ///< 85 °C is plausible within this distance
  static constexpr uint8_t kMaxPowerOnHolds = 3;      ///< Consecutive 85 °C reads that may hold the previous value

  Action apply(float raw) {
    if (std::isnan(raw) || raw < kMinValidC || raw > kMaxValidC) {
      reset();
      return Action::REJECT;
    }
    if (raw == kPowerOnResetC) {
      const bool continuesPrevious =
        !std::isnan(lastAccepted_) && std::fabs(lastAccepted_ - kPowerOnResetC) <= kPowerOnContinuityC;
      if (!continuesPrevious) {
        if (!std::isnan(lastAccepted_) && powerOnReads_ < kMaxPowerOnHolds) {
          powerOnReads_++;
          return Action::HOLD;
        }
        // The held value is too old to trust: drop it so a later 85 °C
        // cannot be accepted as continuing it.
        reset();
        return Action::UNCONFIRMED;
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
