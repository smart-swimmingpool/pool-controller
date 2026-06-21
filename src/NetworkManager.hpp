// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file NetworkManager.hpp
 * @brief WiFi and MQTT connection management with AP fallback.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <functional>
#include <AsyncMqttClient.h>

namespace PoolController {

/**
 * @brief Manages WiFi connectivity and MQTT broker connection.
 *
 * Implements a state machine: connects to the configured WiFi network,
 * falls back to AP mode with captive portal if no credentials are stored
 * or connection fails, and maintains the MQTT connection with automatic
 * retry. Supports secure MQTTS via WiFiClientSecure.
 */
class NetworkManager {
public:
  using MqttMessageCallback = std::function<void(
    char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)>;

  NetworkManager() = default;

  /** @brief Initialize WiFi + MQTT. Attempts STA mode, falls back to AP. @return true if WiFi connected. */
  static bool begin();
  /** @brief Maintain WiFi and MQTT connections with automatic retry. */
  static void loop();

  /** @brief Check if WiFi is connected to the configured network (or AP). */
  static bool isWiFiConnected();
  /** @brief Check if MQTT broker connection is established. */
  static bool isMqttConnected();
  /** @brief Check if the device is in access-point mode (no WiFi config). */
  static bool isApMode();

  /** @brief Publish an MQTT message. @param retained  Set retained flag on the message. */
  static bool publish(const char *topic, const char *payload, bool retained = false);
  /** @brief Subscribe to an MQTT topic. */
  static bool subscribe(const char *topic);

  /** @brief Register the MQTT message callback. */
  static void setMqttCallback(MqttMessageCallback callback);
  /** @brief Switch to access-point mode (captive portal). */
  static void startAPMode();
  /** @brief Disconnect from the MQTT broker. */
  static void disconnectMqtt();

  // Getters for status monitoring
  static int getWiFiRSSI() { return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0; }
  static String getLocalIP() { return WiFi.localIP().toString(); }

private:
  static void handleWiFiEvent(WiFiEvent_t event);
  static void connectWiFi();
  static void connectMqtt();

  static AsyncMqttClient mqttClient_;

  static MqttMessageCallback mqttCallback_;

  static bool apModeActive_;
  static bool mdnsRunning_;
  static uint32_t lastWiFiRetryTime_;
  static uint32_t lastMqttRetryTime_;
  static uint32_t connectionStartTime_;

  static constexpr uint32_t kWiFiRetryIntervalMs = 5000;
  static constexpr uint32_t kMqttRetryIntervalMs = 5000;
};

}  // namespace PoolController
