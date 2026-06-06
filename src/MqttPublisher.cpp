// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "MqttPublisher.hpp"
#include "Version.h"
#include "NetworkManager.hpp"
#include "ConfigManager.hpp"
#include "OtaUpdater.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"
#include "TimeClientHelper.hpp"
#include <ArduinoJson.h>

namespace PoolController {

// Nodes declared in PoolController.cpp
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;
extern RelayModuleNode poolPumpNode;
extern RelayModuleNode solarPumpNode;
extern OperationModeNode operationModeNode;

String MqttPublisher::deviceId_ = "";

void MqttPublisher::begin() {
  // Generate unique MAC-based device identifier
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "pool_controller_%02x%02x%02x", mac[3], mac[4], mac[5]);
  deviceId_ = String(macStr);

  Serial.printf("✓ HA Discovery Device ID set to: %s\n", deviceId_.c_str());

  // Register callback in NetworkManager
  NetworkManager::setMqttCallback(handleMqttMessage);
}

String MqttPublisher::getDeviceJson() {
  // Common device block to consolidate all entities in HA (F5 Fix)
  return String("{\"identifiers\":[\"") + deviceId_ +
    "\"],\"name\":\"Pool Controller\",\"manufacturer\":\"smart-swimmingpool\",\"model\":\"Pool Controller\",\"sw_version\":\"" +
    FW_VERSION + "\"}";
}

String MqttPublisher::getBaseTopic(const char *component, const char *objectId) {
  // homeassistant/<component>/pool-controller/<object-id>/config
  return String("homeassistant/") + component + "/pool-controller/" + objectId;
}

void MqttPublisher::publishSensorDiscovery(
  const char *objectId, const char *name, const char *deviceClass, const char *unit, const char *icon,
  const char *entityCategory) {
  String configTopic = getBaseTopic("sensor", objectId) + "/config";

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = getBaseTopic("sensor", objectId) + "/state";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  if (deviceClass)
    doc["device_class"] = deviceClass;
  if (unit)
    doc["unit_of_measurement"] = unit;
  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;

  // Embedded device block - manually add device info
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = deviceId_;
  deviceObj["name"] = "Pool Controller";
  deviceObj["manufacturer"] = "smart-swimmingpool";
  deviceObj["model"] = "Pool Controller";
  deviceObj["sw_version"] = FW_VERSION;

  String payload;
  serializeJson(doc, payload);
  NetworkManager::publish(configTopic.c_str(), payload.c_str(), true);
}

void MqttPublisher::publishSwitchDiscovery(const char *objectId, const char *name, const char *icon,
  const char *entityCategory) {
  String configTopic = getBaseTopic("switch", objectId) + "/config";

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = getBaseTopic("switch", objectId) + "/state";
  doc["command_topic"] = getBaseTopic("switch", objectId) + "/set";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";

  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  // Embedded device block - manually add device info
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = deviceId_;
  deviceObj["name"] = "Pool Controller";
  deviceObj["manufacturer"] = "smart-swimmingpool";
  deviceObj["model"] = "Pool Controller";
  deviceObj["sw_version"] = FW_VERSION;

  String payload;
  serializeJson(doc, payload);
  NetworkManager::publish(configTopic.c_str(), payload.c_str(), true);
}

void MqttPublisher::publishSelectDiscovery(
  const char *objectId, const char *name, const char *const *options, size_t optionCount, const char *icon,
  const char *entityCategory) {
  String configTopic = getBaseTopic("select", objectId) + "/config";

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = getBaseTopic("select", objectId) + "/state";
  doc["command_topic"] = getBaseTopic("select", objectId) + "/set";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  JsonArray opts = doc["options"].to<JsonArray>();
  for (size_t i = 0; i < optionCount; ++i) {
    opts.add(options[i]);
  }

  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  // Embedded device block - manually add device info
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = deviceId_;
  deviceObj["name"] = "Pool Controller";
  deviceObj["manufacturer"] = "smart-swimmingpool";
  deviceObj["model"] = "Pool Controller";
  deviceObj["sw_version"] = FW_VERSION;

  String payload;
  serializeJson(doc, payload);
  NetworkManager::publish(configTopic.c_str(), payload.c_str(), true);
}

