// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "WpsProvisioner.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wps.h>
#include <atomic>

#include "ConfigManager.hpp"

namespace {
constexpr gpio_num_t WPS_TRIGGER_PIN{GPIO_NUM_0};
constexpr uint32_t WPS_TRIGGER_HOLD_MS{2000UL};
constexpr uint32_t WPS_SESSION_TIMEOUT_MS{120000UL};
constexpr uint32_t WPS_CONNECT_TIMEOUT_MS{30000UL};
constexpr uint32_t WPS_TRIGGER_POLL_INTERVAL_MS{10UL};
constexpr uint32_t WIFI_STATUS_POLL_INTERVAL_MS{50UL};
constexpr size_t WIFI_SSID_MAX_LEN{32};
constexpr bool WIFI_KEEP_RADIO_ON{true};
constexpr bool WIFI_PRESERVE_CREDENTIALS{true};
constexpr wps_type_t WPS_MODE{WPS_TYPE_PBC};

struct WpsProvisionState final {
  std::atomic<bool> success{false};
  std::atomic<bool> failed{false};
  std::atomic<bool> timedOut{false};
};

static WpsProvisionState wpsProvisionState{};

auto stopWps() -> void {
  const esp_err_t disableErr = esp_wifi_wps_disable();
  if (disableErr != ESP_OK && disableErr != ESP_ERR_WIFI_WPS_SM) {
    Serial.printf("WPS disable failed: 0x%x (%s)\n", static_cast<unsigned>(disableErr), esp_err_to_name(disableErr));
  }
}

auto startWps() -> bool {
  esp_wps_config_t config{};
  config.wps_type = WPS_MODE;
  const int manufacturerLen =
    snprintf(config.factory_info.manufacturer, sizeof(config.factory_info.manufacturer), "smart-swimmingpool");
  const int modelNumberLen =
    snprintf(config.factory_info.model_number, sizeof(config.factory_info.model_number), "pool-controller");
  const int modelNameLen =
    snprintf(config.factory_info.model_name, sizeof(config.factory_info.model_name), "ESP32 Pool Controller");
  const int deviceNameLen = snprintf(config.factory_info.device_name, sizeof(config.factory_info.device_name), "Pool Controller");

  if (manufacturerLen < 0 || static_cast<size_t>(manufacturerLen) >= sizeof(config.factory_info.manufacturer) ||
    modelNumberLen < 0 || static_cast<size_t>(modelNumberLen) >= sizeof(config.factory_info.model_number) || modelNameLen < 0 ||
    static_cast<size_t>(modelNameLen) >= sizeof(config.factory_info.model_name) || deviceNameLen < 0 ||
    static_cast<size_t>(deviceNameLen) >= sizeof(config.factory_info.device_name)) {
    Serial.println(F("WPS: factory-info string truncated"));
    return false;
  }

  const esp_err_t enableErr = esp_wifi_wps_enable(&config);
  if (enableErr != ESP_OK) {
    Serial.printf("WPS enable failed: 0x%x (%s)\n", static_cast<unsigned>(enableErr), esp_err_to_name(enableErr));
    return false;
  }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
  const esp_err_t startErr = esp_wifi_wps_start();
#else
  const esp_err_t startErr = esp_wifi_wps_start(0);
#endif
  if (startErr != ESP_OK) {
    Serial.printf("WPS start failed: 0x%x (%s)\n", static_cast<unsigned>(startErr), esp_err_to_name(startErr));
    stopWps();
    return false;
  }
  return true;
}

/**
 * Persist the WiFi credentials obtained via WPS into ConfigManager
 * and save to LittleFS (replaces the old SPIFFS + /homie/config.json approach).
 */
auto persistWpsWifiCredentials() -> bool {
  char connectedSsid[WIFI_SSID_MAX_LEN + 1];
  connectedSsid[0] = '\0';
  WiFi.SSID().toCharArray(connectedSsid, sizeof(connectedSsid));

  if (connectedSsid[0] == '\0') {
    Serial.println(F("WPS: no SSID after successful pairing"));
    return false;
  }

  PoolController::ConfigManager::getWiFi().ssid = connectedSsid;
  PoolController::ConfigManager::getWiFi().password = WiFi.psk();
  PoolController::ConfigManager::setConfigured(true);  // P1: Mark device as configured

  if (!PoolController::ConfigManager::save()) {
    Serial.println(F("WPS: failed to persist WiFi credentials to config"));
    return false;
  }

  Serial.printf("WPS: persisted WiFi credentials for SSID '%s'\n", connectedSsid);
  return true;
}

auto waitForWifiConnected(const uint32_t timeoutMs) -> bool {
  const uint32_t startedAt = millis();
  while ((millis() - startedAt) < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(WIFI_STATUS_POLL_INTERVAL_MS));
  }
  return false;
}

