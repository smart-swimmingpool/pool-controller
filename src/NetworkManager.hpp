// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

namespace PoolController {

class NetworkManager {
public:
  using MqttMessageCallback = void (*)(char* topic, uint8_t* payload, unsigned int length);

  NetworkManager() = default;

  static bool begin();
  static void loop();

  static bool isWiFiConnected();
  static bool isMqttConnected();
  static bool isApMode();

  static bool publish(const char* topic, const char* payload, bool retained = false);
  static bool subscribe(const char* topic);

  static void setMqttCallback(MqttMessageCallback callback);
  static void startAPMode();
  static void disconnectMqtt();

  // Getters for status monitoring
  static int getWiFiRSSI() { return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0; }
  static String getLocalIP() { return WiFi.localIP().toString(); }

private:
  static void handleWiFiEvent(WiFiEvent_t event);
  static void connectWiFi();
  static void connectMqtt();

  static WiFiClient* wifiClient_;
  static WiFiClientSecure* secureClient_;
  static PubSubClient mqttClient_;

  static MqttMessageCallback mqttCallback_;
  
  static bool apModeActive_;
  static uint32_t lastWiFiRetryTime_;
  static uint32_t lastMqttRetryTime_;
  static uint32_t connectionStartTime_;
  
  static constexpr uint32_t kWiFiRetryIntervalMs = 5000;
  static constexpr uint32_t kMqttRetryIntervalMs = 5000;
};

}  // namespace PoolController
