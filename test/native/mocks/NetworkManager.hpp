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
    if (!_publishOk) return false;
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
    _mqttClient.onMessage(cb);
  }

  static void disconnectMqtt() { _mqttClient.disconnect(); _mqttConnected = false; }
  static void startAPMode() { _apMode = true; }
  static void restart() { ESP.restart(); }

  // Test helpers
  static void setWiFiConnected(bool v) { _wifiConnected = v; }
  static void setMqttConnected(bool v) { _mqttConnected = v; }
  static void setApMode(bool v) { _apMode = v; }
  static void setWiFiRSSI(int rssi) { _wifiRssi = rssi; }
  // Simulate AsyncMqttClient refusing to enqueue (publish returns false).
  static void setPublishOk(bool ok) { _publishOk = ok; }
  // One-shot hook fired when the next publish() targets `topic` — lets a test
  // simulate a reentrant publishStates() from the AsyncMqttClient callback
  // path while an export on that topic is already in progress.
  static void setPublishHook(const char *topic, std::function<void()> cb) {
    _publishHookTopic = topic ? topic : "";
    _publishHook = std::move(cb);
  }
  static void clearPublishHook() {
    _publishHook = nullptr;
    _publishHookTopic.clear();
  }

  static AsyncMqttClient &getClient() { return _mqttClient; }

private:
  static AsyncMqttClient _mqttClient;
  static bool _wifiConnected;
  static bool _mqttConnected;
  static bool _apMode;
  static int _wifiRssi;
  static bool _publishOk;
  static std::function<void()> _publishHook;
  static std::string _publishHookTopic;
};

} // namespace PoolController