auto handleWpsEvent(WiFiEvent_t event, arduino_event_info_t info) -> void {
  switch (event) {
  case ARDUINO_EVENT_WPS_ER_SUCCESS:
    wpsProvisionState.success = true;
    stopWps();
    WiFi.begin();
    break;
  case ARDUINO_EVENT_WPS_ER_FAILED:
    wpsProvisionState.failed = true;
    stopWps();
    break;
  case ARDUINO_EVENT_WPS_ER_TIMEOUT:
    wpsProvisionState.timedOut = true;
    stopWps();
    break;
  case ARDUINO_EVENT_WPS_ER_PIN:
    (void)info;
    break;
  default:
    break;
  }
}

auto shouldStartWpsProvisioning() -> bool {
  pinMode(static_cast<uint8_t>(WPS_TRIGGER_PIN), INPUT_PULLUP);
  const uint32_t startedAt = millis();
  while ((millis() - startedAt) < WPS_TRIGGER_HOLD_MS) {
    if (digitalRead(static_cast<uint8_t>(WPS_TRIGGER_PIN)) != LOW) {
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(WPS_TRIGGER_POLL_INTERVAL_MS));
  }
  return true;
}
}  // namespace

namespace PoolController {

auto WpsProvisioner::runIfRequested() -> bool {
  // This runs only during setup and uses bounded waits to detect trigger/WPS completion.
  if (!shouldStartWpsProvisioning()) {
    return false;
  }

  Serial.println(F("WPS: trigger button held, starting WPS provisioning"));

  wpsProvisionState.success.store(false);
  wpsProvisionState.failed.store(false);
  wpsProvisionState.timedOut.store(false);
  WiFi.disconnect(!WIFI_KEEP_RADIO_ON, !WIFI_PRESERVE_CREDENTIALS);
  WiFi.mode(WIFI_MODE_STA);
  WiFiEventId_t handlerId = WiFi.onEvent(handleWpsEvent);

  if (!startWps()) {
    WiFi.removeEvent(handlerId);
    return false;
  }

  const auto cleanupAndReturn = [handlerId](const bool result) {
    WiFi.removeEvent(handlerId);
    return result;
  };

  const uint32_t startedAt = millis();
  while ((millis() - startedAt) < WPS_SESSION_TIMEOUT_MS) {
    if (wpsProvisionState.success.load() || wpsProvisionState.failed.load() || wpsProvisionState.timedOut.load()) {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(WIFI_STATUS_POLL_INTERVAL_MS));
  }

  if (wpsProvisionState.success.load() && waitForWifiConnected(WPS_CONNECT_TIMEOUT_MS)) {
    const bool persisted = persistWpsWifiCredentials();
    if (!persisted) {
      Serial.println(F("WPS: connected, but credentials were not persisted"));
    }
    return cleanupAndReturn(persisted);
  } else {
    Serial.println(F("WPS: provisioning failed or timed out"));
    stopWps();
    // Retry with previously stored WiFi credentials.
    WiFi.begin();
  }

  return cleanupAndReturn(false);
}

}  // namespace PoolController
