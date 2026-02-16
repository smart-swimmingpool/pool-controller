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
#include "MqttInterface.hpp"

#include "Config.hpp"

namespace PoolController {
static LoggerNode            LN;
static DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVAL);
static DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVAL);
#ifdef ESP32
static ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVAL);
#endif
static RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
static RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);

static OperationModeNode operationModeNode("operation-mode", "Operation Mode");

static uint32_t _measurementInterval = 10;
static uint32_t _lastMeasurement;

static bool extractHomeAssistantObjectId(const char* topic, const char* component,
                                         char* objectId, size_t objectIdSize) {
  char prefix[128];
  snprintf(prefix, sizeof(prefix), "homeassistant/%s/pool-controller/", component);
  const size_t prefixLen = strlen(prefix);
  if (strncmp(topic, prefix, prefixLen) != 0) {
    return false;
  }

  const char* objectIdStart = topic + prefixLen;
  const char* objectIdEnd = strstr(objectIdStart, "/set");
  if (!objectIdEnd) {
    return false;
  }

  const size_t objectIdLen = objectIdEnd - objectIdStart;
  if (objectIdLen == 0 || objectIdLen >= objectIdSize) {
    return false;
  }

  strncpy(objectId, objectIdStart, objectIdLen);
  objectId[objectIdLen] = '\0';
  return true;
}

/**
 * MQTT message callback for Home Assistant switch commands
 */
static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if (!HomeAssistant::useHomeAssistant)
    return;

  char payloadStr[32];
  size_t payloadLen = (len < sizeof(payloadStr) - 1) ? len : sizeof(payloadStr) - 1;
  memcpy(payloadStr, payload, payloadLen);
  payloadStr[payloadLen] = '\0';

  char objectId[32];
  if (extractHomeAssistantObjectId(topic, "switch", objectId, sizeof(objectId))) {
    bool state = (strcmp(payloadStr, "ON") == 0);

    if (strcmp(objectId, "pool-pump") == 0) {
      poolPumpNode.setSwitch(state);
      return;
    }
    if (strcmp(objectId, "solar-pump") == 0) {
      solarPumpNode.setSwitch(state);
      return;
    }
    if (strcmp(objectId, "log-serial") == 0) {
      LN.handleHomeAssistantCommand("LogSerial", state ? "true" : "false");
      return;
    }
  }

  if (extractHomeAssistantObjectId(topic, "select", objectId, sizeof(objectId))) {
    if (strcmp(objectId, "mode") == 0) {
      operationModeNode.handleHomeAssistantCommand("mode", payloadStr);
      return;
    }
    if (strcmp(objectId, "log-level") == 0) {
      LN.handleHomeAssistantCommand("Level", payloadStr);
      return;
    }
  }

  if (extractHomeAssistantObjectId(topic, "number", objectId, sizeof(objectId))) {
    if (strcmp(objectId, "pool-max-temp") == 0) {
      operationModeNode.handleHomeAssistantCommand("pool-max-temp", payloadStr);
      return;
    }
    if (strcmp(objectId, "solar-min-temp") == 0) {
      operationModeNode.handleHomeAssistantCommand("solar-min-temp", payloadStr);
      return;
    }
    if (strcmp(objectId, "hysteresis") == 0) {
      operationModeNode.handleHomeAssistantCommand("hysteresis", payloadStr);
      return;
    }
    if (strcmp(objectId, "timer-start-h") == 0) {
      operationModeNode.handleHomeAssistantCommand("timer-start-h", payloadStr);
      return;
    }
    if (strcmp(objectId, "timer-start-min") == 0) {
      operationModeNode.handleHomeAssistantCommand("timer-start-min", payloadStr);
      return;
    }
    if (strcmp(objectId, "timer-end-h") == 0) {
      operationModeNode.handleHomeAssistantCommand("timer-end-h", payloadStr);
      return;
    }
    if (strcmp(objectId, "timer-end-min") == 0) {
      operationModeNode.handleHomeAssistantCommand("timer-end-min", payloadStr);
      return;
    }
    if (strcmp(objectId, "timezone") == 0) {
      operationModeNode.handleHomeAssistantCommand("timezone", payloadStr);
      return;
    }
  }
}

static PoolControllerContext* Self;
auto                          Detail::setupProxy() -> void {
  Self->setupHandler();
}

