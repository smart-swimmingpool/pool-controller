// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file MqttPublisher.cpp
 * @brief Home Assistant MQTT Discovery implementation — entity configs,
 *        state publishing, and command handling.
 */

#include "MqttPublisher.hpp"

#include <ArduinoJson.h>
#include <Preferences.h>
#include <memory>

#include "ConfigManager.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "NetworkManager.hpp"
#include "Nodes.hpp"
#include "OtaUpdater.hpp"
#include "OperationModeNode.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"
#include "Version.h"
#include "LogCapture.hpp"

namespace PoolController {

String MqttPublisher::deviceId_ = "";
std::uint32_t MqttPublisher::s_lastExportedSeq = 0;

void MqttPublisher::begin() {
  // Generate unique MAC-based device identifier
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "pool_controller_%02x%02x%02x", mac[3], mac[4], mac[5]);
  deviceId_ = String(macStr);

  // Start the log-event export watermark at the current ring position so the
  // pre-MQTT boot backlog (plain Info chatter) is not flooded onto the bus.
  s_lastExportedSeq = LogCapture::lastSeq();

  LOG_INFO("✓ HA Discovery Device ID set to: %s\n", deviceId_.c_str());

  // Register callback in NetworkManager
  NetworkManager::setMqttCallback(handleMqttMessage);
}

void MqttPublisher::addDeviceInfo(JsonDocument &doc) {
  // Common device block to consolidate all entities in HA
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = deviceId_;
  deviceObj["name"] = "Pool Controller";
  deviceObj["manufacturer"] = "smart-swimmingpool";
  deviceObj["model"] = "Pool Controller";
  deviceObj["sw_version"] = FW_VERSION;
}

void MqttPublisher::getBaseTopic(char *buf, size_t bufSize, const char *component, const char *objectId) {
  // homeassistant/<component>/pool-controller/<object-id>/config
  snprintf(buf, bufSize, "homeassistant/%s/pool-controller/%s", component, objectId);
}

// ── Helper to build a topic string into a static buffer and return a const char* ──
// Used for JSON document field assignment where a char buffer is needed per call.
namespace {
struct TopicBuilder {
  char buf[128];
  const char *build(const char *component, const char *objectId, const char *suffix) {
    snprintf(buf, sizeof(buf), "homeassistant/%s/pool-controller/%s%s", component, objectId, suffix);
    return buf;
  }
};

// Curated event whitelist — MUST match the event_types in publishEventDiscovery().
bool isKnownEventType(const char *type) {
  static const char *const kEventTypes[] = {
    "LOG_WARN", "LOG_ERROR", "MODE_CHANGED", "PUMP_ON", "PUMP_OFF",
    "WIFI_CONNECTED", "WIFI_DISCONNECTED", "MQTT_CONNECTED", "MQTT_DISCONNECTED"};
  for (const char *known : kEventTypes) {
    if (strcmp(known, type) == 0)
      return true;
  }
  return false;
}
}  // namespace

