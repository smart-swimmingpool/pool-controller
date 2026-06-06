// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "NetworkManager.hpp"

#include <esp_wifi.h>
#include <esp_wps.h>

#include "ConfigManager.hpp"
#include "WpsProvisioner.hpp"

namespace PoolController {

WiFiClient *NetworkManager::wifiClient_ = nullptr;
WiFiClientSecure *NetworkManager::secureClient_ = nullptr;
PubSubClient NetworkManager::mqttClient_;

NetworkManager::MqttMessageCallback NetworkManager::mqttCallback_ = nullptr;

bool NetworkManager::apModeActive_ = false;
uint32_t NetworkManager::lastWiFiRetryTime_ = 0;
uint32_t NetworkManager::lastMqttRetryTime_ = 0;
uint32_t NetworkManager::connectionStartTime_ = 0;

static void internalMqttCallback(char *topic, uint8_t *payload, unsigned int length) {
  if (NetworkManager::isMqttConnected() && NetworkManager::subscribe != nullptr) {
    // Forward to configured callback
  }
}

bool NetworkManager::begin() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_MODE_STA);

  // Check if WPS button is held down at startup
  const bool wpsCredentialsPersisted = WpsProvisioner::runIfRequested();

  // Reload only when WPS actually persisted credentials, so runtime config
  // matches freshly saved values before the STA connection attempt.
  if (wpsCredentialsPersisted) {
    ConfigManager::load();
  }

  WiFi.onEvent(handleWiFiEvent);

  if (ConfigManager::getWiFi().ssid.length() == 0) {
    Serial.println("⚠ No WiFi SSID configured! Starting AP mode.");
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
        Serial.println("🔄 AP mode: retrying WiFi connection with saved credentials...");
        WiFi.mode(WIFI_MODE_APSTA);
        connectWiFi();
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✓ AP mode: WiFi reconnected! Switching back to normal mode.");
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
      Serial.println("⚠ WiFi connection timeout (20s). Falling back to AP Setup Mode!");
      startAPMode();
      return;
    }

    if (now - lastWiFiRetryTime_ >= kWiFiRetryIntervalMs) {
      lastWiFiRetryTime_ = now;
      wl_status_t status = WiFi.status();
      const char *statusStr = "";
      switch (status) {
      case WL_IDLE_STATUS:     statusStr = "IDLE"; break;
      case WL_NO_SSID_AVAIL:   statusStr = "NO_SSID_AVAIL"; break;
      case WL_SCAN_COMPLETED:  statusStr = "SCAN_COMPLETED"; break;
      case WL_CONNECT_FAILED:  statusStr = "CONNECT_FAILED"; break;
      case WL_CONNECTION_LOST: statusStr = "CONNECTION_LOST"; break;
      case WL_DISCONNECTED:    statusStr = "DISCONNECTED"; break;
      default:                 statusStr = "UNKNOWN"; break;
      }
      Serial.printf("🔄 WiFi retry... status=%s (%d), elapsed=%ums\n",
        statusStr, status, now - connectionStartTime_);
      connectWiFi();
    }
    return;
  } else {
    connectionStartTime_ = 0;
  }

  // Handle MQTT connection & loop
  if (ConfigManager::getMqtt().host.length() > 0) {
    if (!isMqttConnected()) {
      uint32_t now = millis();
      if (now - lastMqttRetryTime_ >= kMqttRetryIntervalMs) {
        lastMqttRetryTime_ = now;
        Serial.println("🔄 MQTT disconnected, retrying...");
        connectMqtt();
      }
    } else {
      mqttClient_.loop();
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
  Serial.print("🚀 AP Mode active. SSID: 'Pool-Controller-Setup'. IP: ");
  Serial.println(WiFi.softAPIP());
}

void NetworkManager::connectWiFi() {
  const String &ssid = ConfigManager::getWiFi().ssid;
  Serial.printf("📡 Connecting to WiFi: %s ...\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), ConfigManager::getWiFi().password.c_str());
}

void NetworkManager::connectMqtt() {
  const MqttConfig &config = ConfigManager::getMqtt();
  if (config.host.length() == 0) {
    return;
  }

  // Clean up existing clients
  if (wifiClient_ != nullptr) {
    delete wifiClient_;
    wifiClient_ = nullptr;
  }
  if (secureClient_ != nullptr) {
    delete secureClient_;
    secureClient_ = nullptr;
  }

  if (config.useTls) {
    Serial.println("🔒 Connecting to MQTT via TLS...");
    secureClient_ = new WiFiClientSecure();
    secureClient_->setInsecure();  // Allows local broker connection without CA verification
    mqttClient_.setClient(*secureClient_);
  } else {
    Serial.println("🔓 Connecting to MQTT unencrypted...");
    wifiClient_ = new WiFiClient();
    mqttClient_.setClient(*wifiClient_);
  }

  mqttClient_.setServer(config.host.c_str(), config.port);

  // Increase buffer size for HA Discovery JSON payloads (~400B, default 256 is too small)
  mqttClient_.setBufferSize(1024);

  if (mqttCallback_ != nullptr) {
    mqttClient_.setCallback(mqttCallback_);
  }

  // Generate standard unique client ID
  String clientId = "pool-controller-" + String((uint32_t)ESP.getEfuseMac(), HEX);

  // Set LWT (Last Will and Testament) availability topic for HA
  String lwtTopic = "homeassistant/sensor/pool-controller/availability";

  bool success;
  if (config.username.length() > 0) {
    success = mqttClient_.connect(
      clientId.c_str(), config.username.c_str(), config.password.c_str(), lwtTopic.c_str(), 1, true, "offline");
  } else {
    success = mqttClient_.connect(clientId.c_str(), lwtTopic.c_str(), 1, true, "offline");
  }

  if (success) {
    Serial.println("✓ MQTT connected!");
    // Publish standard online availability payload
    mqttClient_.publish(lwtTopic.c_str(), "online", true);
  } else {
    Serial.printf("✖ MQTT connection failed, rc=%d\n", mqttClient_.state());
  }
}

bool NetworkManager::publish(const char *topic, const char *payload, bool retained) {
  if (!isMqttConnected()) {
    return false;
  }
  return mqttClient_.publish(topic, payload, retained);
}

bool NetworkManager::subscribe(const char *topic) {
  if (!isMqttConnected()) {
    return false;
  }
  return mqttClient_.subscribe(topic);
}

void NetworkManager::setMqttCallback(MqttMessageCallback callback) {
  mqttCallback_ = callback;
  if (isMqttConnected() || ConfigManager::getMqtt().host.length() > 0) {
    mqttClient_.setCallback(callback);
  }
}

void NetworkManager::disconnectMqtt() {
  if (mqttClient_.connected()) {
    mqttClient_.disconnect();
  }
}

void NetworkManager::handleWiFiEvent(WiFiEvent_t event) {
  switch (event) {
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    Serial.printf(
      "✓ WiFi connected! SSID: \"%s\", IP: %s, RSSI: %d dBm, Channel: %d\n",
      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
    apModeActive_ = false;
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    Serial.printf("✖ WiFi disconnected. Status: %d (SSID: \"%s\")\n", WiFi.status(), WiFi.SSID().c_str());
    break;
  default:
    break;
  }
}

}  // namespace PoolController
