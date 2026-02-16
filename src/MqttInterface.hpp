// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Homie.hpp>

#include "HomeAssistantMQTT.hpp"

namespace PoolController {
namespace MqttInterface {

constexpr const char* kDeviceId = "pool-controller";

inline bool isHomeAssistant() {
  return HomeAssistant::useHomeAssistant;
}

inline void publishHomieProperty(HomieNode& node, const char* property, const char* value) {
  if (!isHomeAssistant()) {
    node.setProperty(property).send(value);
  }
}

inline void publishSensorDiscovery(const char* objectId, const char* name,
                                   const char* deviceClass = nullptr,
                                   const char* unitOfMeasurement = nullptr,
                                   const char* icon = nullptr) {
  if (!isHomeAssistant()) {
    return;
  }
  HomeAssistant::DiscoveryPublisher::publishSensor(
      kDeviceId, objectId, name, deviceClass, unitOfMeasurement, icon);
}

inline void publishSwitchDiscovery(const char* objectId, const char* name,
                                   const char* icon = nullptr) {
  if (!isHomeAssistant()) {
    return;
  }
  HomeAssistant::DiscoveryPublisher::publishSwitch(kDeviceId, objectId, name, icon);
}

inline void publishNumberDiscovery(const char* objectId, const char* name,
                                   double minValue, double maxValue, double step,
                                   const char* unitOfMeasurement = nullptr,
                                   const char* icon = nullptr,
                                   const char* mode = nullptr) {
  if (!isHomeAssistant()) {
    return;
  }
  HomeAssistant::DiscoveryPublisher::publishNumber(
      kDeviceId, objectId, name, minValue, maxValue, step,
      unitOfMeasurement, icon, mode);
}

inline void publishSelectDiscovery(const char* objectId, const char* name,
                                   const char* const* options, size_t optionCount,
                                   const char* icon = nullptr) {
  if (!isHomeAssistant()) {
    return;
  }
  HomeAssistant::DiscoveryPublisher::publishSelect(
      kDeviceId, objectId, name, options, optionCount, icon);
}

inline void publishSensorState(HomieNode& node, const char* homieProperty,
                               const char* objectId, const char* value) {
  if (isHomeAssistant()) {
    HomeAssistant::DiscoveryPublisher::publishSensorState(kDeviceId, objectId, value);
  } else {
    node.setProperty(homieProperty).send(value);
  }
}

inline void publishTextState(HomieNode& node, const char* homieProperty,
                             const char* objectId, const char* value) {
  publishSensorState(node, homieProperty, objectId, value);
}

inline void publishSwitchState(HomieNode& node, const char* homieProperty,
                               const char* objectId, bool state) {
  if (isHomeAssistant()) {
    HomeAssistant::DiscoveryPublisher::publishSwitchState(kDeviceId, objectId, state);
  } else {
    node.setProperty(homieProperty).send(state ? "true" : "false");
  }
}

inline void publishNumberState(HomieNode& node, const char* homieProperty,
                               const char* objectId, const char* value) {
  if (isHomeAssistant()) {
    HomeAssistant::DiscoveryPublisher::publishNumberState(kDeviceId, objectId, value);
  } else {
    node.setProperty(homieProperty).send(value);
  }
}

inline void publishSelectState(HomieNode& node, const char* homieProperty,
                               const char* objectId, const char* value) {
  if (isHomeAssistant()) {
    HomeAssistant::DiscoveryPublisher::publishSelectState(kDeviceId, objectId, value);
  } else {
    node.setProperty(homieProperty).send(value);
  }
}

inline void subscribeSwitch(const char* objectId) {
  if (!isHomeAssistant()) {
    return;
  }
  HomeAssistant::DiscoveryPublisher::subscribeSwitch(kDeviceId, objectId);
}

inline void subscribeNumber(const char* objectId) {
  if (!isHomeAssistant()) {
    return;
  }
  HomeAssistant::DiscoveryPublisher::subscribeNumber(kDeviceId, objectId);
}

inline void subscribeSelect(const char* objectId) {
  if (!isHomeAssistant()) {
    return;
  }
  HomeAssistant::DiscoveryPublisher::subscribeSelect(kDeviceId, objectId);
}

}  // namespace MqttInterface
}  // namespace PoolController
