#pragma once

#include <cstdint>

namespace PoolController {
    /**
     * MQTT Protocol types supported by the controller
     */
    enum class MQTTProtocol : std::uint8_t {
        HOMIE = 0,           // Homie convention (default)
        HOME_ASSISTANT = 1   // Home Assistant MQTT Discovery
    };

    /**
     * MQTT Configuration structure
     */
    struct MQTTConfig {
        MQTTProtocol protocol;
        
        MQTTConfig() : protocol(MQTTProtocol::HOMIE) {}
        
        const char* getProtocolName() const {
            switch(protocol) {
                case MQTTProtocol::HOME_ASSISTANT:
                    return "homeassistant";
                case MQTTProtocol::HOMIE:
                default:
                    return "homie";
            }
        }
    };
}
