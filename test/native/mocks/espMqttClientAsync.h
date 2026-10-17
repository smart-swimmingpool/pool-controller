#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>
#include "Arduino.h"

// Minimal stand-in for bertmelis/espMqttClient's espMqttClientAsync.
// Only the parts NetworkManager's test double uses are modelled.

namespace espMqttClientTypes {
enum class DisconnectReason : uint8_t {
  USER_OK = 0,
  MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
  MQTT_IDENTIFIER_REJECTED = 2,
  MQTT_SERVER_UNAVAILABLE = 3,
  MQTT_MALFORMED_CREDENTIALS = 4,
  MQTT_NOT_AUTHORIZED = 5,
  TLS_BAD_FINGERPRINT = 6,
  TCP_DISCONNECTED = 7
};
}  // namespace espMqttClientTypes

// Message callback as NetworkManager::MqttMessageCallback forwards it.
using MqttMessageHandler = std::function<void(const char *, const char *, size_t, size_t, size_t)>;

// MQTT message record for test assertions
struct MqttMessage {
  std::string topic;
  std::string payload;
  uint8_t qos;
  bool retained;
};

// Test capture globals
struct MqttClientCapture {
  std::string lastServerHost;
  uint16_t lastServerPort = 1883;
  std::string lastClientId;
  std::string lastUsername;
  std::string lastPassword;
  std::string lastWillTopic;
  std::string lastWillPayload;
  uint16_t lastKeepAlive = 15;
  std::vector<MqttMessage> published;
  std::vector<std::string> subscribed;
  bool connected = false;
  std::function<void(bool)> onConnectCb;
  std::function<void(espMqttClientTypes::DisconnectReason)> onDisconnectCb;
  MqttMessageHandler onMessageCb;

  void clear() {
    lastServerHost.clear();
    lastServerPort = 1883;
    lastClientId.clear();
    lastUsername.clear();
    lastPassword.clear();
    lastWillTopic.clear();
    lastWillPayload.clear();
    lastKeepAlive = 15;
    published.clear();
    subscribed.clear();
    connected = false;
    onConnectCb = nullptr;
    onDisconnectCb = nullptr;
    onMessageCb = nullptr;
  }

  const MqttMessage *findPublished(const char *topic) const {
    for (const auto &m : published) {
      if (m.topic == topic)
        return &m;
    }
    return nullptr;
  }
};

extern MqttClientCapture mqttCapture;

class espMqttClientAsync {
public:
  espMqttClientAsync() {}

  void setServer(const char *host, uint16_t port) {
    mqttCapture.lastServerHost = host;
    mqttCapture.lastServerPort = port;
  }

  void setCredentials(const char *username, const char *password = nullptr) {
    mqttCapture.lastUsername = username ? username : "";
    mqttCapture.lastPassword = password ? password : "";
  }

  void setClientId(const char *id) { mqttCapture.lastClientId = id; }

  void setKeepAlive(uint16_t seconds) { mqttCapture.lastKeepAlive = seconds; }

  void setWill(const char *topic, uint8_t qos, bool retain, const char *payload) {
    mqttCapture.lastWillTopic = topic;
    mqttCapture.lastWillPayload = payload;
  }

  void connect() {
    mqttCapture.connected = true;
    if (mqttCapture.onConnectCb) {
      mqttCapture.onConnectCb(false);
    }
  }

  void disconnect() { mqttCapture.connected = false; }

  uint16_t publish(const char *topic, uint8_t qos, bool retained, const char *payload) {
    MqttMessage msg;
    msg.topic = topic;
    msg.payload = payload ? payload : "";
    msg.qos = qos;
    msg.retained = retained;
    mqttCapture.published.push_back(msg);
    return 1;
  }

  uint16_t publish(const char *topic, uint8_t qos, bool retained, const String &payload) {
    return publish(topic, qos, retained, payload.c_str());
  }

  uint16_t subscribe(const char *topic, uint8_t qos = 0) {
    mqttCapture.subscribed.push_back(topic);
    return 1;
  }

  void onConnect(std::function<void(bool sessionPresent)> callback) { mqttCapture.onConnectCb = callback; }

  void onDisconnect(std::function<void(espMqttClientTypes::DisconnectReason reason)> callback) {
    mqttCapture.onDisconnectCb = callback;
  }

  void onMessage(MqttMessageHandler callback) { mqttCapture.onMessageCb = callback; }

  // Test helper: simulate an incoming message
  void simulateMessage(const char *topic, const char *payload) {
    if (mqttCapture.onMessageCb) {
      size_t len = strlen(payload);
      // The real client may deliver chunks; tests deliver a single chunk.
      mqttCapture.onMessageCb(topic, payload, len, 0, len);
    }
  }
};
