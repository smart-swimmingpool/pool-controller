// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "PoolController.hpp"

#include <Arduino.h>
#include <Homie.h>
#include <SPI.h>
#include <Preferences.h>
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
#include "DegradationManager.hpp"
#include "HomeAssistantMQTT.hpp"
#include "MqttInterface.hpp"
#include "Utils.hpp"
#include "WpsProvisioner.hpp"

#include "Config.hpp"

namespace PoolController {
static LoggerNode LN;
static DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVAL);
static DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVAL);
static ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVAL);
static RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
static RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);

static OperationModeNode operationModeNode("operation-mode", "Operation Mode");

static uint32_t _measurementInterval = 10;
static uint32_t _lastMeasurement;

static bool extractHomeAssistantObjectId(const char *topic, const char *component, char *objectId, size_t objectIdSize) {
  char prefix[128];
  snprintf(prefix, sizeof(prefix), "homeassistant/%s/pool-controller/", component);
  const size_t prefixLen = strlen(prefix);
  if (strncmp(topic, prefix, prefixLen) != 0) {
    return false;
  }

  const char *objectIdStart = topic + prefixLen;
  const char *objectIdEnd = strstr(objectIdStart, "/set");
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
static void onMqttMessage(
  char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
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
    if (strcmp(objectId, "timezone") == 0) {
      operationModeNode.handleHomeAssistantCommand("timezone", payloadStr);
      return;
    }
  }

  if (extractHomeAssistantObjectId(topic, "text", objectId, sizeof(objectId))) {
    if (strcmp(objectId, "timer-start") == 0) {
      operationModeNode.handleHomeAssistantCommand("timer-start", payloadStr);
      return;
    }
    if (strcmp(objectId, "timer-end") == 0) {
      operationModeNode.handleHomeAssistantCommand("timer-end", payloadStr);
      return;
    }
    Homie.getLogger() << F("⚠ Unhandled HA text entity: ") << objectId << endl;
  }
}

