// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file NetworkManager.cpp
 * @brief WiFi STA/AP connection, MQTT broker management, captive portal handlers.
 */

#include "NetworkManager.hpp"

#include <esp_wifi.h>
#include <esp_wps.h>
#include <ESPmDNS.h>

#include "ConfigManager.hpp"
#include "WpsProvisioner.hpp"
#include "LogCapture.hpp"

namespace PoolController {

AsyncMqttClient NetworkManager::mqttClient_;

NetworkManager::MqttMessageCallback NetworkManager::mqttCallback_ = nullptr;

bool NetworkManager::apModeActive_ = false;
uint32_t NetworkManager::lastWiFiRetryTime_ = 0;
uint32_t NetworkManager::lastMqttRetryTime_ = 0;
uint32_t NetworkManager::connectionStartTime_ = 0;

bool NetworkManager::begin() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_MODE_STA);

  // Check if WPS button is held down at startup
  WpsProvisioner::runIfRequested();

  // WPS may have saved new credentials — reload config
  ConfigManager::load();

  WiFi.onEvent(handleWiFiEvent);

  // Set up MQTT event handlers (one-time, async)
  mqttClient_.onConnect([](bool sessionPresent) {
    LOG_INFO("✓ MQTT connected!\n");
    // Publish online to LWT topic immediately (async, non-blocking)
    mqttClient_.publish("homeassistant/sensor/pool-controller/availability", 1, true, "online");
  });
  mqttClient_.onDisconnect(
    [](AsyncMqttClientDisconnectReason reason) { LOG_ERROR("✖ MQTT disconnected, reason=%d\n", static_cast<int>(reason)); });

  if (ConfigManager::getWiFi().ssid.length() == 0) {
    LOG_WARN("⚠ No WiFi SSID configured! Starting AP mode.\n");
    startAPMode();
    return true;
  }

  connectWiFi();
  connectionStartTime_ = millis();
  return true;
}

void NetworkManager::loop() {
  if (apModeActive_) {
    // Auch im AP-Mode regelmäßig WiFi-Verbindung versuchen, wenn Credentials vorhanden
    if (ConfigManager::getWiFi().ssid.length() > 0) {
      uint32_t now = millis();
      if (now - lastWiFiRetryTime_ >= kWiFiRetryIntervalMs) {
        lastWiFiRetryTime_ = now;
        LOG_INFO("🔄 AP mode: retrying WiFi connection with saved credentials...\n");
        WiFi.mode(WIFI_MODE_APSTA);
        connectWiFi();
      }

      if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("✓ AP mode: WiFi reconnected! Switching back to normal mode.\n");
        WiFi.mode(WIFI_MODE_STA);
        apModeActive_ = false;
        connectionStartTime_ = 0;
      } else {
        return;
      }
    } else {
      return;
    }
  }

  // Handle WiFi reconnection
  if (!isWiFiConnected()) {
    if (connectionStartTime_ == 0) {
      connectionStartTime_ = millis();
    }

    uint32_t now = millis();
    if (now - connectionStartTime_ >= 20000) {
      LOG_WARN("⚠ WiFi connection timeout (20s). Falling back to AP Setup Mode!\n");
      startAPMode();
      return;
    }

    if (now - lastWiFiRetryTime_ >= kWiFiRetryIntervalMs) {
      lastWiFiRetryTime_ = now;
      wl_status_t status = WiFi.status();
      const char *statusStr = "";
      switch (status) {
      case WL_IDLE_STATUS:
        statusStr = "IDLE";
        break;
      case WL_NO_SSID_AVAIL:
        statusStr = "NO_SSID_AVAIL";
        break;
      case WL_SCAN_COMPLETED:
        statusStr = "SCAN_COMPLETED";
        break;
      case WL_CONNECT_FAILED:
        statusStr = "CONNECT_FAILED";
        break;
      case WL_CONNECTION_LOST:
        statusStr = "CONNECTION_LOST";
        break;
      case WL_DISCONNECTED:
        statusStr = "DISCONNECTED";
        break;
      default:
        statusStr = "UNKNOWN";
        break;
      }
      LOG_INFO("🔄 WiFi retry... status=%s (%d), elapsed=%ums\n", statusStr, status, now - connectionStartTime_);
      connectWiFi();
    }
    return;
  } else {
    connectionStartTime_ = 0;
  }

  // Handle MQTT connection & retry (async client — no loop() needed)
  if (ConfigManager::getMqtt().host.length() > 0) {
    if (!isMqttConnected()) {
      uint32_t now = millis();
      if (now - lastMqttRetryTime_ >= kMqttRetryIntervalMs) {
        lastMqttRetryTime_ = now;
        LOG_INFO("🔄 MQTT disconnected, retrying...\n");
        connectMqtt();
      }
    }
  }
}

