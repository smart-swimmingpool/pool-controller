#pragma once
#include <string>
#include <functional>
#include "Arduino.h"
#include "espMqttClientAsync.h"

namespace PoolController {

class NetworkManager {
public:
  using MqttMessageCallback = MqttMessageHandler;

  static bool begin() { return true; }
  static void loop() {}
  static bool isWiFiConnected() { return _wifiConnected; }
  static bool isMqttConnected() { return _mqttConnected; }
  static bool isApMode() { return _apMode; }

  static int getWiFiRSSI() { return _wifiRssi; }
  static IPAddress getLocalIPAddress() { return _localIp; }
  static String getLocalIP() { return getLocalIPAddress().toString(); }

  static bool publish(const char *topic, const char *payload, bool retained = false) {
    if (!_publishOk)
      return false;
    bool ok = _mqttClient.publish(topic, 1, retained, payload) > 0;
    if (_publishHook && _publishHookTopic == topic) {
      auto hook = std::move(_publishHook);  // one-shot: prevents recursion
      _publishHook = nullptr;
      _publishHookTopic.clear();
      hook();
    }
    return ok;
  }
  static bool subscribe(const char *topic) { return _mqttClient.subscribe(topic) > 0; }

  static void setMqttCallback(MqttMessageCallback cb) {
    // Mirrors espMqttClient: onMessage() replaces the previous handler.
    _mqttCallback = cb;
    _mqttClient.onMessage(cb);
  }

  static void clearMqttCallback() {
    _mqttCallback = nullptr;
    _mqttClient.onMessage(nullptr);
  }

  static void disconnectMqtt() {
    _mqttClient.disconnect();
    _mqttConnected = false;
  }
  static void startAPMode() { _apMode = true; }
  static void restart() { ESP.restart(); }

  // Test helpers
  static void setWiFiConnected(bool v) { _wifiConnected = v; }
  static void setMqttConnected(bool v) { _mqttConnected = v; }
  static void setApMode(bool v) { _apMode = v; }
  static void setWiFiRSSI(int rssi) { _wifiRssi = rssi; }
  static void setLocalIP(const IPAddress &ip) { _localIp = ip; }
  // Simulate the MQTT client refusing to enqueue (publish returns false).
  static void setPublishOk(bool ok) { _publishOk = ok; }
  // One-shot hook fired when the next publish() targets `topic` — lets a test
  // simulate a reentrant publishStates() from the MQTT client callback
  // path while an export on that topic is already in progress.
  static void setPublishHook(const char *topic, std::function<void()> cb) {
    _publishHookTopic = topic ? topic : "";
    _publishHook = std::move(cb);
  }
  static void clearPublishHook() {
    _publishHook = nullptr;
    _publishHookTopic.clear();
  }

  static espMqttClientAsync &getClient() { return _mqttClient; }

private:
  static espMqttClientAsync _mqttClient;
  static bool _wifiConnected;
  static bool _mqttConnected;
  static bool _apMode;
  static int _wifiRssi;
  static IPAddress _localIp;
  static bool _publishOk;
  static std::function<void()> _publishHook;
  static std::string _publishHookTopic;
  static MqttMessageCallback _mqttCallback;
};

}  // namespace PoolController
