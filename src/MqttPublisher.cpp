// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file MqttPublisher.cpp
 * @brief Home Assistant MQTT Discovery implementation — entity configs,
 *        state publishing, and command handling.
 */

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
#include <Preferences.h>

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

void MqttPublisher::publishTextDiscovery(const char *objectId, const char *name, const char *icon,
  const char *entityCategory) {
  String configTopic = getBaseTopic("text", objectId) + "/config";

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = getBaseTopic("text", objectId) + "/state";
  doc["command_topic"] = getBaseTopic("text", objectId) + "/set";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  if (entityCategory)
    doc["entity_category"] = entityCategory;

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

void MqttPublisher::publishTimeDiscovery(const char *objectId, const char *name, const char *icon,
  const char *entityCategory) {
  String configTopic = getBaseTopic("time", objectId) + "/config";

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = getBaseTopic("time", objectId) + "/state";
  doc["command_topic"] = getBaseTopic("time", objectId) + "/set";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
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

void MqttPublisher::publishClimateDiscovery() {
  String configTopic = getBaseTopic("climate", "thermostat") + "/config";

  JsonDocument doc;
  doc["name"] = "Pool Thermostat";
  doc["unique_id"] = deviceId_ + "_thermostat";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  // Temperatures
  doc["current_temperature_topic"] = getBaseTopic("climate", "thermostat") + "/current-temperature/state";
  doc["temperature_command_topic"] = getBaseTopic("climate", "thermostat") + "/temperature/set";
  doc["temperature_state_topic"] = getBaseTopic("climate", "thermostat") + "/temperature/state";
  doc["temperature_unit"] = "C";

  // HVAC modes
  JsonArray modes = doc["modes"].to<JsonArray>();
  modes.add("off");
  modes.add("auto");
  modes.add("heat");

  doc["mode_command_topic"] = getBaseTopic("climate", "thermostat") + "/mode/set";
  doc["mode_state_topic"] = getBaseTopic("climate", "thermostat") + "/mode/state";

  // Action (heating / idle / off)
  doc["action_topic"] = getBaseTopic("climate", "thermostat") + "/action/state";

  // Temp range
  doc["min_temp"] = 0.0;
  doc["max_temp"] = 40.0;
  doc["temp_step"] = 0.5;

  // Entity category (Configuration)
  doc["entity_category"] = "config";

  // Device block
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

void MqttPublisher::publishClimateState() {
  if (!NetworkManager::isMqttConnected())
    return;

  // Current temperature (pool water)
  NetworkManager::publish((getBaseTopic("climate", "thermostat") + "/current-temperature/state").c_str(),
    String(poolTemperatureNode.getTemperature(), 1).c_str(), true);

  // Target temperature (= pool max temp setting)
  NetworkManager::publish((getBaseTopic("climate", "thermostat") + "/temperature/state").c_str(),
    String(operationModeNode.getPoolMaxTemperature(), 1).c_str(), true);

  // HVAC mode ← pool operation mode
  //   manu  → off
  //   auto  → auto
  //   boost → heat
  //   timer → auto (pool pump runs on schedule)
  const char *hvacMode = "off";
  String poolMode = operationModeNode.getMode();
  if (poolMode == "auto" || poolMode == "timer") {
    hvacMode = "auto";
  } else if (poolMode == "boost") {
    hvacMode = "heat";
  } else {
    hvacMode = "off";
  }
  NetworkManager::publish(
    (getBaseTopic("climate", "thermostat") + "/mode/state").c_str(), hvacMode, true);

  // Action: solar pump ON → heating, pool pump ON → idle, both OFF → off
  const char *action = "off";
  if (solarPumpNode.getSwitch()) {
    action = "heating";
  } else if (poolPumpNode.getSwitch()) {
    action = "idle";
  } else {
    action = "off";
  }
  NetworkManager::publish(
    (getBaseTopic("climate", "thermostat") + "/action/state").c_str(), action, true);
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

  // ── Primary Sensors (no entity_category — shown on device front page) ──
  publishSensorDiscovery("pool-temp", "Pool Temperature", "temperature", "°C", "mdi:pool");
  publishSensorDiscovery("solar-temp", "Solar Temperature", "temperature", "°C", "mdi:solar-power");

  // ── Diagnostics (entity_category: "diagnostic") ──
  publishSensorDiscovery("controller-temp", "Controller Temperature", "temperature", "°C", "mdi:thermometer", "diagnostic");
  publishSensorDiscovery("heap", "Free Heap Space", nullptr, "B", "mdi:memory", "diagnostic");
  publishSensorDiscovery("max-alloc", "Max Alloc Block", nullptr, "B", "mdi:memory", "diagnostic");
  publishSensorDiscovery("rssi", "WiFi Signal Strength", nullptr, "dBm", "mdi:wifi", "diagnostic");
  publishSensorDiscovery("uptime", "System Uptime", "duration", "s", "mdi:clock-outline", "diagnostic");
  publishSensorDiscovery("local-time", "Local Time", nullptr, nullptr, "mdi:clock", "diagnostic");

  // ── Controls (no entity_category — shown on device front page) ──
  // Relays (Switches)
  publishSwitchDiscovery("pool-pump", "Pool Pump", "mdi:pump");
  publishSwitchDiscovery("solar-pump", "Solar Pump", "mdi:solar-panel");

  // Select Mode
  const char *modeOpts[] = {"auto", "manu", "boost", "timer"};
  publishSelectDiscovery("mode", "Operation Mode", modeOpts, 4, "mdi:sync");

  // ── Configuration (entity_category: "config") ──
  // Parameter Numbers
  publishNumberDiscovery("pool-max-temp", "Max. Pool Temp", 0.0, 40.0, 0.1, "°C", "mdi:thermometer-chevron-up", "config");
  publishNumberDiscovery("solar-min-temp", "Min. Solar Temp", 0.0, 100.0, 0.1, "°C", "mdi:thermometer-chevron-down", "config");
  publishNumberDiscovery("hysteresis", "Temperature Hysteresis", 0.0, 10.0, 0.1, "K", "mdi:delta", "config");

  // Temperature-based circulation parameters
  publishNumberDiscovery(
    "temp-circ-threshold", "Circ. Temp Threshold", 0.0, 40.0, 0.5, "°C", "mdi:thermometer-auto", "config");
  publishNumberDiscovery(
    "temp-circ-factor", "Circ. Temp Factor", 0.0, 120.0, 5.0, "min/°C", "mdi:plus-minus", "config");
  publishNumberDiscovery(
    "temp-circ-max-runtime", "Circ. Max Runtime", 60.0, 1440.0, 15.0, "min", "mdi:timer-outline", "config");

  // Timer as Time entities (HH:MM:SS format)
  publishTimeDiscovery("timer-start", "Timer Start", "mdi:clock-start", "config");
  publishTimeDiscovery("timer-end", "Timer End", "mdi:clock-end", "config");

  // Select Timezone
  publishSelectDiscovery("timezone", "Timezone", getTimezoneLabelList(), getTimezoneLabelCount(), "mdi:map-clock", "config");

  // Text entities
  publishTextDiscovery("ntp-server", "NTP Server", "mdi:clock-outline", "config");

  // Runtime diagnostics
  publishSensorDiscovery(
    "effective-runtime", "Effective Runtime", "duration", "s", "mdi:timer-sand", "diagnostic");

  // ── Sensor mapping diagnostics (static entities, always available) ──
  publishSensorDiscovery(
    "solar-sensor-found", "Solar Sensor Found", nullptr, nullptr, "mdi:check-network-outline", "diagnostic");
  publishSensorDiscovery(
    "pool-sensor-found", "Pool Sensor Found", nullptr, nullptr, "mdi:check-network-outline", "diagnostic");

  // ── Configuration (entity_category: "config") ──
  publishSelectDiscovery("timezone", "Timezone", getTimezoneLabelList(), getTimezoneLabelCount(), "mdi:map-clock", "config");
  publishNumberDiscovery("hysteresis", "Temperature Hysteresis", 0.0, 10.0, 0.1, "K", "mdi:delta", "config");
  publishTextDiscovery("ntp-server", "NTP Server", "mdi:clock-outline");

  // Firmware Update entity
  publishUpdateDiscovery();

  // Climate thermostat entity (additional — existing entities stay)
  publishClimateDiscovery();

  // Sensor mapping select entities (config-category, needs fresh sensor data)
  publishSensorMappingDiscovery();

  // Subscribe to command topics
  NetworkManager::subscribe("homeassistant/switch/pool-controller/pool-pump/set");
  NetworkManager::subscribe("homeassistant/switch/pool-controller/solar-pump/set");
  NetworkManager::subscribe("homeassistant/select/pool-controller/mode/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/pool-max-temp/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/solar-min-temp/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/hysteresis/set");
  NetworkManager::subscribe("homeassistant/time/pool-controller/timer-start/set");
  NetworkManager::subscribe("homeassistant/time/pool-controller/timer-end/set");

  // Temperature-based circulation commands
  NetworkManager::subscribe("homeassistant/number/pool-controller/temp-circ-threshold/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/temp-circ-factor/set");
  NetworkManager::subscribe("homeassistant/number/pool-controller/temp-circ-max-runtime/set");

  NetworkManager::subscribe("homeassistant/select/pool-controller/timezone/set");
  NetworkManager::subscribe("homeassistant/text/pool-controller/ntp-server/set");
  NetworkManager::subscribe("homeassistant/update/pool-controller/firmware-update/set");

  // Sensor mapping commands (subscriptions added by publishSensorMappingDiscovery)

  // Climate thermostat commands
  NetworkManager::subscribe("homeassistant/climate/pool-controller/thermostat/mode/set");
  NetworkManager::subscribe("homeassistant/climate/pool-controller/thermostat/temperature/set");

  // ── Cleanup old retained discovery configs (migrated entities) ──
  // Timer was 4 Number entities → now 2 Time entities
  static const char *kOldConfigTopics[] = {
    "homeassistant/number/pool-controller/timer-start-h/config",
    "homeassistant/number/pool-controller/timer-start-min/config",
    "homeassistant/number/pool-controller/timer-end-h/config",
    "homeassistant/number/pool-controller/timer-end-min/config",
    // Timezone was Number → now Select
    "homeassistant/number/pool-controller/timezone/config",
  };
  for (auto &topic : kOldConfigTopics) {
    NetworkManager::publish(topic, "", true);  // empty retained → HA removes entity
  }

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

  // Climate thermostat
  publishClimateState();

  // Mode & Parameter States
  NetworkManager::publish((getBaseTopic("select", "mode") + "/state").c_str(), operationModeNode.getMode().c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "pool-max-temp") + "/state").c_str(),
    String(operationModeNode.getPoolMaxTemperature(), 1).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "solar-min-temp") + "/state").c_str(),
    String(operationModeNode.getSolarMinTemperature(), 1).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "hysteresis") + "/state").c_str(),
    String(operationModeNode.getTemperatureHysteresis(), 1).c_str(), true);

  {
    TimerSetting ts = operationModeNode.getTimerSetting();
    char timeBuf[9];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:00", ts.timerStartHour, ts.timerStartMinutes);
    NetworkManager::publish((getBaseTopic("time", "timer-start") + "/state").c_str(), timeBuf, true);
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:00", ts.timerEndHour, ts.timerEndMinutes);
    NetworkManager::publish((getBaseTopic("time", "timer-end") + "/state").c_str(), timeBuf, true);
  }

  // Temperature-based circulation parameters
  NetworkManager::publish((getBaseTopic("number", "temp-circ-threshold") + "/state").c_str(),
    String(ConfigManager::getSettings().tempCircThreshold, 1).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "temp-circ-factor") + "/state").c_str(),
    String(ConfigManager::getSettings().tempCircFactor).c_str(), true);
  NetworkManager::publish((getBaseTopic("number", "temp-circ-max-runtime") + "/state").c_str(),
    String(ConfigManager::getSettings().tempCircMaxRuntime).c_str(), true);

  // Effective runtime sensor — actual runtime, published in seconds (for HA duration display)
  {
    Rule *active = operationModeNode.getRule();
    uint16_t effectiveMin = (active != nullptr) ? active->getEffectiveRuntimeMinutes() : 0;
    NetworkManager::publish((getBaseTopic("sensor", "effective-runtime") + "/state").c_str(),
      String(static_cast<uint32_t>(effectiveMin) * 60).c_str(), true);
  }
  NetworkManager::publish((getBaseTopic("select", "timezone") + "/state").c_str(),
    getTimeInfoFor(ConfigManager::getSettings().timezoneIndex).c_str(), true);
  NetworkManager::publish(
    (getBaseTopic("text", "ntp-server") + "/state").c_str(), ConfigManager::getNtp().server.c_str(), true);

  // Sensor mapping states
  {
    char addrBuf[17];
    if (solarTemperatureNode.hasAddressFilter()) {
      solarTemperatureNode.getDeviceAddressString(addrBuf, sizeof(addrBuf));
      NetworkManager::publish((getBaseTopic("select", "solar-sensor") + "/state").c_str(), addrBuf, true);
    } else {
      NetworkManager::publish((getBaseTopic("select", "solar-sensor") + "/state").c_str(), "— Not configured —", true);
    }

    if (poolTemperatureNode.hasAddressFilter()) {
      poolTemperatureNode.getDeviceAddressString(addrBuf, sizeof(addrBuf));
      NetworkManager::publish((getBaseTopic("select", "pool-sensor") + "/state").c_str(), addrBuf, true);
    } else {
      NetworkManager::publish((getBaseTopic("select", "pool-sensor") + "/state").c_str(), "— Not configured —", true);
    }
    // Sensor-found binary indicators
    NetworkManager::publish((getBaseTopic("sensor", "solar-sensor-found") + "/state").c_str(),
      solarTemperatureNode.isSensorFound() ? "Found" : "Missing", true);
    NetworkManager::publish((getBaseTopic("sensor", "pool-sensor-found") + "/state").c_str(),
      poolTemperatureNode.isSensorFound() ? "Found" : "Missing", true);
  }
}

