#pragma once
#include <string>
#include <functional>
#include "Arduino.h"
#include "AsyncMqttClient.h"

namespace PoolController {

class NetworkManager {
public:
  using MqttMessageCallback = std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)>;
  
  static bool begin() { return true; }
  static void loop() {}
  static bool isWiFiConnected() { return _wifiConnected; }
  static bool isMqttConnected() { return _mqttConnected; }
  static bool isApMode() { return _apMode; }
  
  static int getWiFiRSSI() { return _wifiRssi; }
  static String getLocalIP() { return String("192.168.1.100"); }
  
  static bool publish(const char *topic, const char *payload, bool retained = false) {
    return _mqttClient.publish(topic, 1, retained, payload) > 0;
  }
  static bool subscribe(const char *topic) { return _mqttClient.subscribe(topic) > 0; }
  
  static void setMqttCallback(MqttMessageCallback cb) {
    _mqttClient.onMessage(cb);
  }
  
  static void disconnectMqtt() { _mqttClient.disconnect(); _mqttConnected = false; }
  static void startAPMode() { _apMode = true; }

  // Test helpers
  static void setWiFiConnected(bool v) { _wifiConnected = v; }
  static void setMqttConnected(bool v) { _mqttConnected = v; }
  static void setApMode(bool v) { _apMode = v; }
  static void setWiFiRSSI(int rssi) { _wifiRssi = rssi; }
  
  static AsyncMqttClient &getClient() { return _mqttClient; }

private:
  static AsyncMqttClient _mqttClient;
  static bool _wifiConnected;
  static bool _mqttConnected;
  static bool _apMode;
  static int _wifiRssi;
};

} // namespace PoolController
