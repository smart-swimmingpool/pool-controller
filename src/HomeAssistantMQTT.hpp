#pragma once

/**
 * Home Assistant MQTT Discovery Support
 * 
 * This module provides Home Assistant auto-discovery functionality
 * as an alternative to the Homie convention.
 * 
 * Discovery format: homeassistant/<component>/<node_id>/<object_id>/config
 * Example: homeassistant/sensor/pool-controller/pool-temp/config
 * 
 * Memory requirements:
 * - Sensor discovery: ~400-450 bytes JSON payload
 * - Switch discovery: ~450-500 bytes JSON payload
 * - Buffer size: 512 bytes (with safety margin)
 */

#include <Homie.h>
#include <ArduinoJson.h>

namespace PoolController {
namespace HomeAssistant {

    /**
     * Base class for Home Assistant MQTT Discovery
     */
    class DiscoveryPublisher {
    public:
        /**
         * Publish a sensor discovery message
         * @note Uses ~400 bytes of JSON, buffer is 512 bytes
         */
        static bool publishSensor(
            const char* nodeId,
            const char* objectId, 
            const char* name,
            const char* deviceClass = nullptr,
            const char* unitOfMeasurement = nullptr,
            const char* icon = nullptr
        ) {
            if (!Homie.isConnected()) return false;

            char topic[128];
            snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/%s/config", nodeId, objectId);

            JsonDocument doc;
            
            // State topic
            char stateTopic[128];
            snprintf(stateTopic, sizeof(stateTopic), "homeassistant/sensor/%s/%s/state", nodeId, objectId);
            doc["state_topic"] = stateTopic;
            
            // Name and unique ID
            doc["name"] = name;
            char uniqueId[96];
            snprintf(uniqueId, sizeof(uniqueId), "%s_%s", nodeId, objectId);
            doc["unique_id"] = uniqueId;
            
            // Optional attributes
            if (deviceClass) doc["device_class"] = deviceClass;
            if (unitOfMeasurement) doc["unit_of_measurement"] = unitOfMeasurement;
            if (icon) doc["icon"] = icon;
            
            // Device information
            JsonObject device = doc["device"].to<JsonObject>();
            device["identifiers"][0] = nodeId;
            device["name"] = "Pool Controller";
            device["manufacturer"] = "smart-swimmingpool";
            device["model"] = "Pool Controller 2.0";
            
            char buffer[512];
            size_t len = serializeJson(doc, buffer, sizeof(buffer));
            
            // Check for truncation
            if (len >= sizeof(buffer) - 1) {
                Homie.getLogger() << F("✖ Warning: JSON buffer too small, message truncated") << endl;
                return false;
            }
            
            return Homie.getMqttClient().publish(topic, 1, true, buffer, len);
        }

        /**
         * Publish a switch discovery message
         * @note Uses ~450 bytes of JSON, buffer is 512 bytes
         */
        static bool publishSwitch(
            const char* nodeId,
            const char* objectId,
            const char* name,
            const char* icon = nullptr
        ) {
            if (!Homie.isConnected()) return false;

            char topic[128];
            snprintf(topic, sizeof(topic), "homeassistant/switch/%s/%s/config", nodeId, objectId);

            JsonDocument doc;
            
            // State and command topics
            char stateTopic[128];
            char commandTopic[128];
            snprintf(stateTopic, sizeof(stateTopic), "homeassistant/switch/%s/%s/state", nodeId, objectId);
            snprintf(commandTopic, sizeof(commandTopic), "homeassistant/switch/%s/%s/set", nodeId, objectId);
            
            doc["state_topic"] = stateTopic;
            doc["command_topic"] = commandTopic;
            
            // Name and unique ID
            doc["name"] = name;
            char uniqueId[96];
            snprintf(uniqueId, sizeof(uniqueId), "%s_%s", nodeId, objectId);
            doc["unique_id"] = uniqueId;
            
            // Payloads
            doc["payload_on"] = "ON";
            doc["payload_off"] = "OFF";
            doc["state_on"] = "ON";
            doc["state_off"] = "OFF";
            
            if (icon) doc["icon"] = icon;
            
            // Device information
            JsonObject device = doc["device"].to<JsonObject>();
            device["identifiers"][0] = nodeId;
            device["name"] = "Pool Controller";
            device["manufacturer"] = "smart-swimmingpool";
            device["model"] = "Pool Controller 2.0";
            
            char buffer[512];
            size_t len = serializeJson(doc, buffer, sizeof(buffer));
            
            // Check for truncation
            if (len >= sizeof(buffer) - 1) {
                Homie.getLogger() << F("✖ Warning: JSON buffer too small, message truncated") << endl;
                return false;
            }
            
            return Homie.getMqttClient().publish(topic, 1, true, buffer, len);
        }

        /**
         * Publish state for a sensor
         */
        static bool publishSensorState(const char* nodeId, const char* objectId, const char* value) {
            if (!Homie.isConnected()) return false;
            
            char topic[128];
            snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/%s/state", nodeId, objectId);
            
            return Homie.getMqttClient().publish(topic, 1, true, value);
        }

        /**
         * Publish state for a switch
         */
        static bool publishSwitchState(const char* nodeId, const char* objectId, bool state) {
            if (!Homie.isConnected()) return false;
            
            char topic[128];
            snprintf(topic, sizeof(topic), "homeassistant/switch/%s/%s/state", nodeId, objectId);
            
            return Homie.getMqttClient().publish(topic, 1, true, state ? "ON" : "OFF");
        }
    };

} // namespace HomeAssistant
} // namespace PoolController