void MqttPublisher::publishNumberDiscovery(
  const char *objectId, const char *name, double minVal, double maxVal, double step, const char *unit, const char *icon,
  const char *entityCategory) {
  String configTopic = getBaseTopic("number", objectId) + "/config";

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = getBaseTopic("number", objectId) + "/state";
  doc["command_topic"] = getBaseTopic("number", objectId) + "/set";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["min"] = minVal;
  doc["max"] = maxVal;
  doc["step"] = step;
  doc["mode"] = "box";

  if (unit)
    doc["unit_of_measurement"] = unit;
  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  // Embedded device block - manually add device info
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = deviceId_;
  deviceObj["name"] = "Pool Controller";
  deviceObj["manufacturer"] = "smart-swimmingpool";
  deviceObj["model"] = "Pool Controller";
  deviceObj["sw_version"] = FW_VERSION;

  String payload;
  serializeJson(doc, payload);
  NetworkManager::publish(configTopic.c_str(), payload.c_str(), true);
}

void MqttPublisher::publishTextDiscovery(const char *objectId, const char *name, const char *icon) {
  String configTopic = getBaseTopic("text", objectId) + "/config";

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = getBaseTopic("text", objectId) + "/state";
  doc["command_topic"] = getBaseTopic("text", objectId) + "/set";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["entity_category"] = "config";

  if (icon)
    doc["icon"] = icon;
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = deviceId_;
  deviceObj["name"] = "Pool Controller";
  deviceObj["manufacturer"] = "smart-swimmingpool";
  deviceObj["model"] = "Pool Controller";
  deviceObj["sw_version"] = FW_VERSION;

  String payload;
  serializeJson(doc, payload);
  NetworkManager::publish(configTopic.c_str(), payload.c_str(), true);
}

void MqttPublisher::publishUpdateDiscovery() {
  String configTopic = getBaseTopic("update", "firmware-update") + "/config";

  JsonDocument doc;
  doc["name"] = "Firmware";
  doc["unique_id"] = deviceId_ + "_fw_update";
  doc["state_topic"] = getBaseTopic("update", "firmware-update") + "/state";
  doc["command_topic"] = getBaseTopic("update", "firmware-update") + "/set";
  doc["json_attributes_topic"] = getBaseTopic("update", "firmware-update") + "/attr";
  doc["latest_version_topic"] = getBaseTopic("update", "firmware-update") + "/latest";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["payload_install"] = "INSTALL";
  doc["device_class"] = "firmware";
  doc["entity_category"] = "config";

  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = deviceId_;
  deviceObj["name"] = "Pool Controller";
  deviceObj["manufacturer"] = "smart-swimmingpool";
  deviceObj["model"] = "Pool Controller";
  deviceObj["sw_version"] = FW_VERSION;

  String payload;
  serializeJson(doc, payload);
  NetworkManager::publish(configTopic.c_str(), payload.c_str(), true);
}

void MqttPublisher::publishUpdateState() {
  if (!NetworkManager::isMqttConnected())
    return;

  // State topic: current installed version
  String stateTopic = getBaseTopic("update", "firmware-update") + "/state";
  NetworkManager::publish(stateTopic.c_str(), OtaUpdater::getCurrentVersion().c_str(), true);

  // Latest version topic: the newest available version (or current if up to date)
  String latestTopic = getBaseTopic("update", "firmware-update") + "/latest";
  String latestVer = OtaUpdater::isUpdateAvailable() ? OtaUpdater::getLatestVersion() : OtaUpdater::getCurrentVersion();
  NetworkManager::publish(latestTopic.c_str(), latestVer.c_str(), true);

  // Attributes topic: extra metadata
  String attrTopic = getBaseTopic("update", "firmware-update") + "/attr";

  JsonDocument doc;
  doc["installed_version"] = OtaUpdater::getCurrentVersion();
  doc["title"] = "Pool Controller Firmware";

  if (OtaUpdater::isUpdateAvailable()) {
    doc["latest_version"] = OtaUpdater::getLatestVersion();
    doc["release_url"] = OtaUpdater::getReleaseUrl();
  } else {
    doc["latest_version"] = OtaUpdater::getCurrentVersion();
  }

  String payload;
  serializeJson(doc, payload);
  NetworkManager::publish(attrTopic.c_str(), payload.c_str(), true);
}