static PoolControllerContext *Self;
auto Detail::setupProxy() -> void {
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
  // Validate pin configuration - check for conflicts
  const uint8_t pins[] = {PIN_DS_SOLAR, PIN_DS_POOL, PIN_RELAY_POOL, PIN_RELAY_SOLAR};
  const char *pinNames[] = {"Solar Temp", "Pool Temp", "Pool Relay", "Solar Relay"};
  bool pinConflict = false;

  for (size_t i = 0; i < 4; i++) {
    for (size_t j = i + 1; j < 4; j++) {
      if (pins[i] == pins[j]) {
        Serial.printf("✖ PIN CONFLICT: %s (pin %d) and %s (pin %d) use same "
                      "pin!\n",
          pinNames[i], pins[i], pinNames[j], pins[j]);
        pinConflict = true;
      }
    }
  }

  if (pinConflict) {
    Serial.println("✖ FATAL: Pin configuration conflicts detected!");
    Serial.println("  Check Config.hpp for correct pin assignments.");
    Serial.println("  System halted.");
    Serial.flush();
    while (true) {
      delay(1000);  // Halt system
    }
  } else {
    Serial.println("✓ Pin configuration validated - no conflicts");
    Serial.printf("  Solar Temp: GPIO %d\n", PIN_DS_SOLAR);
    Serial.printf("  Pool Temp:  GPIO %d\n", PIN_DS_POOL);
    Serial.printf("  Pool Relay: GPIO %d\n", PIN_RELAY_POOL);
    Serial.printf("  Solar Relay: GPIO %d\n", PIN_RELAY_SOLAR);
  }

  // set measurement intervals
  const std::uint32_t _loopInterval = this->loopIntervalSetting_.get();

  // Initialize NTP client with configured server
  timeClientSetup(this->ntpServerSetting_.get());

  // P9: Propagate configurable time-loss thresholds to TimeClientHelper.
  // The red-hours setter enforces red > green internally.
  setTimeDegradationGreenHours(static_cast<uint8_t>(this->timeLossGreenHoursSetting_.get()));
  setTimeDegradationRedHours(static_cast<uint8_t>(this->timeLossRedHoursSetting_.get()));
  LN.logf(__PRETTY_FUNCTION__, LoggerNode::INFO, "Degradation thresholds: GREEN=%d h, RED=%d h", getTimeDegradationGreenHours(),
    getTimeDegradationRedHours());

  // Set the timezone from configuration
  setTimezoneIndex(this->timezoneSetting_.get());

  solarTemperatureNode.setMeasurementInterval(_loopInterval);
  poolTemperatureNode.setMeasurementInterval(_loopInterval);

  poolPumpNode.setMeasurementInterval(_loopInterval);
  solarPumpNode.setMeasurementInterval(_loopInterval);

  ctrlTemperatureNode.setMeasurementInterval(_loopInterval);

  operationModeNode.setMode(this->operationModeSetting_.get());
  operationModeNode.setPoolMaxTemperature(this->temperatureMaxPoolSetting_.get());
  operationModeNode.setSolarMinTemperature(this->temperatureMinSolarSetting_.get());
  operationModeNode.setTemperatureHysteresis(this->temperatureHysteresisSetting_.get());
  TimerSetting ts = operationModeNode.getTimerSetting();  //TODO: Configurable
  ts.timerStartHour = 10;
  ts.timerStartMinutes = 30;
  ts.timerEndHour = 17;
  ts.timerEndMinutes = 30;
  operationModeNode.setTimerSetting(ts);

  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

  // add the rules
  RuleAuto *autoRule = new RuleAuto(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(autoRule);

  RuleManu *manuRule = new RuleManu();
  operationModeNode.addRule(manuRule);

  RuleBoost *boostRule = new RuleBoost(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(boostRule);

  RuleTimer *timerRule = new RuleTimer(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(timerRule);

  _lastMeasurement = 0;
}

/**
 * Publish all current states to MQTT.
 * Called at the end of setupHandler() to refresh all states on every
 * (re-)connect, ensuring Home Assistant / Homie never shows stale data
 * after a temporary MQTT outage.
 */
static void publishAllStates() {
  // Operation mode + settings
  String mode = operationModeNode.getMode();
  MqttInterface::publishSelectState(operationModeNode, "mode", "mode", mode.c_str());

  char buffer[20];

  Utils::floatToString(operationModeNode.getPoolMaxTemperature(), buffer, sizeof(buffer));
  MqttInterface::publishNumberState(operationModeNode, "pool-max-temp", "pool-max-temp", buffer);

  Utils::floatToString(operationModeNode.getSolarMinTemperature(), buffer, sizeof(buffer));
  MqttInterface::publishNumberState(operationModeNode, "solar-min-temp", "solar-min-temp", buffer);

  Utils::floatToString(operationModeNode.getTemperatureHysteresis(), buffer, sizeof(buffer));
  MqttInterface::publishNumberState(operationModeNode, "hysteresis", "hysteresis", buffer);

  TimerSetting ts = operationModeNode.getTimerSetting();
  if (MqttInterface::isHomeAssistant()) {
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u", ts.timerStartHour, ts.timerStartMinutes);
    MqttInterface::publishTextEntityState(operationModeNode, "timer-start", "timer-start", timeStr);
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u", ts.timerEndHour, ts.timerEndMinutes);
    MqttInterface::publishTextEntityState(operationModeNode, "timer-end", "timer-end", timeStr);
  } else {
    Utils::intToString(ts.timerStartHour, buffer, sizeof(buffer));
    MqttInterface::publishNumberState(operationModeNode, "timer-start-h", "timer-start-h", buffer);

    Utils::intToString(ts.timerStartMinutes, buffer, sizeof(buffer));
    MqttInterface::publishNumberState(operationModeNode, "timer-start-min", "timer-start-min", buffer);

    Utils::intToString(ts.timerEndHour, buffer, sizeof(buffer));
    MqttInterface::publishNumberState(operationModeNode, "timer-end-h", "timer-end-h", buffer);

    Utils::intToString(ts.timerEndMinutes, buffer, sizeof(buffer));
    MqttInterface::publishNumberState(operationModeNode, "timer-end-min", "timer-end-min", buffer);
  }

  int tzIndex = getTimezoneIndex();
  Utils::intToString(tzIndex, buffer, sizeof(buffer));
  MqttInterface::publishNumberState(operationModeNode, "timezone", "timezone", buffer);

  String tzInfo = getTimeInfoFor(tzIndex);
  MqttInterface::publishTextState(operationModeNode, "timezone-info", "timezone-info", tzInfo.c_str());

  // Relay states — homie property MUST match the RelayModuleNode advertise()
  // (cSwitch = "switch"), not the node-id.  In HomeAssistant mode the
  // objectId is the node-id so discovery topics stay consistent.
  MqttInterface::publishSwitchState(poolPumpNode, "switch", "pool-pump", poolPumpNode.getSwitch());
  MqttInterface::publishSwitchState(solarPumpNode, "switch", "solar-pump", solarPumpNode.getSwitch());

  // Temperature values
  Utils::floatToString(poolTemperatureNode.getTemperature(), buffer, sizeof(buffer));
  MqttInterface::publishSensorState(poolTemperatureNode, "temperature", "pool-temp", buffer);

  Utils::floatToString(solarTemperatureNode.getTemperature(), buffer, sizeof(buffer));
  MqttInterface::publishSensorState(solarTemperatureNode, "temperature", "solar-temp", buffer);

  LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "All states published to MQTT");
}

/**
 * Homie Setup handler.
 * Only called when wifi and mqtt are connected.
 * StateManager, SystemMonitor, and loadState are initialized in setup()
 * to ensure they run regardless of network connectivity.
 */
auto PoolControllerContext::setupHandler() -> void {
  // Configure MQTT protocol based on setting
  const char *protocol = this->mqttProtocolSetting_.get();
  HomeAssistant::useHomeAssistant = (std::strcmp(protocol, "homeassistant") == 0);

  if (HomeAssistant::useHomeAssistant) {
    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Using Home Assistant MQTT Discovery");

    // Register MQTT message callback for Home Assistant
    Homie.getMqttClient().onMessage(onMqttMessage);

    // Publish Home Assistant discovery messages for all sensors and switches
    // Temperature sensors
    PoolController::MqttInterface::publishSensorDiscovery(
      "solar-temp", "Solar Temperature", "temperature", "°C", "mdi:solar-power");

    PoolController::MqttInterface::publishSensorDiscovery("pool-temp", "Pool Temperature", "temperature", "°C", "mdi:pool");

    PoolController::MqttInterface::publishSensorDiscovery(
      "controller-temp", "Controller Temperature", "temperature", "°C", "mdi:thermometer");

    // Switches (relays) - publish discovery and subscribe to command topics
    PoolController::MqttInterface::publishSwitchDiscovery("pool-pump", "Pool Pump", "mdi:pump");
    PoolController::MqttInterface::subscribeSwitch("pool-pump");

    PoolController::MqttInterface::publishSwitchDiscovery("solar-pump", "Solar Pump", "mdi:solar-panel");
    PoolController::MqttInterface::subscribeSwitch("solar-pump");

    const char *modeOptions[] = {"manu", "auto", "boost", "timer"};
    PoolController::MqttInterface::publishSelectDiscovery("mode", "Operation Mode", modeOptions, 4, "mdi:toggle-switch");
    PoolController::MqttInterface::subscribeSelect("mode");

    PoolController::MqttInterface::publishNumberDiscovery(
      "pool-max-temp", "Max. Pool Temperature", 0.0, 40.0, 0.1, "°C", "mdi:coolant-temperature", "box");
    PoolController::MqttInterface::subscribeNumber("pool-max-temp");

    PoolController::MqttInterface::publishNumberDiscovery(
      "solar-min-temp", "Min. Solar Temperature", 0.0, 100.0, 0.1, "°C", "mdi:thermometer", "box");
    PoolController::MqttInterface::subscribeNumber("solar-min-temp");

    PoolController::MqttInterface::publishNumberDiscovery("hysteresis", "Hysterese", 0.0, 10.0, 0.1, "K", "mdi:delta", "box");
    PoolController::MqttInterface::subscribeNumber("hysteresis");

    PoolController::MqttInterface::publishTextEntityDiscovery(
      "timer-start", "Timer Start", "^([0-1][0-9]|2[0-3]):[0-5][0-9]$", "mdi:clock-start");
    PoolController::MqttInterface::subscribeTextEntity("timer-start");

    PoolController::MqttInterface::publishTextEntityDiscovery(
      "timer-end", "Timer End", "^([0-1][0-9]|2[0-3]):[0-5][0-9]$", "mdi:clock-end");
    PoolController::MqttInterface::subscribeTextEntity("timer-end");

    PoolController::MqttInterface::publishNumberDiscovery("timezone", "Timezone", 0.0, 9.0, 1.0, nullptr, "mdi:map-clock", "box");
    PoolController::MqttInterface::subscribeNumber("timezone");

    PoolController::MqttInterface::publishSensorDiscovery("timezone-info", "Timezone Info", nullptr, nullptr, "mdi:map-clock");

    PoolController::MqttInterface::publishSensorDiscovery("log", "Log Output", nullptr, nullptr, "mdi:message-text");

    const char *logLevelOptions[] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    PoolController::MqttInterface::publishSelectDiscovery(
      "log-level", "Loglevel", logLevelOptions, 5, "mdi:format-list-bulleted");
    PoolController::MqttInterface::subscribeSelect("log-level");

    PoolController::MqttInterface::publishSwitchDiscovery("log-serial", "Log to serial interface", "mdi:serial-port");
    PoolController::MqttInterface::subscribeSwitch("log-serial");

    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Home Assistant discovery messages published");
  } else {
    LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "Using Homie MQTT Convention");
  }

  // Refresh all states on (re-)connect so that Home Assistant / Homie never
  // shows stale data after a temporary MQTT outage (P4).
  publishAllStates();

  LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "State persistence and system monitoring initialized");
}

