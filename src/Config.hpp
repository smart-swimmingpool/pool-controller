#pragma once

#include <cstdint>

namespace PoolController
{
    enum struct ControllerType : std::uint8_t
    {
        CT_ESP32 = 0,
        CT_ESP8266
    };

    constexpr ControllerType CurrentControllerType
    {
        #ifdef ESP32
            ControllerType::CT_ESP32
        #elif defined(ESP8266)
            ControllerType::CT_ESP8266
        #else
        #   error "Unknown platform"
        #endif
    };

    #ifdef ESP32
        constexpr std::uint8_t D0  { 16 };
        constexpr std::uint8_t D1  { 5 };
        constexpr std::uint8_t D2  { 4 };
        constexpr std::uint8_t D3  { 0 };
        constexpr std::uint8_t D4  { 2 };
        constexpr std::uint8_t D5  { 14 };
        constexpr std::uint8_t D6  { 12 };
        constexpr std::uint8_t D7  { 13 };
        constexpr std::uint8_t D8  { 15 };
        constexpr std::uint8_t D9  { 3 };
        constexpr std::uint8_t D10 { 1 };
    #endif

    /**
     * Interval to temp updates.
    */
    constexpr std::uint8_t TEMP_READ_INTERVALL { 30 };

    /**
     * Pin of Temp-Sensor Solar
    */
    constexpr std::uint8_t PIN_DS_SOLAR
    {
        CurrentControllerType == ControllerType::CT_ESP32 ? 15 : D5
    };

    /**
     * Pin of Temp-Sensor Pool
    */
    constexpr std::uint8_t PIN_DS_POOL
    {
        CurrentControllerType == ControllerType::CT_ESP32 ? 16 : D6
    };

    constexpr std::uint8_t PIN_RELAY_POOL
    {
        CurrentControllerType == ControllerType::CT_ESP32 ? 18 : D1
    };

    constexpr std::uint8_t PIN_RELAY_SOLAR
    {
        CurrentControllerType == ControllerType::CT_ESP32 ? 19 : D2
    };

    #ifdef MOD_PROBE
        static_assert(sizeof(unsigned long int) == sizeof(std::uint32_t) && alignof(unsigned long int) == alignof(std::uint32_t), "Arch check failed");
        static_assert(sizeof(unsigned long int) == sizeof(std::uint32_t) && alignof(unsigned long int) == alignof(std::uint32_t), "Arch check failed");
        static_assert(sizeof(void*) == sizeof(std::uint32_t), "Pointer check failed");
    #endif
}
