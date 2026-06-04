// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "PoolController.hpp"

#include <Arduino.h>
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

#include "TimeClientHelper.hpp"
#include "StateManager.hpp"
#include "SystemMonitor.hpp"
#include "DegradationManager.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "WebPortal.hpp"
#include "MqttPublisher.hpp"
#include "OtaUpdater.hpp"
#include "Utils.hpp"
#include "WpsProvisioner.hpp"

#include "Config.hpp"

namespace PoolController {

DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVAL);
DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVAL);
ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVAL);
RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);

OperationModeNode operationModeNode("operation-mode", "Operation Mode");

static uint32_t _measurementInterval = 10;
static uint32_t _lastMeasurement = 0;

static PoolControllerContext *Self = nullptr;

PoolControllerContext::PoolControllerContext() {
  assert(!Self);
  Self = this;
}

PoolControllerContext::~PoolControllerContext() {
  assert(Self);
  Self = nullptr;
}

auto PoolControllerContext::initializeController() -> void {
  // Validate pin configuration - check for conflicts
  const uint8_t pins[] = {PIN_DS_SOLAR, PIN_DS_POOL, PIN_RELAY_POOL, PIN_RELAY_SOLAR};
  const char *pinNames[] = {"Solar Temp", "Pool Temp", "Pool Relay", "Solar Relay"};
  bool pinConflict = false;

  for (size_t i = 0; i < 4; i++) {
    for (size_t j = i + 1; j < 4; j++) {
      if (pins[i] == pins[j]) {
        Serial.printf("✖ PIN CONFLICT: %s (pin %d) and %s (pin %d) use same pin!\n",
          pinNames[i], pins[i], pinNames[j], pins[j]);
        pinConflict = true;
      }
    }
  }

  if (pinConflict) {
    Serial.println("✖ FATAL: Pin configuration conflicts detected!");
    Serial.println("  System will reboot in 5 seconds to try and recover...");
    Serial.flush();
    delay(5000);
    ESP.restart(); // F27 Fix! Clean restart instead of blocking WDT loop
  } else {
    Serial.println("✓ Pin configuration validated - no conflicts");
    Serial.printf("  Solar Temp: GPIO %d\n", PIN_DS_SOLAR);
    Serial.printf("  Pool Temp:  GPIO %d\n", PIN_DS_POOL);
    Serial.printf("  Pool Relay: GPIO %d\n", PIN_RELAY_POOL);
    Serial.printf("  Solar Relay: GPIO %d\n", PIN_RELAY_SOLAR);
  }

  // Set measurement intervals
  _measurementInterval = ConfigManager::getSettings().loopInterval;
  
  // Initialize NTP Client
  timeClientSetup(ConfigManager::getNtp().server.c_str());

  // Configure time degradation limits
  setTimeDegradationGreenHours(static_cast<uint8_t>(ConfigManager::getSettings().timeLossGreenHours));
  setTimeDegradationRedHours(static_cast<uint8_t>(ConfigManager::getSettings().timeLossRedHours));
  
  // Set Timezone index
  setTimezoneIndex(ConfigManager::getSettings().timezoneIndex);

  // Initialize Temperature and Relay node drivers
  solarTemperatureNode.begin();
  poolTemperatureNode.begin();
  ctrlTemperatureNode.begin();
  poolPumpNode.begin();
  solarPumpNode.begin();
  operationModeNode.begin();

  // Load properties into Operation Mode
  operationModeNode.setMode(ConfigManager::getSettings().opMode.c_str());
  operationModeNode.setPoolMaxTemperature(ConfigManager::getSettings().tempMaxPool);
  operationModeNode.setSolarMinTemperature(ConfigManager::getSettings().tempMinSolar);
  operationModeNode.setTemperatureHysteresis(ConfigManager::getSettings().tempHysteresis);
  
  TimerSetting ts = operationModeNode.getTimerSetting();
  ts.timerStartHour = 10;
  ts.timerStartMinutes = 30;
  ts.timerEndHour = 17;
  ts.timerEndMinutes = 30;
  operationModeNode.setTimerSetting(ts);

  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

  // Add the rules (OperationModeNode vector unique_ptr will own and delete them)
  operationModeNode.addRule(new RuleAuto(&solarPumpNode, &poolPumpNode));
  operationModeNode.addRule(new RuleManu());
  operationModeNode.addRule(new RuleBoost(&solarPumpNode, &poolPumpNode));
  operationModeNode.addRule(new RuleTimer(&solarPumpNode, &poolPumpNode));

  _lastMeasurement = 0;
}