bool NetworkManager::isWiFiConnected() {
  return !apModeActive_ && (WiFi.status() == WL_CONNECTED);
}

bool NetworkManager::isMqttConnected() {
  return !apModeActive_ && mqttClient_.connected();
}

bool NetworkManager::isApMode() {
  return apModeActive_;
}

void NetworkManager::startAPMode() {
  apModeActive_ = true;
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_MODE_AP);

  // Setup standard open AP named 'Pool-Controller-Setup'
  WiFi.softAP("Pool-Controller-Setup");
  LOG_INFO("🚀 AP Mode active. SSID: 'Pool-Controller-Setup'. IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void NetworkManager::connectWiFi() {
  const String &ssid = ConfigManager::getWiFi().ssid;
  LOG_INFO("📡 Connecting to WiFi: %s ...\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), ConfigManager::getWiFi().password.c_str());
}

void NetworkManager::connectMqtt() {
  const MqttConfig &config = ConfigManager::getMqtt();
  if (config.host.length() == 0) {
    return;
  }

  // AsyncMqttClient remembers previous config; re-apply for safety
  mqttClient_.setServer(config.host.c_str(), config.port);

  // Generate standard unique client ID (static to survive async CONNECT packet)
  static char clientId[32];
  if (clientId[0] == '\0') {
    snprintf(clientId, sizeof(clientId), "pool-controller-%08X", (uint32_t)ESP.getEfuseMac());
  }
  mqttClient_.setClientId(clientId);
  mqttClient_.setKeepAlive(15);

  if (config.username.length() > 0) {
    mqttClient_.setCredentials(config.username.c_str(), config.password.c_str());
  }

  // LWT — broker publishes "offline" if we disconnect unexpectedly
  mqttClient_.setWill("homeassistant/sensor/pool-controller/availability", 1, true, "offline");

  // onMessage callback is registered once in setMqttCallback() — do NOT re-add here
  // to avoid accumulating duplicates in AsyncMqttClient's internal vector.

  // Initiate async connection
  mqttClient_.connect();
}

bool NetworkManager::publish(const char *topic, const char *payload, bool retained) {
  if (!isMqttConnected()) {
    return false;
  }
  return mqttClient_.publish(topic, 0, retained, payload) != 0;
}

bool NetworkManager::subscribe(const char *topic) {
  if (!isMqttConnected()) {
    return false;
  }
  return mqttClient_.subscribe(topic, 0) != 0;
}

void NetworkManager::setMqttCallback(MqttMessageCallback callback) {
  mqttCallback_ = callback;
  // AsyncMqttClient onMessage is persistent (not connection-bound)
  mqttClient_.onMessage(callback);
}

void NetworkManager::publishOffline() {
  if (mqttClient_.connected()) {
    // Publish "offline" retained so HA sees the device as unavailable even
    // when the clean MQTT DISCONNECT suppresses the LWT.
    mqttClient_.publish("homeassistant/sensor/pool-controller/availability", 1, true, "offline");
    delay(350);  // Allow the async publish to be sent before disconnect/restart
  }
}

void NetworkManager::disconnectMqtt() {
  publishOffline();
  if (mqttClient_.connected()) {
    mqttClient_.disconnect();
  }
}

void NetworkManager::restart() {
  publishOffline();
  if (mqttClient_.connected()) {
    mqttClient_.disconnect();
  }
  delay(500);
  ESP.restart();
}

void NetworkManager::handleWiFiEvent(WiFiEvent_t event) {
  switch (event) {
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    LOG_INFO("✓ WiFi connected! SSID: \"%s\", IP: %s, RSSI: %d dBm, Channel: %d\n", WiFi.SSID().c_str(),
      WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
    apModeActive_ = false;
    // Start mDNS responder so the device is reachable as pool-controller.local
    if (MDNS.begin("pool-controller")) {
      MDNS.addService("http", "tcp", 80);
      LOG_INFO("✓ mDNS: pool-controller.local\n");
    } else {
      LOG_ERROR("✖ mDNS responder setup failed\n");
    }
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    LOG_ERROR("✖ WiFi disconnected. Status: %d (SSID: \"%s\")\n", WiFi.status(), WiFi.SSID().c_str());
    break;
  default:
    break;
  }
}

}  // namespace PoolController
