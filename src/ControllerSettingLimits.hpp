// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerSettingLimits.hpp
 * @brief Single source of the accepted ranges for controller settings.
 *
 * The command handler validates against these ranges and the MQTT discovery
 * advertises the same bounds to Home Assistant, so both stay in sync.
 */

#pragma once

namespace PoolController {

/** @brief Inclusive range of accepted values for one setting. */
struct SettingRange final {
  double min;
  double max;

  constexpr bool contains(double value) const { return value >= min && value <= max; }
};

namespace SettingLimits {

constexpr SettingRange kPoolMaxTemperature{0.0, 40.0};        ///< °C
constexpr SettingRange kSolarMinTemperature{0.0, 100.0};      ///< °C
constexpr SettingRange kTemperatureHysteresis{0.0, 10.0};     ///< K
constexpr SettingRange kCirculationThreshold{0.0, 40.0};      ///< °C
constexpr SettingRange kCirculationFactor{0.0, 120.0};        ///< extra minutes per °C
constexpr SettingRange kCirculationMaxRuntime{60.0, 1440.0};  ///< minutes
constexpr SettingRange kLoopInterval{1.0, 3600.0};            ///< seconds
constexpr SettingRange kTimeLossGreenHours{1.0, 71.0};        ///< must also stay below red
constexpr SettingRange kTimeLossRedHours{2.0, 72.0};          ///< must also stay above green
constexpr SettingRange kButtonAdc{0.0, 4096.0};               ///< 4096 = "no press" sentinel above ADC max

}  // namespace SettingLimits

}  // namespace PoolController