PoolControllerContext::PoolControllerContext() {
  assert(!Self);
  Self = this;
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

  poolPumpNode.setMeasurementInterval(_loopInterval);
  solarPumpNode.setMeasurementInterval(_loopInterval);

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

  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

  // add the rules
  RuleAuto* autoRule = new RuleAuto(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(autoRule);

  RuleManu* manuRule = new RuleManu();
  operationModeNode.addRule(manuRule);

  RuleBoost* boostRule = new RuleBoost(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(boostRule);

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
    // Temperature sensors
    PoolController::MqttInterface::publishSensorDiscovery(
      "solar-temp", "Solar Temperature", "temperature", "°C", "mdi:solar-power");

    PoolController::MqttInterface::publishSensorDiscovery(
      "pool-temp", "Pool Temperature", "temperature", "°C", "mdi:pool");

#ifdef ESP32
    PoolController::MqttInterface::publishSensorDiscovery(
      "controller-temp", "Controller Temperature", "temperature", "°C", "mdi:thermometer");
#endif

    // Switches (relays) - publish discovery and subscribe to command topics
    PoolController::MqttInterface::publishSwitchDiscovery(
      "pool-pump", "Pool Pump", "mdi:pump");
    PoolController::MqttInterface::subscribeSwitch("pool-pump");

    PoolController::MqttInterface::publishSwitchDiscovery(
      "solar-pump", "Solar Pump", "mdi:solar-panel");
    PoolController::MqttInterface::subscribeSwitch("solar-pump");

    const char* modeOptions[] = {"manu", "auto", "boost", "timer"};
    PoolController::MqttInterface::publishSelectDiscovery(
      "mode", "Operation Mode", modeOptions, 4, "mdi:toggle-switch");
    PoolController::MqttInterface::subscribeSelect("mode");

    PoolController::MqttInterface::publishNumberDiscovery(
      "pool-max-temp", "Max. Pool Temperature", 0.0, 40.0, 0.1, "°C", "mdi:coolant-temperature", "box");
    PoolController::MqttInterface::subscribeNumber("pool-max-temp");

    PoolController::MqttInterface::publishNumberDiscovery(
      "solar-min-temp", "Min. Solar Temperature", 0.0, 100.0, 0.1, "°C", "mdi:thermometer", "box");
    PoolController::MqttInterface::subscribeNumber("solar-min-temp");

    PoolController::MqttInterface::publishNumberDiscovery(
      "hysteresis", "Hysterese", 0.0, 10.0, 0.1, "K", "mdi:delta", "box");
    PoolController::MqttInterface::subscribeNumber("hysteresis");

    PoolController::MqttInterface::publishNumberDiscovery(
      "timer-start-h", "Timer Start", 0.0, 23.0, 1.0, "h", "mdi:clock-start", "box");
    PoolController::MqttInterface::subscribeNumber("timer-start-h");

    PoolController::MqttInterface::publishNumberDiscovery(
      "timer-start-min", "Timer Start", 0.0, 59.0, 1.0, "min", "mdi:clock-start", "box");
    PoolController::MqttInterface::subscribeNumber("timer-start-min");

    PoolController::MqttInterface::publishNumberDiscovery(
      "timer-end-h", "Timer End", 0.0, 23.0, 1.0, "h", "mdi:clock-end", "box");
    PoolController::MqttInterface::subscribeNumber("timer-end-h");

    PoolController::MqttInterface::publishNumberDiscovery(
      "timer-end-min", "Timer End", 0.0, 59.0, 1.0, "min", "mdi:clock-end", "box");
    PoolController::MqttInterface::subscribeNumber("timer-end-min");

    PoolController::MqttInterface::publishNumberDiscovery(
      "timezone", "Timezone", 0.0, 9.0, 1.0, nullptr, "mdi:map-clock", "box");
    PoolController::MqttInterface::subscribeNumber("timezone");

    PoolController::MqttInterface::publishSensorDiscovery(
      "timezone-info", "Timezone Info", nullptr, nullptr, "mdi:map-clock");

    PoolController::MqttInterface::publishSensorDiscovery(
      "log", "Log Output", nullptr, nullptr, "mdi:message-text");

    const char* logLevelOptions[] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    PoolController::MqttInterface::publishSelectDiscovery(
      "log-level", "Loglevel", logLevelOptions, 5, "mdi:format-list-bulleted");
    PoolController::MqttInterface::subscribeSelect("log-level");

    PoolController::MqttInterface::publishSwitchDiscovery(
      "log-serial", "Log to serial interface", "mdi:serial-port");
    PoolController::MqttInterface::subscribeSwitch("log-serial");

    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Home Assistant discovery messages published");
  } else {
    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Using Homie MQTT Convention");
  }

  LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "State persistence and system monitoring initialized");
}

auto PoolControllerContext::setup() -> void {
  Homie.setLoggingPrinter(&Serial);

  Homie_setFirmware("pool-controller", "3.1.0");
  Homie_setBrand("smart-swimmingpool");

  // default interval of sending Temperature values
  this->loopIntervalSetting_.setDefaultValue(TEMP_READ_INTERVAL).setValidator([](const int32_t candidate) -> bool {
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

  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  Homie.setup();

  // Initialize controller regardless of WiFi/MQTT connection status
  // This ensures offline operation works from startup
  initializeController();

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