auto PoolControllerContext::setup() -> void {
  Homie.setLoggingPrinter(&Serial);

  // --- P8: Boot-loop detection (run before any potentially crash-prone init) ---
  bootLoopDetected_ = SystemMonitor::detectBootLoop();
  if (bootLoopDetected_) {
    Serial.println("✖ SAFE MODE ACTIVE — all relays forced OFF");
    DegradationManager::forceSafeMode();

    // Clear stored relay states so they default to OFF after reboot
    Preferences prefs;
    prefs.begin("pool-pump", false);
    prefs.clear();
    prefs.end();
    prefs.begin("solar-pump", false);
    prefs.clear();
    prefs.end();
  }

  Homie_setFirmware("pool-controller", "3.2.0");
  Homie_setBrand("smart-swimmingpool");

  WpsProvisioner::runIfRequested();

  // default interval of sending Temperature values
  this->loopIntervalSetting_.setDefaultValue(TEMP_READ_INTERVAL).setValidator([](const long candidate) -> bool {
    return candidate >= 0 && candidate <= 300;
  });

  this->timezoneSetting_.setDefaultValue(0).setValidator(
    [](const long candidate) -> bool { return candidate >= 0 && candidate < getTzCount(); });

  this->ntpServerSetting_.setDefaultValue("pool.ntp.org").setValidator([](const char *const candidate) -> bool {
    return candidate != nullptr && strlen(candidate) > 0;
  });

  this->temperatureMaxPoolSetting_.setDefaultValue(28.5).setValidator(
    [](const double candidate) -> bool { return candidate >= 0 && candidate <= 30; });

  this->temperatureMinSolarSetting_.setDefaultValue(55.0).setValidator(
    [](const double candidate) noexcept -> bool { return candidate >= 0 && candidate <= 100; });

  this->temperatureHysteresisSetting_.setDefaultValue(1.0).setValidator(
    [](const double candidate) -> bool { return candidate >= 0 && candidate <= 10; });

  this->operationModeSetting_.setDefaultValue("auto").setValidator([](const char *const candidate) -> bool {
    return std::strcmp(candidate, "auto") == 0 || std::strcmp(candidate, "manu") == 0 || std::strcmp(candidate, "boost") == 0;
  });

  this->mqttProtocolSetting_.setDefaultValue("homeassistant").setValidator([](const char *const candidate) -> bool {
    return std::strcmp(candidate, "homie") == 0 || std::strcmp(candidate, "homeassistant") == 0;
  });

  // P9: Configurable time-loss thresholds
  this->timeLossGreenHoursSetting_.setDefaultValue(1).setValidator(
    [](const long candidate) -> bool { return candidate >= 1 && candidate <= 6; });
  this->timeLossRedHoursSetting_.setDefaultValue(24).setValidator([this](const long candidate) -> bool {
    return candidate >= 1 && candidate <= 72 && candidate > this->timeLossGreenHoursSetting_.get();
  });

  Homie.setSetupFunction(&Detail::setupProxy);

  // Initialize state management, system monitor, and degradation tracking
  // regardless of WiFi/MQTT connectivity. These must run before Homie.setup()
  // so nodes can access persisted state even when no MQTT connection is ever
  // established (e.g. WiFi outage at boot).
  StateManager::begin();
  SystemMonitor::begin();
  DegradationManager::begin();

  // P4: Republish all states on every MQTT (re-)connect so Home Assistant
  // never shows stale data after a temporary outage.
  Homie.onEvent([](const HomieEvent &event) {
    if (event.type == HomieEventType::MQTT_READY) {
      publishAllStates();
    }
  });

  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  Homie.setup();

  // Suppress persistence during initialization so Homie compile-time defaults
  // set below do NOT overwrite the user values already in NVS.  loadState()
  // restores those user values into memory right after.
  OperationModeNode::suppressPersist(true);

  // Initialize controller regardless of WiFi/MQTT connection status
  // This ensures offline operation works from startup
  initializeController();

  // End the suppression window: user values are now in NVS and will be
  // overwritten only on explicit user changes.
  OperationModeNode::suppressPersist(false);

  // Load persisted state — runs after initializeController so the rule-engine
  // and MQTT infra are fully set up, and suppressPersist ensures the Homie
  // defaults set by initializeController did not clobber the NVS state we are
  // about to read.
  operationModeNode.loadState();

  // P1: Refresh MQTT state now that persisted values are in memory.
  // publishAllStates() also runs during the Homie.setup() callback (setupProxy)
  // and the MQTT_READY event, but both fire before loadState() completes, so
  // clients may have received compile-time defaults.  This call ensures MQTT
  // reflects the actual persisted state.
  publishAllStates();

  LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Free heap: %d", ESP.getFreeHeap());
  Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << endl;
}

