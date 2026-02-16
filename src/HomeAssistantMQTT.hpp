// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

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
#include <cstdio>

namespace PoolController {
namespace HomeAssistant {

// Global flag to track whether Home Assistant mode is active
extern bool useHomeAssistant;

/**
 * Base class for Home Assistant MQTT Discovery
 */
class DiscoveryPublisher {
public:
  /**
   * Publish a sensor discovery message
   * @note Uses ~400 bytes of JSON, buffer is 512 bytes
   */
  static bool publishSensor(const char* nodeId, const char* objectId, const char* name, const char* deviceClass = nullptr,
                            const char* unitOfMeasurement = nullptr, const char* icon = nullptr) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/%s/config", nodeId, objectId);

    StaticJsonDocument<1024> doc;

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
    if (deviceClass)
      doc["device_class"] = deviceClass;
    if (unitOfMeasurement)
      doc["unit_of_measurement"] = unitOfMeasurement;
    if (icon)
      doc["icon"] = icon;

    // Device information
    JsonObject device        = doc["device"].to<JsonObject>();
    device["identifiers"][0] = nodeId;
    device["name"]           = "Pool Controller";
    device["manufacturer"]   = "smart-swimmingpool";
    device["model"]          = "Pool Controller 2.0";

    char   buffer[512];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    // Check for truncation
    if (len >= sizeof(buffer) - 1) {
      Homie.getLogger() << F("✖ Warning: JSON buffer too small, "
                             "message truncated")
                        << endl;
      return false;
    }

    return Homie.getMqttClient().publish(topic, 1, true, buffer, len);
  }

  /**
   * Publish a switch discovery message
   * @note Uses ~450 bytes of JSON, buffer is 512 bytes
   */
  static bool publishSwitch(const char* nodeId, const char* objectId, const char* name, const char* icon = nullptr) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/switch/%s/%s/config", nodeId, objectId);

    StaticJsonDocument<1024> doc;

    // State and command topics
    char stateTopic[128];
    char commandTopic[128];
    snprintf(stateTopic, sizeof(stateTopic), "homeassistant/switch/%s/%s/state", nodeId, objectId);
    snprintf(commandTopic, sizeof(commandTopic), "homeassistant/switch/%s/%s/set", nodeId, objectId);

    doc["state_topic"]   = stateTopic;
    doc["command_topic"] = commandTopic;

    // Name and unique ID
    doc["name"] = name;
    char uniqueId[96];
    snprintf(uniqueId, sizeof(uniqueId), "%s_%s", nodeId, objectId);
    doc["unique_id"] = uniqueId;

    // Payloads
    doc["payload_on"]  = "ON";
    doc["payload_off"] = "OFF";
    doc["state_on"]    = "ON";
    doc["state_off"]   = "OFF";

    if (icon)
      doc["icon"] = icon;

    // Device information
    JsonObject device        = doc["device"].to<JsonObject>();
    device["identifiers"][0] = nodeId;
    device["name"]           = "Pool Controller";
    device["manufacturer"]   = "smart-swimmingpool";
    device["model"]          = "Pool Controller 2.0";

    char   buffer[512];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    // Check for truncation
    if (len >= sizeof(buffer) - 1) {
      Homie.getLogger() << F("✖ Warning: JSON buffer too small, "
                             "message truncated")
                        << endl;
      return false;
    }

    return Homie.getMqttClient().publish(topic, 1, true, buffer, len);
  }

  /**
   * Publish a number discovery message
   */
  static bool publishNumber(const char* nodeId, const char* objectId, const char* name,
                            double minValue, double maxValue, double step,
                            const char* unitOfMeasurement = nullptr,
                            const char* icon = nullptr,
                            const char* mode = nullptr) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/number/%s/%s/config", nodeId, objectId);

    StaticJsonDocument<1024> doc;

    char stateTopic[128];
    char commandTopic[128];
    snprintf(stateTopic, sizeof(stateTopic), "homeassistant/number/%s/%s/state", nodeId, objectId);
    snprintf(commandTopic, sizeof(commandTopic), "homeassistant/number/%s/%s/set", nodeId, objectId);

    doc["state_topic"] = stateTopic;
    doc["command_topic"] = commandTopic;

    doc["name"] = name;
    char uniqueId[96];
    snprintf(uniqueId, sizeof(uniqueId), "%s_%s", nodeId, objectId);
    doc["unique_id"] = uniqueId;

    doc["min"] = minValue;
    doc["max"] = maxValue;
    doc["step"] = step;

    if (unitOfMeasurement)
      doc["unit_of_measurement"] = unitOfMeasurement;
    if (icon)
      doc["icon"] = icon;
    if (mode)
      doc["mode"] = mode;