// ═══════════════════════════════════════════════════════════════════════
// Sensor mapping select-entity discovery (published after bus scan)
// ═══════════════════════════════════════════════════════════════════════

void MqttPublisher::publishSensorMappingDiscovery() {
  // Collect unique detected addresses
  uint8_t maxDev = max(solarTemperatureNode.getDeviceCount(), poolTemperatureNode.getDeviceCount());
  if (maxDev == 0) return;

  // Options: hex addresses only (no temperature — temps are volatile and would
  // cause state/option mismatches when NaN). Users identify sensors via the
  // sensor.solar_sensor_found / sensor.pool_sensor_found diagnostic entities.
  static constexpr uint8_t kMaxOpts = 21;
  const char *solarOpts[kMaxOpts];
  uint8_t solarOptCount = 0;

  const char *kNotCfg = "— Not configured —";
  solarOpts[solarOptCount++] = kNotCfg;

  String storedAddrs[kMaxOpts];
  uint8_t storedCount = 0;

  for (uint8_t i = 0; i < maxDev && storedCount < kMaxOpts - 1; i++) {
    DeviceAddress addr;
    if (solarTemperatureNode.getDetectedDeviceAddress(i, addr) ||
        poolTemperatureNode.getDetectedDeviceAddress(i, addr)) {
      char buf[17];
      snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);

      // Deduplicate (same address may appear on shared bus)
      bool seen = false;
      for (uint8_t si = 0; si < storedCount; si++) {
        if (storedAddrs[si] == buf) {
          seen = true;
          break;
        }
      }
      if (seen) continue;

      storedAddrs[storedCount] = buf;
      solarOpts[solarOptCount++] = storedAddrs[storedCount].c_str();
      storedCount++;
    }
  }

  // Publish both select entities with the same option list
  publishSelectDiscovery("solar-sensor", "Solar Sensor", solarOpts, solarOptCount, "mdi:solar-panel", "config");
  publishSelectDiscovery("pool-sensor", "Pool Sensor", solarOpts, solarOptCount, "mdi:pool", "config");

  // Subscribe to command topics
  NetworkManager::subscribe("homeassistant/select/pool-controller/solar-sensor/set");
  NetworkManager::subscribe("homeassistant/select/pool-controller/pool-sensor/set");

  Serial.printf("• HA: Sensor mapping select entities published (%u options)\n", solarOptCount);
}