auto PoolControllerContext::loop() -> void {
  // Feed watchdog and check memory
  SystemMonitor::feedWatchdog();
  SystemMonitor::checkMemory();

  // Evaluate system health and trigger degradation transitions
  DegradationManager::evaluate();

  // P8: Clear boot-loop counter after 5 minutes of stable uptime.
  // This tells detectBootLoop() on the next boot that this boot was healthy.
  // Even during safe mode we clear it: if the device runs 5+ minutes without
  // crashing, the next intentional restart should not be treated as a loop.
  // The guard ensures we only write NVS once per boot (avoids flash wear).
  static uint32_t lastBootClear = 0;
  static bool bootCounterCleared = false;
  if (!bootCounterCleared &&
    (millis() - lastBootClear) > static_cast<uint32_t>(SystemMonitor::BOOT_LOOP_CLEAR_AFTER_SEC) * 1000UL) {
    bootCounterCleared = true;
    SystemMonitor::clearBootLoopCounter();
    if (bootLoopDetected_) {
      Serial.println(F("→ Safe-mode: 5 min stable — boot-loop counter cleared"));
      DegradationManager::unforceSafeMode();
      bootLoopDetected_ = false;
    }
    lastBootClear = millis();
  }

  Homie.loop();
}
}  // namespace PoolController
