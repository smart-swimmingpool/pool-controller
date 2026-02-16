// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "PoolController.hpp"

#include <Arduino.h>
#include <Homie.h>
#include <SPI.h>
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"
#include "Rule.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
#include "RuleTimer.hpp"

#include "LoggerNode.hpp"
#include "TimeClientHelper.hpp"
#include "StateManager.hpp"
#include "SystemMonitor.hpp"
#include "HomeAssistantMQTT.hpp"

#include "Config.hpp"

namespace PoolController {
static LoggerNode            LN;
static DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVALL);
static DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVALL);
#ifdef ESP32
static ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVALL);
#endif
static RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
static RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);

static OperationModeNode operationModeNode("operation-mode", "Operation Mode");

static uint32_t _measurementInterval = 10;
static uint32_t _lastMeasurement;

/**
 * MQTT message callback for Home Assistant switch commands
 */
static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if (!HomeAssistant::useHomeAssistant)
    return;

  // Check if this is a Home Assistant switch command
  if (strstr(topic, "homeassistant/switch/pool-controller/") != nullptr && strstr(topic, "/set") != nullptr) {
    // Extract the object ID from the topic
    // Topic format: homeassistant/switch/pool-controller/<object-id>/set
    char* objectIdStart = strstr(topic, "pool-controller/") + 16;
    char* objectIdEnd = strstr(objectIdStart, "/set");
    if (objectIdStart && objectIdEnd) {
      size_t objectIdLen = objectIdEnd - objectIdStart;
      char objectId[32];
      if (objectIdLen < sizeof(objectId)) {
        strncpy(objectId, objectIdStart, objectIdLen);
        objectId[objectIdLen] = '\0';

        // Null-terminate payload
        char payloadStr[16];
        size_t payloadLen = (len < sizeof(payloadStr) - 1) ? len : sizeof(payloadStr) - 1;
        strncpy(payloadStr, payload, payloadLen);
        payloadStr[payloadLen] = '\0';

        // Determine state
        bool state = (strcmp(payloadStr, "ON") == 0);

        // Route to appropriate relay
        if (strcmp(objectId, "pool-pump") == 0) {
          poolPumpNode.setSwitch(state);
        } else if (strcmp(objectId, "solar-pump") == 0) {
          solarPumpNode.setSwitch(state);
        }
      }
    }
  }
}

static PoolControllerContext* Self;
auto                          Detail::setupProxy() -> void {
  Self->setupHandler();
}
static PoolControllerContext* Self;
auto                          Detail::setupProxy() -> void {
  Self->setupHandler();
}

PoolControllerContext::PoolControllerContext() {
  assert(!Self);
  Self = this;
}
PoolControllerContext::PoolControllerContext() {
  assert(!Self);
  Self = this;
}

PoolControllerContext::~PoolControllerContext() {
  assert(Self);
  Self = nullptr;
}
PoolControllerContext::~PoolControllerContext() {
  assert(Self);
  Self = nullptr;
}

/**
 * Initialize controller components that don't require WiFi/MQTT.
 * This is called regardless of connection status to ensure offline operation.
 */
