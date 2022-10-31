#pragma once

#include <cstdint>

namespace PoolController
{
    /**
     * Interval to temp updates.
    */
    constexpr std::uint8_t TEMP_READ_INTERVALL { 30 };

    /**
     * Pin of Temp-Sensor Solar
    */
    constexpr std::uint8_t PIN_DS_SOLAR
    {
        #ifdef ESP32
            15
        #elif defined(ESP8266)
            D5
        #else
        #   error "Unknown platform"
        #endif
    };

    /**
     * Pin of Temp-Sensor Pool
    */
    constexpr std::uint8_t PIN_DS_POOL
    {
        #ifdef ESP32
            16
        #elif defined(ESP8266)
            D6
        #else
        #   error "Unknown platform"
        #endif
    };

    constexpr std::uint8_t PIN_RELAY_POOL
    {
        #ifdef ESP32
            18
        #elif defined(ESP8266)
            D1
        #else
        #   error "Unknown platform"
        #endif
    };

    constexpr std::uint8_t PIN_RELAY_SOLAR
    {
        #ifdef ESP32
            19
        #elif defined(ESP8266)
            D2
        #else
        #   error "Unknown platform"
        #endif
    };


    #ifdef MOD_PROBE
        static_assert(sizeof(unsigned long int) == sizeof(std::uint32_t) && alignof(unsigned long int) == alignof(std::uint32_t), "Arch check failed");
        static_assert(sizeof(unsigned long int) == sizeof(std::uint32_t) && alignof(unsigned long int) == alignof(std::uint32_t), "Arch check failed");
        static_assert(sizeof(void*) == sizeof(std::uint32_t), "Pointer check failed");
    #endif
}
