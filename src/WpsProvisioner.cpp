// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "WpsProvisioner.hpp"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wps.h>
#include <atomic>

namespace {
constexpr gpio_num_t WPS_TRIGGER_PIN{GPIO_NUM_0};
constexpr uint32_t WPS_TRIGGER_HOLD_MS{1500UL};
constexpr uint32_t WPS_SESSION_TIMEOUT_MS{120000UL};
constexpr uint32_t WPS_CONNECT_TIMEOUT_MS{30000UL};
constexpr uint32_t WPS_TRIGGER_POLL_INTERVAL_MS{10UL};
constexpr uint32_t WIFI_STATUS_POLL_INTERVAL_MS{50UL};
constexpr size_t WIFI_SSID_MAX_LEN{32};
constexpr bool WIFI_DISCONNECT_TURN_OFF_RADIO{false};
constexpr bool WIFI_DISCONNECT_ERASE_CREDENTIALS{false};
constexpr size_t HOMIE_CONFIG_BUFFER_SIZE{4096};
constexpr const char *HOMIE_CONFIG_PATH{"/homie/config.json"};
constexpr const char *HOMIE_CONFIG_TMP_PATH{"/homie/config.wps.tmp"};
constexpr wps_type_t WPS_MODE{WPS_TYPE_PBC};

struct WpsProvisionState final {
  std::atomic<bool> success{false};
  std::atomic<bool> failed{false};
  std::atomic<bool> timedOut{false};
};

static WpsProvisionState wpsProvisionState{};
static bool spiffsMountedForWps{false};
static StaticJsonDocument<HOMIE_CONFIG_BUFFER_SIZE> homieConfigJson{};

auto stopWps() -> void {
  const esp_err_t disableErr = esp_wifi_wps_disable();
  if (disableErr != ESP_OK && disableErr != ESP_ERR_WIFI_WPS_SM) {
    Serial.printf("WPS disable failed: 0x%x (%s)\n", static_cast<unsigned>(disableErr), esp_err_to_name(disableErr));
  }
}

auto startWps() -> bool {
  esp_wps_config_t config{};
  config.wps_type = WPS_MODE;
  snprintf(config.factory_info.manufacturer, sizeof(config.factory_info.manufacturer), "smart-swimmingpool");
  snprintf(config.factory_info.model_number, sizeof(config.factory_info.model_number), "pool-controller");
  snprintf(config.factory_info.model_name, sizeof(config.factory_info.model_name), "ESP32 Pool Controller");
  snprintf(config.factory_info.device_name, sizeof(config.factory_info.device_name), "Pool Controller");
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

auto persistWpsWifiCredentials() -> bool {
  if (!spiffsMountedForWps && !SPIFFS.begin(false)) {
    Serial.println("WPS: cannot mount SPIFFS");
    return false;
  }
  spiffsMountedForWps = true;

  if (!SPIFFS.exists(HOMIE_CONFIG_PATH)) {
    Serial.println("WPS: Homie config missing, skip credential persistence");
    return false;
  }

  File configFile = SPIFFS.open(HOMIE_CONFIG_PATH, "r");
  if (!configFile) {
    Serial.println("WPS: cannot open Homie config for read");
    return false;
  }

  const size_t configSize = configFile.size();
  if (configSize == 0 || configSize >= HOMIE_CONFIG_BUFFER_SIZE) {
    Serial.println("WPS: Homie config size invalid");
    configFile.close();
    return false;
  }

  homieConfigJson.clear();
  const DeserializationError parseErr = deserializeJson(homieConfigJson, configFile);
  configFile.close();
  if (parseErr != DeserializationError::Ok || !homieConfigJson.is<JsonObject>()) {
    Serial.println("WPS: Homie config JSON parse failed");
    return false;
  }

  char connectedSsid[WIFI_SSID_MAX_LEN + 1];
  connectedSsid[0] = '\0';
  WiFi.SSID().toCharArray(connectedSsid, sizeof(connectedSsid));
  if (connectedSsid[0] == '\0') {
    Serial.println("WPS: no SSID after successful pairing");
    return false;
  }

  JsonObject root = homieConfigJson.as<JsonObject>();
  JsonObject wifi = root["wifi"].is<JsonObject>() ? root["wifi"].as<JsonObject>() : root.createNestedObject("wifi");
  wifi["ssid"] = connectedSsid;
  wifi["password"] = WiFi.psk();

  SPIFFS.remove(HOMIE_CONFIG_TMP_PATH);
  File outFile = SPIFFS.open(HOMIE_CONFIG_TMP_PATH, "w");
  if (!outFile) {
    Serial.println("WPS: cannot open Homie config for write");
    return false;
  }

  const size_t written = serializeJson(homieConfigJson, outFile);
  outFile.close();

  if (written == 0) {
    SPIFFS.remove(HOMIE_CONFIG_TMP_PATH);
    Serial.println("WPS: failed serializing Homie config");
    return false;
  }

  if (SPIFFS.exists(HOMIE_CONFIG_PATH) && !SPIFFS.remove(HOMIE_CONFIG_PATH)) {
    SPIFFS.remove(HOMIE_CONFIG_TMP_PATH);
    Serial.println("WPS: cannot replace old Homie config");
    return false;
  }

  if (!SPIFFS.rename(HOMIE_CONFIG_TMP_PATH, HOMIE_CONFIG_PATH)) {
    SPIFFS.remove(HOMIE_CONFIG_TMP_PATH);
    Serial.println("WPS: cannot activate updated Homie config");
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
auto WpsProvisioner::runIfRequested() -> void {
  if (!shouldStartWpsProvisioning()) {
    return;
  }

  Serial.println("WPS: trigger button held, starting WPS provisioning");

  wpsProvisionState.success.store(false);
  wpsProvisionState.failed.store(false);
  wpsProvisionState.timedOut.store(false);
  WiFi.disconnect(WIFI_DISCONNECT_TURN_OFF_RADIO, WIFI_DISCONNECT_ERASE_CREDENTIALS);
  WiFi.mode(WIFI_MODE_STA);
  WiFiEventId_t handlerId = WiFi.onEvent(handleWpsEvent);

  if (!startWps()) {
    WiFi.removeEvent(handlerId);
    return;
  }

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
      Serial.println("WPS: connected, but credentials were not persisted");
    }
  } else {
    Serial.println("WPS: provisioning failed or timed out");
    stopWps();
  }

  WiFi.removeEvent(handlerId);
}
}  // namespace PoolController