    JsonObject device        = doc["device"].to<JsonObject>();
    device["identifiers"][0] = nodeId;
    device["name"]           = "Pool Controller";
    device["manufacturer"]   = "smart-swimmingpool";
    device["model"]          = "Pool Controller 2.0";

    char   buffer[512];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    if (len >= sizeof(buffer) - 1) {
      Homie.getLogger() << F("✖ Warning: JSON buffer too small, "
                             "message truncated")
                        << endl;
      return false;
    }

    return Homie.getMqttClient().publish(topic, 1, true, buffer, len);
  }

  /**
   * Publish a select discovery message
   */
  static bool publishSelect(const char* nodeId, const char* objectId, const char* name,
                            const char* const* options, size_t optionCount,
                            const char* icon = nullptr) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/select/%s/%s/config", nodeId, objectId);

    StaticJsonDocument<1024> doc;

    char stateTopic[128];
    char commandTopic[128];
    snprintf(stateTopic, sizeof(stateTopic), "homeassistant/select/%s/%s/state", nodeId, objectId);
    snprintf(commandTopic, sizeof(commandTopic), "homeassistant/select/%s/%s/set", nodeId, objectId);

    doc["state_topic"] = stateTopic;
    doc["command_topic"] = commandTopic;

    doc["name"] = name;
    char uniqueId[96];
    snprintf(uniqueId, sizeof(uniqueId), "%s_%s", nodeId, objectId);
    doc["unique_id"] = uniqueId;

    JsonArray optionsArray = doc["options"].to<JsonArray>();
    for (size_t i = 0; i < optionCount; ++i) {
      optionsArray.add(options[i]);
    }

    if (icon)
      doc["icon"] = icon;

    JsonObject device        = doc["device"].to<JsonObject>();
    device["identifiers"][0] = nodeId;
    device["name"]           = "Pool Controller";
    device["manufacturer"]   = "smart-swimmingpool";
    device["model"]          = "Pool Controller 2.0";

    char   buffer[512];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    if (len >= sizeof(buffer) - 1) {
      Homie.getLogger() << F("✖ Warning: JSON buffer too small, "
                             "message truncated")
                        << endl;
      return false;
    }

    return Homie.getMqttClient().publish(topic, 1, true, buffer, len);
  }

  /**
   * Publish state for a sensor
   */
  static bool publishSensorState(const char* nodeId, const char* objectId, const char* value) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/%s/state", nodeId, objectId);

    return Homie.getMqttClient().publish(topic, 1, true, value);
  }

  /**
   * Publish state for a switch
   */
  static bool publishSwitchState(const char* nodeId, const char* objectId, bool state) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/switch/%s/%s/state", nodeId, objectId);

    return Homie.getMqttClient().publish(topic, 1, true, state ? "ON" : "OFF");
  }

  /**
   * Publish state for a number
   */
  static bool publishNumberState(const char* nodeId, const char* objectId, const char* value) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/number/%s/%s/state", nodeId, objectId);

    return Homie.getMqttClient().publish(topic, 1, true, value);
  }

  /**
   * Publish state for a select
   */
  static bool publishSelectState(const char* nodeId, const char* objectId, const char* value) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/select/%s/%s/state", nodeId, objectId);

    return Homie.getMqttClient().publish(topic, 1, true, value);
  }

  /**
   * Subscribe to switch command topic
   */
  static bool subscribeSwitch(const char* nodeId, const char* objectId) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/switch/%s/%s/set", nodeId, objectId);

    return Homie.getMqttClient().subscribe(topic, 1);
  }

  /**
   * Subscribe to number command topic
   */
  static bool subscribeNumber(const char* nodeId, const char* objectId) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/number/%s/%s/set", nodeId, objectId);

    return Homie.getMqttClient().subscribe(topic, 1);
  }

  /**
   * Subscribe to select command topic
   */
  static bool subscribeSelect(const char* nodeId, const char* objectId) {
    if (!Homie.isConnected())
      return false;

    char topic[128];
    snprintf(topic, sizeof(topic), "homeassistant/select/%s/%s/set", nodeId, objectId);

    return Homie.getMqttClient().subscribe(topic, 1);
  }

  /**
   * Get command topic for switch
   */
  static void getSwitchCommandTopic(char* buffer, size_t bufferSize, const char* nodeId, const char* objectId) {
    snprintf(buffer, bufferSize, "homeassistant/switch/%s/%s/set", nodeId, objectId);
  }
};

}  // namespace HomeAssistant
}  // namespace PoolController