void MqttPublisher::handleMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
  size_t len, size_t index, size_t total) {
  // Only process complete messages (single-chunk delivery for typical HA commands)
  if (index != 0) {
    return;
  }

  // Convert payload to String safely (AsyncMqttClient null-terminates)
  String value(payload, len);
  String top(topic);

  if (top.endsWith("/firmware-update/set")) {
    if (value == "INSTALL") {
      Serial.println("MQTT: Firmware update triggered from Home Assistant");
      OtaUpdater::startUpdate();
    }
    return;
  }

  // Climate thermostat commands (mode + temperature)
  if (top.endsWith("/thermostat/mode/set")) {
    String poolMode;
    if (value == "off")
      poolMode = "manu";
    else if (value == "auto")
      poolMode = "auto";
    else if (value == "heat")
      poolMode = "boost";
    else {
      Serial.printf("MQTT: Unknown climate mode \"%s\" — ignoring\n", value.c_str());
      publishStates();
      return;
    }
    Serial.printf("MQTT: Climate mode → pool mode \"%s\"\n", poolMode.c_str());
    operationModeNode.setMode(poolMode.c_str());
    ConfigManager::getSettings().opMode = poolMode;
    ConfigManager::save();
    publishStates();
    return;
  }

  if (top.endsWith("/thermostat/temperature/set")) {
    float val = value.toFloat();
    Serial.printf("MQTT: Climate target temperature → %.1f\n", val);
    operationModeNode.setPoolMaxTemperature(val);
    ConfigManager::getSettings().tempMaxPool = val;
    ConfigManager::save();
    publishStates();
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
    operationModeNode.setMode(value.c_str());
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
  } else if (top.endsWith("/temp-circ-threshold/set")) {
    float val = value.toFloat();
    if (val >= 0.0f && val <= 40.0f) {
      ConfigManager::getSettings().tempCircThreshold = val;
      ConfigManager::save();
    }
  } else if (top.endsWith("/temp-circ-factor/set")) {
    uint16_t val = value.toInt();
    if (val <= 120) {
      ConfigManager::getSettings().tempCircFactor = val;
      ConfigManager::save();
    }
  } else if (top.endsWith("/temp-circ-max-runtime/set")) {
    uint16_t val = value.toInt();
    if (val >= 60 && val <= 1440) {
      ConfigManager::getSettings().tempCircMaxRuntime = val;
      ConfigManager::save();
    }
  } else if (top.endsWith("/timer-start/set")) {
    // Payload format: HH:MM:SS
    int h = value.substring(0, 2).toInt();
    int m = value.substring(3, 5).toInt();
    if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
      TimerSetting ts = operationModeNode.getTimerSetting();
      ts.timerStartHour = h;
      ts.timerStartMinutes = m;
      operationModeNode.setTimerSetting(ts);
    }
  } else if (top.endsWith("/timer-end/set")) {
    int h = value.substring(0, 2).toInt();
    int m = value.substring(3, 5).toInt();
    if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
      TimerSetting ts = operationModeNode.getTimerSetting();
      ts.timerEndHour = h;
      ts.timerEndMinutes = m;
      operationModeNode.setTimerSetting(ts);
    }
  } else if (top.endsWith("/timezone/set")) {
    int idx = getTimezoneIndexFromLabel(value);
    if (idx < 0) {
      Serial.printf("MQTT: Unknown timezone label \"%s\" — ignoring\n", value.c_str());
      publishStates();
      return;
    }
    ConfigManager::getSettings().timezoneIndex = idx;
    ConfigManager::save();
    // Apply timezone change to running clock immediately (P2 review fix)
    setTimezoneIndex(idx);
  } else if (top.endsWith("/ntp-server/set")) {
    if (value.length() > 0 && value.length() < 128) {
      ConfigManager::getNtp().server = value;
      ConfigManager::save();
      // Restart NTP client with new server immediately
      timeClientSetup(ConfigManager::getNtp().server.c_str());
    }
  } else if (top.endsWith("/solar-sensor/set") || top.endsWith("/pool-sensor/set")) {
    // Handle select entity: value is hex address or "— Not configured —"
    uint8_t addr[8] = {0};
    bool hasAddr = (value.length() >= 16);

    if (hasAddr) {
      // Extract address from first 16 hex chars of the option value
      for (uint8_t i = 0; i < 8; i++) {
        char byteStr[3] = {value[i * 2], value[i * 2 + 1], '\0'};
        char *end = nullptr;
        unsigned long val = strtoul(byteStr, &end, 16);
        if (end != byteStr + 2) {
          Serial.printf("MQTT: Invalid hex in sensor selection — ignoring\n");
          publishStates();
          return;
        }
        addr[i] = static_cast<uint8_t>(val);
      }
    }

    // Save to NVS
    {
      Preferences prefs;
      prefs.begin("ds18b20", false);
      if (top.endsWith("/solar-sensor/set")) {
        prefs.putBytes("solar_adr", addr, 8);
        if (hasAddr) solarTemperatureNode.setAddressFilter(addr);
        else solarTemperatureNode.clearAddressFilter();
        Serial.printf("MQTT: Solar sensor %s via HA\n", hasAddr ? "assigned" : "cleared");
      } else {
        prefs.putBytes("pool_adr", addr, 8);
        if (hasAddr) poolTemperatureNode.setAddressFilter(addr);
        else poolTemperatureNode.clearAddressFilter();
        Serial.printf("MQTT: Pool sensor %s via HA\n", hasAddr ? "assigned" : "cleared");
      }
      prefs.end();
    }
  }

  // Refresh states to confirm changes to Home Assistant
  publishStates();
}

}  // namespace PoolController