auto PoolControllerContext::setup() -> void {
  // Initialize Preferences (NVS), System Monitor and Degradation tracker
  StateManager::begin();
  SystemMonitor::begin();
  DegradationManager::begin();

  // --- Boot-loop detection ---
  bootLoopDetected_ = SystemMonitor::detectBootLoop();
  if (bootLoopDetected_) {
    Serial.println("✖ SAFE MODE ACTIVE — all relays forced OFF");
    DegradationManager::forceSafeMode();

    // Clear stored relay states
    Preferences prefs;
    prefs.begin("pool-pump", false);
    prefs.clear();
    prefs.end();
    prefs.begin("solar-pump", false);
    prefs.clear();
    prefs.end();
  }

  // Initialize Configuration
  ConfigManager::begin();

  // Start WiFi/WPS and MQTT services
  NetworkManager::begin();

  // Start Captive Setup Web Portal
  WebPortal::begin();

  // Start Home Assistant Discovery stack
  MqttPublisher::begin();

  // Start OTA update checker
  OtaUpdater::begin();

  // Suppress NVS persistence during setup initialization
  OperationModeNode::suppressPersist(true);

  // Initialize core drivers and parameters
  initializeController();

  OperationModeNode::suppressPersist(false);

  // Load operational settings from NVS Preferences
  operationModeNode.loadState();

  // OTA safety: detect version transition and verify config integrity
  ConfigManager::logOtaTransition();
  
  Serial.printf("✓ Controller setup completed. Free heap: %u B\n", ESP.getFreeHeap());
}

auto PoolControllerContext::loop() -> void {
  // Feed watchdog and check memory thresholds
  SystemMonitor::feedWatchdog();
  SystemMonitor::checkMemory();

  // Evaluate degradation levels and health
  DegradationManager::evaluate();

  // Stable bootloop counter cleanup after 5 minutes
  static uint32_t lastBootClear = 0;
  static bool bootCounterCleared = false;
  if (!bootCounterCleared && (millis() - lastBootClear) >
      static_cast<uint32_t>(SystemMonitor::BOOT_LOOP_CLEAR_AFTER_SEC) * 1000UL) {
    bootCounterCleared = true;
    SystemMonitor::clearBootLoopCounter();
    if (bootLoopDetected_) {
      Serial.println("→ Safe-mode: 5 min stable — boot-loop counter cleared");
      DegradationManager::unforceSafeMode();
      bootLoopDetected_ = false;
    }
    lastBootClear = millis();
  }

  // Run managers
  NetworkManager::loop();
  WebPortal::loop();
  OtaUpdater::loop();

  // Run drivers & logic rules
  solarTemperatureNode.loop();
  poolTemperatureNode.loop();
  ctrlTemperatureNode.loop();
  poolPumpNode.loop();
  solarPumpNode.loop();
  operationModeNode.loop();

  // Handle Home Assistant Connection State transition
  static bool wasMqttConnected = false;
  bool currentMqttState = NetworkManager::isMqttConnected();
  if (currentMqttState && !wasMqttConnected) {
    // Freshly connected to MQTT: publish Discovery and States
    MqttPublisher::publishDiscovery();
    MqttPublisher::publishStates();
    wasMqttConnected = true;
  } else if (!currentMqttState) {
    wasMqttConnected = false;
  }

  // Periodically publish telemetry states to HA (P4)
  if (currentMqttState && Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();
    MqttPublisher::publishStates();
  }
}

}  // namespace PoolController