void MqttPublisher::publishDiscovery() {
  if (!NetworkManager::isMqttConnected())
    return;

  Serial.println("Publishing HA Discovery Payloads...");

  // ── Diagnostics (entity_category: "diagnostic") ──
  // Temperatures
  publishSensorDiscovery("pool-temp", "Pool Temperature", "temperature", "°C", "mdi:pool", "diagnostic");
  publishSensorDiscovery("solar-temp", "Solar Temperature", "temperature", "°C", "mdi:solar-power", "diagnostic");
  publishSensorDiscovery("controller-temp", "Controller Temperature", "temperature", "°C", "mdi:thermometer", "diagnostic");

  // System diagnostics
  publishSensorDiscovery("heap", "Free Heap Space", nullptr, "B", "mdi:memory", "diagnostic");
  publishSensorDiscovery("max-alloc", "Max Alloc Block", nullptr, "B", "mdi:memory", "diagnostic");
  publishSensorDiscovery("rssi", "WiFi Signal Strength", nullptr, "dBm", "mdi:wifi", "diagnostic");
  publishSensorDiscovery("uptime", "System Uptime", nullptr, "s", "mdi:clock-outline", "diagnostic");
  publishSensorDiscovery("local-time", "Local Time", nullptr, nullptr, "mdi:clock", "diagnostic");

  // ── Controls (no entity_category — shown on device page) ──
  // Relays (Switches)
  publishSwitchDiscovery("pool-pump", "Pool Pump", "mdi:pump");
  publishSwitchDiscovery("solar-pump", "Solar Pump", "mdi:solar-panel");

  // Select Mode
  const char *modeOpts[] = {"auto", "manu", "boost", "timer"};
  publishSelectDiscovery("mode", "Operation Mode", modeOpts, 4, "mdi:sync");

  // Parameter Numbers
  publishNumberDiscovery("pool-max-temp", "Max. Pool Temp", 0.0, 40.0, 0.1, "°C", "mdi:thermometer-chevron-up");
  publishNumberDiscovery("solar-min-temp", "Min. Solar Temp", 0.0, 100.0, 0.1, "°C", "mdi:thermometer-chevron-down");
  publishNumberDiscovery("hysteresis", "Temperature Hysteresis", 0.0, 10.0, 0.1, "K", "mdi:delta");
  publishNumberDiscovery("timer-start-h", "Timer Start Hour", 0.0, 23.0, 1.0, "h", "mdi:clock-start");
  publishNumberDiscovery("timer-start-min", "Timer Start Minute", 0.0, 59.0, 1.0, "min", "mdi:clock-start");
  publishNumberDiscovery("timer-end-h", "Timer End Hour", 0.0, 23.0, 1.0, "h", "mdi:clock-end");
  publishNumberDiscovery("timer-end-min", "Timer End Minute", 0.0, 59.0, 1.0, "min", "mdi:clock-end");

  // ── Configuration (entity_category: "config") ──
  publishNumberDiscovery("timezone", "Timezone Index", 0.0, 9.0, 1.0, nullptr, "mdi:map-clock", "config");
  publishTextDiscovery("ntp-server", "NTP Server", "mdi:clock-outline");

  // Firmware Update entity
  publishUpdateDiscovery();

  // Subscribe to command topics
  NetworkManager::subscribe("homeassistant/switch/pool-controller/pool-pump/set");
  NetworkManager::subscribe("homeassistant/switch/pool-controller/solar-pump/set");
  NetworkManager::subscribe("homeassistant/select/pool-controller/mode/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/pool-max-temp/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/solar-min-temp/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/hysteresis/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/timer-start-h/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/timer-start-min/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/timer-end-h/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/timer-end-min/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/timezone/set");
  NetworkManager::subscribe("homeassistant/text/pool-controller/ntp-server/set");
  NetworkManager::subscribe("homeassistant/update/pool-controller/firmware-update/set");

  Serial.println("✓ HA Discovery Payloads & Subscriptions complete");
}

