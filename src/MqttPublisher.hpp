// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file MqttPublisher.hpp
 * @brief Home Assistant MQTT Discovery — publishes entities and telemetry.
 */

#pragma once

#include <Arduino.h>

namespace PoolController {

/**
 * @brief Publishes Home Assistant MQTT Discovery configs and telemetry states.
 *
 * On startup and MQTT (re)connect, announces all entities via HA Discovery
 * topics. Periodically publishes sensor values and relay states. Handles
 * incoming MQTT commands (mode, pump toggle, config changes).
 */
class MqttPublisher {
public:
  MqttPublisher() = default;

  /** @brief Initialize MQTT publisher — set up message callback and subscribe to command topics. */
  static void begin();
  /** @brief Publish Home Assistant MQTT Discovery configs for all entities. */
  static void publishDiscovery();
  /** @brief Publish current telemetry states (temperatures, relays, mode). */
  static void publishStates();
  /**
   * @brief Handle an incoming MQTT message from Home Assistant.
   * @param topic   The MQTT topic the message was received on.
   * @param payload Raw payload bytes.
   * @param length  Payload length in bytes.
   */
  static void handleMqttMessage(char *topic, uint8_t *payload, unsigned int length);

private:
  static void publishTextDiscovery(const char *objectId, const char *name, const char *icon = nullptr);
  static void publishSensorDiscovery(const char *objectId, const char *name, const char *deviceClass = nullptr,
    const char *unit = nullptr, const char *icon = nullptr, const char *entityCategory = nullptr);
  static void publishSwitchDiscovery(const char *objectId, const char *name, const char *icon = nullptr,
    const char *entityCategory = nullptr);
  static void publishSelectDiscovery(
    const char *objectId, const char *name, const char *const *options, size_t optionCount, const char *icon = nullptr,
    const char *entityCategory = nullptr);
  static void publishNumberDiscovery(const char *objectId, const char *name, double minVal, double maxVal, double step,
    const char *unit = nullptr, const char *icon = nullptr, const char *entityCategory = nullptr);
  static void publishTimeDiscovery(const char *objectId, const char *name, const char *icon = nullptr,
    const char *entityCategory = nullptr);
  static void publishUpdateDiscovery();
  static void publishClimateDiscovery();

  static String getBaseTopic(const char *component, const char *objectId);
  static String getDeviceJson();
  static void publishUpdateState();
  static void publishClimateState();

  static String deviceId_;
};

}  // namespace PoolController
