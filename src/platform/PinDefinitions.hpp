/**
 * Platform-specific pin definitions for Smart Swimming Pool Controller
 * 
 * This header provides a unified interface for pin definitions across
 * different platforms (ESP32, ESP8266, etc.)
 */

#pragma once

#include <cstdint>

// Pin definitions for different platforms
#ifdef ESP32

namespace PlatformPins {
    // Temperature sensor pins
    static constexpr uint8_t DS_SOLAR = 15;  // Pin of Temp-Sensor Solar
    static constexpr uint8_t DS_POOL  = 16;  // Pin of Temp-Sensor Pool

    // Relay pins
    static constexpr uint8_t RELAY_POOL  = 18;
    static constexpr uint8_t RELAY_SOLAR = 19;
}

#elif defined(ESP8266)

// see: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
namespace PlatformPins {
    // Temperature sensor pins
    static constexpr uint8_t DS_SOLAR = D5;  // Pin of Temp-Sensor Solar
    static constexpr uint8_t DS_POOL  = D6;  // Pin of Temp-Sensor Pool

    // Relay pins
    static constexpr uint8_t RELAY_POOL  = D1;
    static constexpr uint8_t RELAY_SOLAR = D2;
}

#else
#error "Unsupported platform. Please define ESP32 or ESP8266."
#endif

// Default measurement intervals (in seconds)
static constexpr uint8_t TEMP_READ_INTERVAL = 30;
