/**
 * Pin definitions for Smart Swimming Pool Controller (ESP32 only)
 * 
 * This project now focuses on ESP32 platform only.
 */

#pragma once

#include <cstdint>

// ESP32 pin definitions
namespace PlatformPins {
    // Temperature sensor pins
    static constexpr uint8_t DS_SOLAR = 15;  // Pin of Temp-Sensor Solar
    static constexpr uint8_t DS_POOL  = 16;  // Pin of Temp-Sensor Pool

    // Relay pins
    static constexpr uint8_t RELAY_POOL  = 18;
    static constexpr uint8_t RELAY_SOLAR = 19;
}

// Default measurement intervals (in seconds)
static constexpr uint8_t TEMP_READ_INTERVAL = 30;