void MqttPublisher::publishSensorDiscovery(const char *objectId, const char *name, const char *deviceClass, const char *unit,
  const char *icon, const char *entityCategory, const char *stateClass) {
  TopicBuilder cfgTopic, stateTopic;
  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("sensor", objectId, "/state");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  if (deviceClass)
    doc["device_class"] = deviceClass;
  if (unit)
    doc["unit_of_measurement"] = unit;
  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  if (stateClass)
    doc["state_class"] = stateClass;
  if (deviceClass && strcmp(deviceClass, "temperature") == 0)
    doc["suggested_display_precision"] = 1;

  // Embedded device block - manually add device info
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("sensor", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishBinarySensorDiscovery(const char *objectId, const char *name, const char *payloadOn,
  const char *payloadOff, const char *deviceClass, const char *icon, const char *entityCategory) {
  TopicBuilder cfgTopic, stateTopic;

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("binary_sensor", objectId, "/state");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["payload_on"] = payloadOn;
  doc["payload_off"] = payloadOff;

  if (deviceClass)
    doc["device_class"] = deviceClass;
  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("binary_sensor", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishSwitchDiscovery(
  const char *objectId, const char *name, const char *icon, const char *entityCategory, const char *deviceClass) {
  TopicBuilder cfgTopic, stateTopic, cmdTopic;

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("switch", objectId, "/state");
  doc["command_topic"] = cmdTopic.build("switch", objectId, "/set");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";

  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  if (deviceClass)
    doc["device_class"] = deviceClass;
  // Embedded device block - manually add device info
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("switch", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishSelectDiscovery(const char *objectId, const char *name, const char *const *options, size_t optionCount,
  const char *icon, const char *entityCategory) {
  TopicBuilder cfgTopic, stateTopic, cmdTopic;

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("select", objectId, "/state");
  doc["command_topic"] = cmdTopic.build("select", objectId, "/set");
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
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("select", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishNumberDiscovery(const char *objectId, const char *name, double minVal, double maxVal, double step,
  const char *unit, const char *icon, const char *entityCategory) {
  TopicBuilder cfgTopic, stateTopic, cmdTopic;

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("number", objectId, "/state");
  doc["command_topic"] = cmdTopic.build("number", objectId, "/set");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["min"] = minVal;
  doc["max"] = maxVal;
  doc["step"] = step;
  doc["mode"] = "slider";

  if (unit)
    doc["unit_of_measurement"] = unit;
  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  // Embedded device block - manually add device info
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("number", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishTextDiscovery(const char *objectId, const char *name, const char *icon, const char *entityCategory) {
  TopicBuilder cfgTopic, stateTopic, cmdTopic;

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("text", objectId, "/state");
  doc["command_topic"] = cmdTopic.build("text", objectId, "/set");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  if (entityCategory)
    doc["entity_category"] = entityCategory;

  if (icon)
    doc["icon"] = icon;
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("text", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishEventDiscovery(const char *objectId, const char *name, const char *icon) {
  TopicBuilder cfgTopic, stateTopic;
  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("event", objectId, "/state");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["platform"] = "event";

  // Whitelist MUST match the curated event types parsed by exportLogEvents().
  JsonArray eventTypes = doc["event_types"].to<JsonArray>();
  eventTypes.add("LOG_WARN");
  eventTypes.add("LOG_ERROR");
  eventTypes.add("MODE_CHANGED");
  eventTypes.add("PUMP_ON");
  eventTypes.add("PUMP_OFF");
  eventTypes.add("WIFI_CONNECTED");
  eventTypes.add("WIFI_DISCONNECTED");
  eventTypes.add("MQTT_CONNECTED");
  eventTypes.add("MQTT_DISCONNECTED");

  if (icon)
    doc["icon"] = icon;
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("event", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishTimeDiscovery(const char *objectId, const char *name, const char *icon, const char *entityCategory) {
  TopicBuilder cfgTopic, stateTopic, cmdTopic;

  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("time", objectId, "/state");
  doc["command_topic"] = cmdTopic.build("time", objectId, "/set");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  if (icon)
    doc["icon"] = icon;
  if (entityCategory)
    doc["entity_category"] = entityCategory;
  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("time", objectId, "/config"), payloadBuf, true);
}

void MqttPublisher::publishUpdateDiscovery() {
  TopicBuilder cfgTopic, stateTopic, cmdTopic, attrTopic, latestTopic;

  JsonDocument doc;
  doc["name"] = "Firmware";
  doc["unique_id"] = deviceId_ + "_fw_update";
  doc["state_topic"] = stateTopic.build("update", "firmware-update", "/state");
  doc["command_topic"] = cmdTopic.build("update", "firmware-update", "/set");
  doc["json_attributes_topic"] = attrTopic.build("update", "firmware-update", "/attr");
  doc["latest_version_topic"] = latestTopic.build("update", "firmware-update", "/latest");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["payload_install"] = "INSTALL";
  doc["device_class"] = "firmware";
  doc["entity_category"] = "config";

  addDeviceInfo(doc);

  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("update", "firmware-update", "/config"), payloadBuf, true);
}

void MqttPublisher::publishClimateDiscovery() {
  TopicBuilder cfgTopic, currentTempTopic, tempCmdTopic, tempStateTopic, modeCmdTopic, modeStateTopic, actionTopic,
    presetCmdTopic, presetStateTopic;

  JsonDocument doc;
  doc["name"] = "Pool Thermostat";
  doc["unique_id"] = deviceId_ + "_thermostat";
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";

  // Temperatures
  doc["current_temperature_topic"] = currentTempTopic.build("climate", "thermostat", "/current-temperature/state");
  doc["temperature_command_topic"] = tempCmdTopic.build("climate", "thermostat", "/temperature/set");
  doc["temperature_state_topic"] = tempStateTopic.build("climate", "thermostat", "/temperature/state");
  doc["temperature_unit"] = "C";

  // HVAC modes
  JsonArray modes = doc["modes"].to<JsonArray>();
  modes.add("off");
  modes.add("auto");
  modes.add("heat");

  doc["mode_command_topic"] = modeCmdTopic.build("climate", "thermostat", "/mode/set");
  doc["mode_state_topic"] = modeStateTopic.build("climate", "thermostat", "/mode/state");

  // Action (heating / idle / off)
  doc["action_topic"] = actionTopic.build("climate", "thermostat", "/action/state");

  // Temp range
  doc["min_temp"] = 0.0;
  doc["max_temp"] = 40.0;
  doc["temp_step"] = 0.5;

  // Preset modes (sub-modes: manual/schedule/boost)
  // NOTE: "none" is RESERVED by HA MQTT climate schema and must NOT be
  // in this list — HA rejects the entire discovery config if present.
  // HA internally sets preset_mode to "none" when no preset is active.
  {
    JsonArray presets = doc["preset_modes"].to<JsonArray>();
    presets.add("manual");
    presets.add("schedule");
    presets.add("boost");
  }
  doc["preset_mode_command_topic"] = presetCmdTopic.build("climate", "thermostat", "/preset/set");
  doc["preset_mode_state_topic"] = presetStateTopic.build("climate", "thermostat", "/preset/state");

  // No entity_category — shown directly on device dashboard (users expect
  // a thermostat control on the front page, not hidden in config).
  // Device block
  addDeviceInfo(doc);

  char payloadBuf[1536];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("climate", "thermostat", "/config"), payloadBuf, true);
}

void MqttPublisher::publishClimateState() {
  if (!NetworkManager::isMqttConnected())
    return;

  char topic[128];
  char valBuf[32];

  // Current temperature (pool water)
  snprintf(topic, sizeof(topic), "homeassistant/climate/pool-controller/thermostat/current-temperature/state");
  snprintf(valBuf, sizeof(valBuf), "%.1f", poolTemperatureNode.getTemperature());
  NetworkManager::publish(topic, valBuf, true);

  // Target temperature (= pool max temp setting)
  snprintf(topic, sizeof(topic), "homeassistant/climate/pool-controller/thermostat/temperature/state");
  snprintf(valBuf, sizeof(valBuf), "%.1f", operationModeNode.getPoolMaxTemperature());
  NetworkManager::publish(topic, valBuf, true);

  // HVAC mode ← pool operation mode
  //   manu  → off
  //   auto  → auto
  //   boost → heat
  //   timer → auto (pool pump runs on schedule)
  snprintf(topic, sizeof(topic), "homeassistant/climate/pool-controller/thermostat/mode/state");
  const char *hvacMode = "off";
  {
    String poolMode = operationModeNode.getMode();
    if (poolMode == "auto" || poolMode == "timer") {
      hvacMode = "auto";
    } else if (poolMode == "boost") {
      hvacMode = "heat";
    } else {
      hvacMode = "off";
    }
  }
  NetworkManager::publish(topic, hvacMode, true);

  // Action: solar pump ON → heating, pool pump ON → circulating, both OFF → off
  snprintf(topic, sizeof(topic), "homeassistant/climate/pool-controller/thermostat/action/state");
  const char *action = "off";
  if (solarPumpNode.getSwitch()) {
    action = "heating";
  } else if (poolPumpNode.getSwitch()) {
    action = "circulating";
  } else {
    action = "off";
  }
  NetworkManager::publish(topic, action, true);

  // Preset mode ← pool operation mode
  {
    snprintf(topic, sizeof(topic), "homeassistant/climate/pool-controller/thermostat/preset/state");
    String poolMode = operationModeNode.getMode();
    const char *preset = "none";
    if (poolMode == "manu") {
      preset = "manual";
    } else if (poolMode == "boost") {
      preset = "boost";
    } else if (poolMode == "timer") {
      preset = "schedule";
    }
    NetworkManager::publish(topic, preset, true);
  }
}

void MqttPublisher::publishUpdateState() {
  if (!NetworkManager::isMqttConnected())
    return;

  char topic[128];

  // State topic: current installed version
  snprintf(topic, sizeof(topic), "homeassistant/update/pool-controller/firmware-update/state");
  NetworkManager::publish(topic, OtaUpdater::getCurrentVersion().c_str(), true);

  // Latest version topic: the newest available version (or current if up to date)
  snprintf(topic, sizeof(topic), "homeassistant/update/pool-controller/firmware-update/latest");
  String latestVer = OtaUpdater::isUpdateAvailable() ? OtaUpdater::getLatestVersion() : OtaUpdater::getCurrentVersion();
  NetworkManager::publish(topic, latestVer.c_str(), true);

  // Attributes topic: extra metadata
  snprintf(topic, sizeof(topic), "homeassistant/update/pool-controller/firmware-update/attr");

  JsonDocument doc;
  doc["installed_version"] = OtaUpdater::getCurrentVersion();
  doc["title"] = "Pool Controller Firmware";

  if (OtaUpdater::isUpdateAvailable()) {
    doc["latest_version"] = OtaUpdater::getLatestVersion();
    doc["release_url"] = OtaUpdater::getReleaseUrl();
  } else {
    doc["latest_version"] = OtaUpdater::getCurrentVersion();
  }

  char payloadBuf[512];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(topic, payloadBuf, true);
}

void MqttPublisher::publishDiscovery() {
  if (!NetworkManager::isMqttConnected())
    return;

  LOG_INFO("Publishing HA Discovery Payloads...\n");

  // ── Log event entity (HA "event" component) ──
  publishEventDiscovery("logs", "Pool Controller Logs", "mdi:clipboard-text-outline");

  // ── Primary Sensors (no entity_category — shown on device front page) ──
  publishSensorDiscovery("pool-temp", "Pool Temperature", "temperature", "°C", "mdi:pool", nullptr, "measurement");
  publishSensorDiscovery("solar-temp", "Solar Temperature", "temperature", "°C", "mdi:solar-power", nullptr, "measurement");

  // ── Diagnostics (entity_category: "diagnostic") ──
  publishSensorDiscovery(
    "controller-temp", "Controller Temperature", "temperature", "°C", "mdi:thermometer", "diagnostic", "measurement");
  publishSensorDiscovery("heap", "Free Heap Space", nullptr, "B", "mdi:memory", "diagnostic", "measurement");
  publishSensorDiscovery("max-alloc", "Max Alloc Block", nullptr, "B", "mdi:memory", "diagnostic", "measurement");
  publishSensorDiscovery("rssi", "WiFi Signal Strength", "signal_strength", "dBm", "mdi:wifi", "diagnostic", "measurement");
  publishSensorDiscovery("uptime", "System Uptime", "duration", "s", "mdi:clock-outline", "diagnostic", "total_increasing");
  publishSensorDiscovery("local-time", "Local Time", nullptr, nullptr, "mdi:clock", "diagnostic");

  // ── Controls (no entity_category — shown on device front page) ──
  // Relays (Switches)
  publishSwitchDiscovery("pool-pump", "Pool Pump", "mdi:pump", nullptr, "outlet");
  publishSwitchDiscovery("solar-pump", "Solar Pump", "mdi:solar-panel", nullptr, "outlet");

  // Select Mode
  const char *modeOpts[] = {"auto", "manu", "boost", "timer"};
  publishSelectDiscovery("mode", "Operation Mode", modeOpts, 4, "mdi:sync");

  // ── Configuration (entity_category: "config") ──
  // Parameter Numbers
  publishNumberDiscovery(
    "pool-max-temp", "Maximum Pool Temperature", 0.0, 40.0, 0.1, "°C", "mdi:thermometer-chevron-up", "config");
  publishNumberDiscovery(
    "solar-min-temp", "Minimum Solar Temperature", 0.0, 100.0, 0.1, "°C", "mdi:thermometer-chevron-down", "config");
  publishNumberDiscovery("hysteresis", "Temperature Hysteresis", 0.0, 10.0, 0.1, "K", "mdi:delta", "config");

  // Temperature-based circulation parameters
  publishNumberDiscovery(
    "temp-circ-threshold", "Circulation Temperature Threshold", 0.0, 40.0, 0.5, "°C", "mdi:thermometer-auto", "config");
  publishNumberDiscovery(
    "temp-circ-factor", "Circulation Temperature Factor", 0.0, 120.0, 5.0, "min/°C", "mdi:plus-minus", "config");
  publishNumberDiscovery(
    "temp-circ-max-runtime", "Circulation Maximum Runtime", 60.0, 1440.0, 15.0, "min", "mdi:timer-outline", "config");

  // Timer as Time entities (HH:MM:SS format)
  publishTimeDiscovery("timer-start", "Timer Start", "mdi:clock-start", "config");
  publishTimeDiscovery("timer-end", "Timer End", "mdi:clock-end", "config");

  // Select Timezone
  publishSelectDiscovery("timezone", "Timezone", getTimezoneLabelList(), getTimezoneLabelCount(), "mdi:map-clock", "config");

  // Text entities
  publishTextDiscovery("ntp-server", "NTP Server", "mdi:clock-outline", "config");

  // Runtime diagnostics
  publishSensorDiscovery(
    "effective-runtime", "Effective Runtime", "duration", "s", "mdi:timer-sand", "diagnostic", "measurement");
  publishSensorDiscovery(
    "circulation-extension", "Circulation Extension", "duration", "s", "mdi:timer-plus", "diagnostic", "measurement");

  // ── Sensor mapping diagnostics (static entities, always available) ──
  publishBinarySensorDiscovery(
    "solar-sensor-found", "Solar Sensor Found", "Found", "Missing", "connectivity", "mdi:check-network-outline", "diagnostic");
  publishBinarySensorDiscovery(
    "pool-sensor-found", "Pool Sensor Found", "Found", "Missing", "connectivity", "mdi:check-network-outline", "diagnostic");

  // MQTT Connection status
  publishBinarySensorDiscovery("mqtt-status", "MQTT Connected", "ON", "OFF", "connectivity", "mdi:connection", "diagnostic");

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
  NetworkManager::subscribe("homeassistant/climate/pool-controller/thermostat/preset/set");

  // ── Cleanup old retained discovery configs (migrated entities) ──
  // Timer was 4 Number entities → now 2 Time entities
  static const char *kOldConfigTopics[] = {
    "homeassistant/number/pool-controller/timer-start-h/config",
    "homeassistant/number/pool-controller/timer-start-min/config",
    "homeassistant/number/pool-controller/timer-end-h/config",
    "homeassistant/number/pool-controller/timer-end-min/config",
    // Timezone was Number → now Select
    "homeassistant/number/pool-controller/timezone/config",
    // Sensor-found were Sensor → now Binary Sensor
    "homeassistant/sensor/pool-controller/solar-sensor-found/config",
    "homeassistant/sensor/pool-controller/pool-sensor-found/config",
  };
  for (auto &topic : kOldConfigTopics) {
    NetworkManager::publish(topic, "", true);  // empty retained → HA removes entity
  }

  LOG_INFO("✓ HA Discovery Payloads & Subscriptions complete\n");
}

void MqttPublisher::publishStates() {
  if (!NetworkManager::isMqttConnected())
    return;

  // Buffer for topic construction — reused for each publish call
  char topic[128];
  char valBuf[32];

  // Temperature States
  getBaseTopic(topic, sizeof(topic), "sensor", "pool-temp");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%.1f", poolTemperatureNode.getTemperature());
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "sensor", "solar-temp");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%.1f", solarTemperatureNode.getTemperature());
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "sensor", "controller-temp");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%.1f", ctrlTemperatureNode.getTemperature());
  NetworkManager::publish(topic, valBuf, true);

  // Diagnostic States
  getBaseTopic(topic, sizeof(topic), "sensor", "heap");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%u", ESP.getFreeHeap());
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "sensor", "max-alloc");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%u", ESP.getMaxAllocHeap());
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "sensor", "rssi");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%d", NetworkManager::getWiFiRSSI());
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "sensor", "uptime");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%u", millis() / 1000);
  NetworkManager::publish(topic, valBuf, true);

  // Local time state
  {
    TimeChangeRule *tcr;
    time_t localTime = getTimeFor(ConfigManager::getSettings().timezoneIndex, &tcr);
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d", year(localTime), month(localTime), day(localTime),
      hour(localTime), minute(localTime), second(localTime));
    getBaseTopic(topic, sizeof(topic), "sensor", "local-time");
    strlcat(topic, "/state", sizeof(topic));
    NetworkManager::publish(topic, timeBuf, true);
  }

  // Switch States
  getBaseTopic(topic, sizeof(topic), "switch", "pool-pump");
  strlcat(topic, "/state", sizeof(topic));
  NetworkManager::publish(topic, poolPumpNode.getSwitch() ? "ON" : "OFF", true);

  getBaseTopic(topic, sizeof(topic), "switch", "solar-pump");
  strlcat(topic, "/state", sizeof(topic));
  NetworkManager::publish(topic, solarPumpNode.getSwitch() ? "ON" : "OFF", true);

  // Firmware Update
  publishUpdateState();

  // Climate thermostat
  publishClimateState();

  // Mode & Parameter States
  getBaseTopic(topic, sizeof(topic), "select", "mode");
  strlcat(topic, "/state", sizeof(topic));
  NetworkManager::publish(topic, operationModeNode.getMode().c_str(), true);

  getBaseTopic(topic, sizeof(topic), "number", "pool-max-temp");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%.1f", operationModeNode.getPoolMaxTemperature());
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "number", "solar-min-temp");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%.1f", operationModeNode.getSolarMinTemperature());
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "number", "hysteresis");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%.1f", operationModeNode.getTemperatureHysteresis());
  NetworkManager::publish(topic, valBuf, true);

  {
    TimerSetting ts = operationModeNode.getTimerSetting();
    char timeBuf[9];
    getBaseTopic(topic, sizeof(topic), "time", "timer-start");
    strlcat(topic, "/state", sizeof(topic));
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:00", ts.timerStartHour, ts.timerStartMinutes);
    NetworkManager::publish(topic, timeBuf, true);

    getBaseTopic(topic, sizeof(topic), "time", "timer-end");
    strlcat(topic, "/state", sizeof(topic));
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:00", ts.timerEndHour, ts.timerEndMinutes);
    NetworkManager::publish(topic, timeBuf, true);
  }

  // Temperature-based circulation parameters
  getBaseTopic(topic, sizeof(topic), "number", "temp-circ-threshold");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%.1f", ConfigManager::getSettings().tempCircThreshold);
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "number", "temp-circ-factor");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%u", ConfigManager::getSettings().tempCircFactor);
  NetworkManager::publish(topic, valBuf, true);

  getBaseTopic(topic, sizeof(topic), "number", "temp-circ-max-runtime");
  strlcat(topic, "/state", sizeof(topic));
  snprintf(valBuf, sizeof(valBuf), "%u", ConfigManager::getSettings().tempCircMaxRuntime);
  NetworkManager::publish(topic, valBuf, true);

  // Effective runtime sensor — actual runtime, published in seconds (for HA duration display)
  {
    Rule *active = operationModeNode.getRule();
    uint16_t effectiveMin = (active != nullptr) ? active->getEffectiveRuntimeMinutes() : 0;
    getBaseTopic(topic, sizeof(topic), "sensor", "effective-runtime");
    strlcat(topic, "/state", sizeof(topic));
    snprintf(valBuf, sizeof(valBuf), "%u", static_cast<uint32_t>(effectiveMin) * 60);
    NetworkManager::publish(topic, valBuf, true);
  }

  // Circulation extension sensor — extra minutes beyond base timer (in seconds for HA duration)
  {
    Rule *active = operationModeNode.getRule();
    uint16_t extensionMin = (active != nullptr) ? active->getCirculationExtensionMinutes() : 0;
    getBaseTopic(topic, sizeof(topic), "sensor", "circulation-extension");
    strlcat(topic, "/state", sizeof(topic));
    snprintf(valBuf, sizeof(valBuf), "%u", static_cast<uint32_t>(extensionMin) * 60);
    NetworkManager::publish(topic, valBuf, true);
  }

  getBaseTopic(topic, sizeof(topic), "select", "timezone");
  strlcat(topic, "/state", sizeof(topic));
  NetworkManager::publish(topic, getTimeInfoFor(ConfigManager::getSettings().timezoneIndex).c_str(), true);

  getBaseTopic(topic, sizeof(topic), "text", "ntp-server");
  strlcat(topic, "/state", sizeof(topic));
  NetworkManager::publish(topic, ConfigManager::getNtp().server.c_str(), true);

  // Sensor mapping states
  {
    char addrBuf[17];
    getBaseTopic(topic, sizeof(topic), "select", "solar-sensor");
    strlcat(topic, "/state", sizeof(topic));
    if (solarTemperatureNode.hasAddressFilter()) {
      solarTemperatureNode.getDeviceAddressString(addrBuf, sizeof(addrBuf));
      NetworkManager::publish(topic, addrBuf, true);
    } else {
      NetworkManager::publish(topic, "— Not configured —", true);
    }

    getBaseTopic(topic, sizeof(topic), "select", "pool-sensor");
    strlcat(topic, "/state", sizeof(topic));
    if (poolTemperatureNode.hasAddressFilter()) {
      poolTemperatureNode.getDeviceAddressString(addrBuf, sizeof(addrBuf));
      NetworkManager::publish(topic, addrBuf, true);
    } else {
      NetworkManager::publish(topic, "— Not configured —", true);
    }

    // Sensor-found binary indicators (binary_sensor → "Found"/"Missing" payload)
    getBaseTopic(topic, sizeof(topic), "binary_sensor", "solar-sensor-found");
    strlcat(topic, "/state", sizeof(topic));
    NetworkManager::publish(topic, solarTemperatureNode.isSensorFound() ? "Found" : "Missing", true);

    getBaseTopic(topic, sizeof(topic), "binary_sensor", "pool-sensor-found");
    strlcat(topic, "/state", sizeof(topic));
    NetworkManager::publish(topic, poolTemperatureNode.isSensorFound() ? "Found" : "Missing", true);

    // MQTT connection status
    getBaseTopic(topic, sizeof(topic), "binary_sensor", "mqtt-status");
    strlcat(topic, "/state", sizeof(topic));
    NetworkManager::publish(topic, NetworkManager::isMqttConnected() ? "ON" : "OFF", true);

    // Export new log entries as MQTT events (WARN/ERROR + curated logEvent markers)
    exportLogEvents();
  }
}

// ═══════════════════════════════════════════════════════════════════════
// Log-event export pump (WARN/ERROR + curated logEvent markers)
// ═══════════════════════════════════════════════════════════════════════

void MqttPublisher::exportLogEvents() {
  // Batch snapshot of the ring (static: keeps ~9 KB off the loop stack).
  static LogEntry entries[LogCapture::LOG_BUFFER_ENTRIES];
  const size_t count = LogCapture::getEntries(
    s_lastExportedSeq, LogCapture::LOG_BUFFER_ENTRIES, LogLevel::Info, entries, LogCapture::LOG_BUFFER_ENTRIES);
  if (count == 0)
    return;

  TopicBuilder stateTopic;
  for (size_t i = 0; i < count; ++i) {
    const LogEntry &entry = entries[i];
    const char *body = entry.message;
    const char *eventType = nullptr;
    char typeBuf[32];

    // Parse the "[TYPE] message" marker written by LogCapture::logEvent().
    if (body[0] == '[') {
      const char *close = strchr(body, ']');
      if (close != nullptr && close > body + 1) {
        const size_t len = static_cast<size_t>(close - body - 1);
        if (len < sizeof(typeBuf)) {
          memcpy(typeBuf, body + 1, len);
          typeBuf[len] = '\0';
          if (isKnownEventType(typeBuf)) {
            eventType = typeBuf;
            body = close + 1;
            while (*body == ' ')
              ++body;
          }
        }
      }
    }

    // WARN/ERROR entries without a curated marker → LOG_WARN/LOG_ERROR.
    // Info/Debug chatter is skipped (volume control — nothing on MQTT).
    if (eventType == nullptr) {
      switch (entry.level) {
        case LogLevel::Warning: eventType = "LOG_WARN"; break;
        case LogLevel::Error: eventType = "LOG_ERROR"; break;
        default: continue;
      }
    }

    // HA event-entity state: {"event_type": "...", "message": "..."}
    JsonDocument doc;
    doc["event_type"] = eventType;
    doc["message"] = body;
    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    NetworkManager::publish(stateTopic.build("event", "logs", "/state"), payload, false);

    // Raw JSON-line for external tools (WARN/ERROR only — not Info-level events).
    if (entry.level == LogLevel::Warning || entry.level == LogLevel::Error) {
      JsonDocument rawDoc;
      rawDoc["seq"] = entry.seq;
      rawDoc["t"] = entry.uptimeMs;
      rawDoc["level"] = LogCapture::levelName(entry.level);
      rawDoc["msg"] = entry.message;
      char rawBuf[256];
      serializeJson(rawDoc, rawBuf, sizeof(rawBuf));
      NetworkManager::publish("pool-controller/log", rawBuf, false);
    }
  }

  s_lastExportedSeq = LogCapture::lastSeq();
}

// ═══════════════════════════════════════════════════════════════════════
// Sensor mapping select-entity discovery (published after bus scan)
// ═══════════════════════════════════════════════════════════════════════

void MqttPublisher::publishSensorMappingDiscovery() {
  // Collect unique detected addresses
  uint8_t maxDev = max(solarTemperatureNode.getDeviceCount(), poolTemperatureNode.getDeviceCount());
  if (maxDev == 0)
    return;

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
    if (solarTemperatureNode.getDetectedDeviceAddress(i, addr) || poolTemperatureNode.getDetectedDeviceAddress(i, addr)) {
      char buf[17];
      snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
        addr[6], addr[7]);

      // Deduplicate (same address may appear on shared bus)
      bool seen = false;
      for (uint8_t si = 0; si < storedCount; si++) {
        if (storedAddrs[si] == buf) {
          seen = true;
          break;
        }
      }
      if (seen)
        continue;

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

  LOG_INFO("• HA: Sensor mapping select entities published (%u options)\n", solarOptCount);
}

// Helper function to check if MQTT authentication is configured
static bool isMqttAuthenticated() {
  // Check if MQTT connection is using authentication
  const MqttConfig &config = ConfigManager::getMqtt();
  // If MQTT username is set, we consider it authenticated
  return config.username.length() > 0;
}

// Command validation - public static method for testing
bool MqttPublisher::isValidCommand(const String &value, const char *const validCommands[], size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (value == validCommands[i]) {
      return true;
    }
  }
  return false;
}

// Helper function to check if MQTT authentication is required for sensitive commands
static bool shouldEnforceMqttAuth() {
  // For now, we make MQTT authentication optional but recommended
  // Users can enable it by setting a username in MQTT config
  // In the future, this could be made configurable via settings
  return false;  // Made optional as per user request
}

void MqttPublisher::handleMqttMessage(
  char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  // Only process complete messages (single-chunk delivery for typical HA commands)
  if (index != 0) {
    return;
  }

  // Convert payload to String safely (AsyncMqttClient null-terminates)
  String value(payload, len);
  String top(topic);

  if (top.endsWith("/firmware-update/set")) {
    // MQTT authentication is optional, but if configured, we check it
    if (shouldEnforceMqttAuth() && !isMqttAuthenticated()) {
      LOG_WARN("MQTT: Firmware update command rejected - MQTT authentication required\n");
      return;
    }

    // Always validate command value for security
    static const char *validFirmwareCommands[] = {"INSTALL"};
    if (!MqttPublisher::isValidCommand(value, validFirmwareCommands, 1)) {
      LOG_WARN("MQTT: Invalid firmware command: %s\n", value.c_str());
      return;
    }

    if (value == "INSTALL") {
      LOG_INFO("MQTT: Firmware update triggered from Home Assistant\n");
      OtaUpdater::startUpdate();
    }
    return;
  }

  // Climate thermostat commands (mode + temperature + preset)
  if (top.endsWith("/thermostat/preset/set")) {
    String poolMode;
    if (value == "none")
      poolMode = "auto";
    else if (value == "manual")
      poolMode = "manu";
    else if (value == "schedule")
      poolMode = "timer";
    else if (value == "boost")
      poolMode = "boost";
    else {
      LOG_WARN("MQTT: Unknown preset \"%s\" — ignoring\n", value.c_str());
      publishStates();
      return;
    }
    LOG_INFO("MQTT: Climate preset → pool mode \"%s\"\n", poolMode.c_str());
    operationModeNode.setMode(poolMode.c_str());
    ConfigManager::getSettings().opMode = poolMode;
    ConfigManager::save();
    publishStates();
    return;
  }

  if (top.endsWith("/thermostat/mode/set")) {
    String poolMode;
    if (value == "off")
      poolMode = "manu";
    else if (value == "auto")
      poolMode = "auto";
    else if (value == "heat")
      poolMode = "boost";
    else {
      LOG_WARN("MQTT: Unknown climate mode \"%s\" — ignoring\n", value.c_str());
      publishStates();
      return;
    }
    LOG_INFO("MQTT: Climate mode → pool mode \"%s\"\n", poolMode.c_str());
    operationModeNode.setMode(poolMode.c_str());
    ConfigManager::getSettings().opMode = poolMode;
    ConfigManager::save();
    publishStates();
    return;
  }

  if (top.endsWith("/thermostat/temperature/set")) {
    float val = value.toFloat();
    LOG_INFO("MQTT: Climate target temperature → %.1f\n", val);
    operationModeNode.setPoolMaxTemperature(val);
    ConfigManager::getSettings().tempMaxPool = val;
    ConfigManager::save();
    publishStates();
    return;
  }

  if (top.endsWith("/pool-pump/set") || top.endsWith("/solar-pump/set")) {
    // MQTT authentication is optional, but if configured, we check it
    if (shouldEnforceMqttAuth() && !isMqttAuthenticated()) {
      LOG_WARN("MQTT: Pump command rejected - MQTT authentication required\n");
      publishStates();
      return;
    }

    // Always validate payload for security
    static const char *validPumpCommands[] = {"ON", "OFF"};
    if (!MqttPublisher::isValidCommand(value, validPumpCommands, 2)) {
      LOG_WARN("MQTT: Invalid pump command: %s\n", value.c_str());
      publishStates();
      return;
    }

    // Only allow pump control from HA in manual mode
    if (operationModeNode.getMode() != "manu") {
      LOG_WARN("MQTT: Ignoring pump command — not in manual mode (current: %s)\n", operationModeNode.getMode().c_str());
      publishStates();
      return;
    }
    if (top.endsWith("/pool-pump/set")) {
      poolPumpNode.setSwitch(value == "ON");
    } else {
      solarPumpNode.setSwitch(value == "ON");
    }
  } else if (top.endsWith("/mode/set")) {
    // MQTT authentication is optional, but if configured, we check it
    if (shouldEnforceMqttAuth() && !isMqttAuthenticated()) {
      LOG_WARN("MQTT: Mode command rejected - MQTT authentication required\n");
      publishStates();
      return;
    }

    // Always validate mode value for security
    static const char *validModes[] = {"auto", "manu", "boost", "timer"};
    if (!MqttPublisher::isValidCommand(value, validModes, 4)) {
      LOG_WARN("MQTT: Invalid mode command: %s\n", value.c_str());
      publishStates();
      return;
    }

    operationModeNode.setMode(value.c_str());
    ConfigManager::getSettings().opMode = value;
    ConfigManager::save();
  } else if (top.endsWith("/pool-max-temp/set")) {
    // MQTT authentication is optional, but if configured, we check it
    if (shouldEnforceMqttAuth() && !isMqttAuthenticated()) {
      LOG_WARN("MQTT: Config command rejected - MQTT authentication required\n");
      publishStates();
      return;
    }

    float val = value.toFloat();
    // Always validate range for security
    if (val < 0.0f || val > 40.0f) {
      LOG_WARN("MQTT: Invalid pool-max-temp value: %.1f\n", val);
      publishStates();
      return;
    }
    operationModeNode.setPoolMaxTemperature(val);
    ConfigManager::getSettings().tempMaxPool = val;
    ConfigManager::save();
  } else if (top.endsWith("/solar-min-temp/set")) {
    // MQTT authentication is optional, but if configured, we check it
    if (shouldEnforceMqttAuth() && !isMqttAuthenticated()) {
      LOG_WARN("MQTT: Config command rejected - MQTT authentication required\n");
      publishStates();
      return;
    }

    float val = value.toFloat();
    // Always validate range for security
    if (val < 0.0f || val > 100.0f) {
      LOG_WARN("MQTT: Invalid solar-min-temp value: %.1f\n", val);
      publishStates();
      return;
    }
    operationModeNode.setSolarMinTemperature(val);
    ConfigManager::getSettings().tempMinSolar = val;
    ConfigManager::save();
  } else if (top.endsWith("/hysteresis/set")) {
    // MQTT authentication is optional, but if configured, we check it
    if (shouldEnforceMqttAuth() && !isMqttAuthenticated()) {
      LOG_WARN("MQTT: Config command rejected - MQTT authentication required\n");
      publishStates();
      return;
    }

    float val = value.toFloat();
    // Always validate range for security
    if (val < 0.0f || val > 10.0f) {
      LOG_WARN("MQTT: Invalid hysteresis value: %.1f\n", val);
      publishStates();
      return;
    }
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
      LOG_WARN("MQTT: Unknown timezone label \"%s\" — ignoring\n", value.c_str());
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
          LOG_WARN("MQTT: Invalid hex in sensor selection — ignoring\n");
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
        if (hasAddr)
          solarTemperatureNode.setAddressFilter(addr);
        else
          solarTemperatureNode.clearAddressFilter();
        LOG_INFO("MQTT: Solar sensor %s via HA\n", hasAddr ? "assigned" : "cleared");
      } else {
        prefs.putBytes("pool_adr", addr, 8);
        if (hasAddr)
          poolTemperatureNode.setAddressFilter(addr);
        else
          poolTemperatureNode.clearAddressFilter();
        LOG_INFO("MQTT: Pool sensor %s via HA\n", hasAddr ? "assigned" : "cleared");
      }
      prefs.end();
    }
  }

  // Refresh states to confirm changes to Home Assistant
  publishStates();
}

}  // namespace PoolController
