#pragma once

#include <cstdint>

namespace PoolController {

/**
 * GPIO pin assignments for ESP32.
 * ESP8266 support was removed in v3.2.0.
 */

/** Interval for temperature updates (seconds). */
constexpr std::uint8_t TEMP_READ_INTERVAL{30};

/** Pin assignments */
constexpr std::uint8_t PIN_DS_SOLAR{15};
constexpr std::uint8_t PIN_DS_POOL{16};
constexpr std::uint8_t PIN_RELAY_POOL{18};
constexpr std::uint8_t PIN_RELAY_SOLAR{19};

}  // namespace PoolController
