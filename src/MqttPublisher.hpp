// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>

namespace PoolController {

class MqttPublisher {
public:
  MqttPublisher() = default;

  static void begin();
  static void publishDiscovery();
  static void publishStates();
  static void handleMqttMessage(char *topic, uint8_t *payload, unsigned int length);

private:
  static void publishSensorDiscovery(const char *objectId, const char *name, const char *deviceClass = nullptr,
    const char *unit = nullptr, const char *icon = nullptr);
  static void publishSwitchDiscovery(const char *objectId, const char *name, const char *icon = nullptr);
  static void publishSelectDiscovery(
    const char *objectId, const char *name, const char *const *options, size_t optionCount, const char *icon = nullptr);
  static void publishNumberDiscovery(const char *objectId, const char *name, double minVal, double maxVal, double step,
    const char *unit = nullptr, const char *icon = nullptr);
  static void publishUpdateDiscovery();

  static String getBaseTopic(const char *component, const char *objectId);
  static String getDeviceJson();
  static void publishUpdateState();

  static String deviceId_;
};

}  // namespace PoolController