void MqttPublisher::publishStates() {
  if (!NetworkManager::isMqttConnected())
    return;

  // Temperature States
  NetworkManager::publish(
    (getBaseTopic("sensor", "pool-temp") + "/state").c_str(), String(poolTemperatureNode.getTemperature(), 1).c_str(), true);
  NetworkManager::publish(
    (getBaseTopic("sensor", "solar-temp") + "/state").c_str(), String(solarTemperatureNode.getTemperature(), 1).c_str(), true);
  NetworkManager::publish((getBaseTopic("sensor", "controller-temp") + "/state").c_str(),
    String(ctrlTemperatureNode.getTemperature(), 1).c_str(), true);

  // Diagnostic States
  NetworkManager::publish((getBaseTopic("sensor", "heap") + "/state").c_str(), String(ESP.getFreeHeap()).c_str(), true);
  NetworkManager::publish((getBaseTopic("sensor", "max-alloc") + "/state").c_str(), String(ESP.getMaxAllocHeap()).c_str(), true);
  NetworkManager::publish(
    (getBaseTopic("sensor", "rssi") + "/state").c_str(), String(NetworkManager::getWiFiRSSI()).c_str(), true);
  NetworkManager::publish((getBaseTopic("sensor", "uptime") + "/state").c_str(), String(millis() / 1000).c_str(), true);

  // Local time state
  {
    TimeChangeRule *tcr;
    time_t localTime = getTimeFor(ConfigManager::getSettings().timezoneIndex, &tcr);
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d",
      year(localTime), month(localTime), day(localTime),
      hour(localTime), minute(localTime), second(localTime));
    NetworkManager::publish((getBaseTopic("sensor", "local-time") + "/state").c_str(), timeBuf, true);
  }

  // Switch States
  NetworkManager::publish(
    (getBaseTopic("switch", "pool-pump") + "/state").c_str(), poolPumpNode.getSwitch() ? "ON" : "OFF", true);
  NetworkManager::publish(
    (getBaseTopic("switch", "solar-pump") + "/state").c_str(), solarPumpNode.getSwitch() ? "ON" : "OFF", true);

  // Firmware Update
  publishUpdateState();

  // Mode & Parameter States
  NetworkManager::publish((getBaseTopic("select", "mode") + "/state").c_str(), operationModeNode.getMode().c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "pool-max-temp") + "/state").c_str(),
    String(operationModeNode.getPoolMaxTemperature(), 1).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "solar-min-temp") + "/state").c_str(),
    String(operationModeNode.getSolarMinTemperature(), 1).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "hysteresis") + "/state").c_str(),
    String(operationModeNode.getTemperatureHysteresis(), 1).c_str(), true);

  TimerSetting ts = operationModeNode.getTimerSetting();
  NetworkManager::publish((getBaseTopic("number", "timer-start-h") + "/state").c_str(), String(ts.timerStartHour).c_str(), true);
  NetworkManager::publish(
    (getBaseTopic("number", "timer-start-min") + "/state").c_str(), String(ts.timerStartMinutes).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "timer-end-h") + "/state").c_str(), String(ts.timerEndHour).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "timer-end-min") + "/state").c_str(), String(ts.timerEndMinutes).c_str(), true);
  NetworkManager::publish(
    (getBaseTopic("number", "timezone") + "/state").c_str(), String(ConfigManager::getSettings().timezoneIndex).c_str(), true);
  NetworkManager::publish(
    (getBaseTopic("text", "ntp-server") + "/state").c_str(), ConfigManager::getNtp().server.c_str(), true);
}