auto PoolControllerContext::initializeController() -> void {
  // set measurement intervals
  const std::uint32_t _loopInterval = this->loopIntervalSetting_.get();

  // Initialize NTP client with configured server
  timeClientSetup(this->ntpServerSetting_.get());

  // Set the timezone from configuration
  setTimezoneIndex(this->timezoneSetting_.get());

  solarTemperatureNode.setMeasurementInterval(_loopInterval);
  poolTemperatureNode.setMeasurementInterval(_loopInterval);
  solarTemperatureNode.setMeasurementInterval(_loopInterval);
  poolTemperatureNode.setMeasurementInterval(_loopInterval);

  poolPumpNode.setMeasurementInterval(_loopInterval);
  solarPumpNode.setMeasurementInterval(_loopInterval);
  poolPumpNode.setMeasurementInterval(_loopInterval);
  solarPumpNode.setMeasurementInterval(_loopInterval);

#ifdef ESP32
  ctrlTemperatureNode.setMeasurementInterval(_loopInterval);
#endif
#ifdef ESP32
  ctrlTemperatureNode.setMeasurementInterval(_loopInterval);
#endif

  operationModeNode.setMode(this->operationModeSetting_.get());
  operationModeNode.setPoolMaxTemperature(this->temperatureMaxPoolSetting_.get());
  operationModeNode.setSolarMinTemperature(this->temperatureMinSolarSetting_.get());
  operationModeNode.setTemperatureHysteresis(this->temperatureHysteresisSetting_.get());
  TimerSetting ts      = operationModeNode.getTimerSetting();  //TODO: Configurable
  ts.timerStartHour    = 10;
  ts.timerStartMinutes = 30;
  ts.timerEndHour      = 17;
  ts.timerEndMinutes   = 30;
  operationModeNode.setTimerSetting(ts);
  operationModeNode.setMode(this->operationModeSetting_.get());
  operationModeNode.setPoolMaxTemperature(this->temperatureMaxPoolSetting_.get());
  operationModeNode.setSolarMinTemperature(this->temperatureMinSolarSetting_.get());
  operationModeNode.setTemperatureHysteresis(this->temperatureHysteresisSetting_.get());
  TimerSetting ts      = operationModeNode.getTimerSetting();  //TODO: Configurable
  ts.timerStartHour    = 10;
  ts.timerStartMinutes = 30;
  ts.timerEndHour      = 17;
  ts.timerEndMinutes   = 30;
  operationModeNode.setTimerSetting(ts);

  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);
  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

  // add the rules
  RuleAuto* autoRule = new RuleAuto(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(autoRule);
  // add the rules
  RuleAuto* autoRule = new RuleAuto(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(autoRule);

  RuleManu* manuRule = new RuleManu();
  operationModeNode.addRule(manuRule);
  RuleManu* manuRule = new RuleManu();
  operationModeNode.addRule(manuRule);

  RuleBoost* boostRule = new RuleBoost(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(boostRule);
  RuleBoost* boostRule = new RuleBoost(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(boostRule);

  RuleTimer* timerRule = new RuleTimer(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(timerRule);
  RuleTimer* timerRule = new RuleTimer(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(timerRule);

  _lastMeasurement = 0;
}

/**
 * Homie Setup handler.
 * Only called when wifi and mqtt are connected.
 * Non-network-dependent initialization is now in initializeController().
 */
auto PoolControllerContext::setupHandler() -> void {
  // Initialize state management
  StateManager::begin();

  // Initialize system monitor and watchdog
  SystemMonitor::begin();

  // Load persisted state
  operationModeNode.loadState();

  // Configure MQTT protocol based on setting
  const char* protocol = this->mqttProtocolSetting_.get();
  HomeAssistant::useHomeAssistant = (std::strcmp(protocol, "homeassistant") == 0);

  if (HomeAssistant::useHomeAssistant) {
    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Using Home Assistant MQTT Discovery");

    // Register MQTT message callback for Home Assistant
    Homie.getMqttClient().onMessage(onMqttMessage);

    // Publish Home Assistant discovery messages for all sensors and switches
    const char* deviceId = "pool-controller";

    // Temperature sensors
    HomeAssistant::DiscoveryPublisher::publishSensor(
        deviceId, "solar-temp", "Solar Temperature",
        "temperature", "°C", "mdi:solar-power");

    HomeAssistant::DiscoveryPublisher::publishSensor(
        deviceId, "pool-temp", "Pool Temperature",
        "temperature", "°C", "mdi:pool");

#ifdef ESP32
    HomeAssistant::DiscoveryPublisher::publishSensor(
        deviceId, "controller-temp", "Controller Temperature",
        "temperature", "°C", "mdi:thermometer");
#endif

    // Switches (relays) - publish discovery and subscribe to command topics
    HomeAssistant::DiscoveryPublisher::publishSwitch(
        deviceId, "pool-pump", "Pool Pump", "mdi:pump");
    HomeAssistant::DiscoveryPublisher::subscribeSwitch(deviceId, "pool-pump");

    HomeAssistant::DiscoveryPublisher::publishSwitch(
        deviceId, "solar-pump", "Solar Pump", "mdi:solar-panel");
    HomeAssistant::DiscoveryPublisher::subscribeSwitch(deviceId, "solar-pump");

    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Home Assistant discovery messages published");
  } else {
    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Using Homie MQTT Convention");
  }

  LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "State persistence and system monitoring initialized");
}

auto PoolControllerContext::setup() -> void {
  Homie.setLoggingPrinter(&Serial);
auto PoolControllerContext::setup() -> void {
  Homie.setLoggingPrinter(&Serial);

  Homie_setFirmware("pool-controller", "3.1.0");
  Homie_setBrand("smart-swimmingpool");

  // default interval of sending Temperature values
  this->loopIntervalSetting_.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](const int32_t candidate) -> bool {
    return candidate >= 0 && candidate <= 300;
  });

  this->timezoneSetting_.setDefaultValue(0).setValidator([](const long candidate) -> bool {
    return candidate >= 0 && candidate < getTzCount();
  });

  this->ntpServerSetting_.setDefaultValue("pool.ntp.org").setValidator([](const char* const candidate) -> bool {
    return candidate != nullptr && strlen(candidate) > 0;
  });

  this->temperatureMaxPoolSetting_.setDefaultValue(28.5).setValidator(
      [](const double candidate) -> bool { return candidate >= 0 && candidate <= 30; });

  this->temperatureMinSolarSetting_.setDefaultValue(55.0).setValidator(
      [](const double candidate) noexcept -> bool { return candidate >= 0 && candidate <= 100; });

  this->temperatureHysteresisSetting_.setDefaultValue(1.0).setValidator(
      [](const double candidate) -> bool { return candidate >= 0 && candidate <= 10; });

  this->operationModeSetting_.setDefaultValue("auto").setValidator([](const char* const candidate) -> bool {
    return std::strcmp(candidate, "auto") == 0 || std::strcmp(candidate, "manu") == 0 || std::strcmp(candidate, "boost") == 0;
  });

  this->mqttProtocolSetting_.setDefaultValue("homie").setValidator([](const char* const candidate) -> bool {
    return std::strcmp(candidate, "homie") == 0 || std::strcmp(candidate, "homeassistant") == 0;
  });

  Homie.setSetupFunction(&Detail::setupProxy);
  Homie.setSetupFunction(&Detail::setupProxy);

  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  Homie.setup();

  // Initialize controller regardless of WiFi/MQTT connection status
  // This ensures offline operation works from startup
  initializeController();
  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  Homie.setup();

  // Initialize controller regardless of WiFi/MQTT connection status
  // This ensures offline operation works from startup
  initializeController();

  LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Free heap: %d", ESP.getFreeHeap());
  Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << endl;
}
  LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Free heap: %d", ESP.getFreeHeap());
  Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << endl;
}

auto PoolControllerContext::loop() -> void {
  // Feed watchdog and check memory
  SystemMonitor::feedWatchdog();
  SystemMonitor::checkMemory();

  Homie.loop();
}
}  // namespace PoolController
