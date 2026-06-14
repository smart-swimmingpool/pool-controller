#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>
#include "Arduino.h"

struct AsyncMqttClientMessageProperties {
  uint8_t qos = 0;
  bool retain = false;
  bool dup = false;
};

enum class AsyncMqttClientDisconnectReason : uint8_t {
  TCP_DISCONNECTED = 0,
  MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
  MQTT_IDENTIFIER_REJECTED = 2,
  MQTT_SERVER_UNAVAILABLE = 3,
  MQTT_MALFORMED_CREDENTIALS = 4,
  MQTT_NOT_AUTHORIZED = 5,
  TLS_BAD_FINGERPRINT = 6,
  TLS_BAD_CA_CERT = 7,
  TLS_CONNECTION_LOST = 8,
  TLS_CERTIFICATE_REQUIRED = 9,
  TLS_INVALID_CERTIFICATE = 10
};

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
  std::function<void(AsyncMqttClientDisconnectReason)> onDisconnectCb;
  std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> onMessageCb;

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
      if (m.topic == topic) return &m;
    }
    return nullptr;
  }
};

extern MqttClientCapture mqttCapture;

class AsyncMqttClient {
public:
  AsyncMqttClient() {}

  void setServer(const char *host, uint16_t port) {
    mqttCapture.lastServerHost = host;
    mqttCapture.lastServerPort = port;
  }

  void setCredentials(const char *username, const char *password = nullptr) {
    mqttCapture.lastUsername = username ? username : "";
    mqttCapture.lastPassword = password ? password : "";
  }

  void setClientId(const char *id) {
    mqttCapture.lastClientId = id;
  }

  void setKeepAlive(uint16_t seconds) {
    mqttCapture.lastKeepAlive = seconds;
  }

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

  void disconnect() {
    mqttCapture.connected = false;
  }

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

  void onConnect(std::function<void(bool sessionPresent)> callback) {
    mqttCapture.onConnectCb = callback;
  }

  void onDisconnect(std::function<void(AsyncMqttClientDisconnectReason reason)> callback) {
    mqttCapture.onDisconnectCb = callback;
  }

  void onMessage(std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> callback) {
    mqttCapture.onMessageCb = callback;
  }

  // Test helper: simulate an incoming message
  void simulateMessage(const char *topic, const char *payload) {
    if (mqttCapture.onMessageCb) {
      AsyncMqttClientMessageProperties props;
      size_t len = strlen(payload);
      // Note: AsyncMqttClient typically provides chunked delivery
      // For tests, we deliver as single chunk
      char *t = strdup(topic);
      char *p = strdup(payload);
      mqttCapture.onMessageCb(t, p, props, len, 0, len);
      free(t);
      free(p);
    }
  }
};