void MqttPublisher::handleMqttMessage(char *topic, uint8_t *payload, unsigned int length) {
  // Convert payload to String safely
  char valStr[32];
  size_t valLen = (length < sizeof(valStr) - 1) ? length : sizeof(valStr) - 1;
  memcpy(valStr, payload, valLen);
  valStr[valLen] = '\0';
  String value(valStr);

  String top(topic);

  if (top.endsWith("/firmware-update/set")) {
    if (value == "INSTALL") {
      Serial.println("MQTT: Firmware update triggered from Home Assistant");
      OtaUpdater::startUpdate();
    }
    return;
  }

  if (top.endsWith("/pool-pump/set") || top.endsWith("/solar-pump/set")) {
    // Only allow pump control from HA in manual mode
    if (operationModeNode.getMode() != "manu") {
      Serial.printf("MQTT: Ignoring pump command — not in manual mode (current: %s)\n", operationModeNode.getMode().c_str());
      publishStates();
      return;
    }
    if (top.endsWith("/pool-pump/set")) {
      poolPumpNode.setSwitch(value == "ON");
    } else {
      solarPumpNode.setSwitch(value == "ON");
    }
  } else if (top.endsWith("/mode/set")) {
    operationModeNode.setMode(valStr);
    ConfigManager::getSettings().opMode = value;
    ConfigManager::save();
  } else if (top.endsWith("/pool-max-temp/set")) {
    float val = value.toFloat();
    operationModeNode.setPoolMaxTemperature(val);
    ConfigManager::getSettings().tempMaxPool = val;
    ConfigManager::save();
  } else if (top.endsWith("/solar-min-temp/set")) {
    float val = value.toFloat();
    operationModeNode.setSolarMinTemperature(val);
    ConfigManager::getSettings().tempMinSolar = val;
    ConfigManager::save();
  } else if (top.endsWith("/hysteresis/set")) {
    float val = value.toFloat();
    operationModeNode.setTemperatureHysteresis(val);
    ConfigManager::getSettings().tempHysteresis = val;
    ConfigManager::save();
  } else if (top.endsWith("/timer-start-h/set")) {
    int val = value.toInt();
    if (val >= 0 && val <= 23) {
      TimerSetting ts = operationModeNode.getTimerSetting();
      ts.timerStartHour = val;
      operationModeNode.setTimerSetting(ts);  // Persisted via NVS, no ConfigManager save needed
    }
  } else if (top.endsWith("/timer-start-min/set")) {
    int val = value.toInt();
    if (val >= 0 && val <= 59) {
      TimerSetting ts = operationModeNode.getTimerSetting();
      ts.timerStartMinutes = val;
      operationModeNode.setTimerSetting(ts);
    }
  } else if (top.endsWith("/timer-end-h/set")) {
    int val = value.toInt();
    if (val >= 0 && val <= 23) {
      TimerSetting ts = operationModeNode.getTimerSetting();
      ts.timerEndHour = val;
      operationModeNode.setTimerSetting(ts);
    }
  } else if (top.endsWith("/timer-end-min/set")) {
    int val = value.toInt();
    if (val >= 0 && val <= 59) {
      TimerSetting ts = operationModeNode.getTimerSetting();
      ts.timerEndMinutes = val;
      operationModeNode.setTimerSetting(ts);
    }
  } else if (top.endsWith("/timezone/set")) {
    int val = value.toInt();
    ConfigManager::getSettings().timezoneIndex = val;
    ConfigManager::save();
    // Apply timezone change to running clock immediately (P2 review fix)
    setTimezoneIndex(val);
  } else if (top.endsWith("/ntp-server/set")) {
    if (value.length() > 0 && value.length() < 128) {
      ConfigManager::getNtp().server = value;
      ConfigManager::save();
      // Restart NTP client with new server immediately
      timeClientSetup(ConfigManager::getNtp().server.c_str());
    }
  }

  // Refresh states to confirm changes to Home Assistant
  publishStates();
}

}  // namespace PoolController
