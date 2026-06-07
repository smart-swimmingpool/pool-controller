// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Config.hpp
 * @brief Pin assignments and compile-time constants for the Pool Controller.
 *
 * All GPIO pin assignments and tunable constants are centralized here.
 * Changing these values requires rebuilding the firmware.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Interval for temperature sensor updates (seconds).
 *
 * @note SERIAL_SPEED (baud rate) is provided as a compiler macro from
 *       platformio.ini build flags, not defined here.
 */

constexpr std::uint8_t TEMP_READ_INTERVAL{30};

/** @brief DS18B20 data pin — solar collector temperature sensor. */
constexpr std::uint8_t PIN_DS_SOLAR{15};
/** @brief DS18B20 data pin — pool water temperature sensor. */
constexpr std::uint8_t PIN_DS_POOL{16};
/** @brief Relay control pin — pool circulation pump. */
constexpr std::uint8_t PIN_RELAY_POOL{18};
/** @brief Relay control pin — solar heating pump. */
constexpr std::uint8_t PIN_RELAY_SOLAR{19};

}  // namespace PoolController
