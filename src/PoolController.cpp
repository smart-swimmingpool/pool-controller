// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file PoolController.cpp
 * @brief PoolControllerContext implementation — boot-loop detection, subsystem
 *        initialization, and the main control loop.
 */

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
#include "LogCapture.hpp"

#include "StatusLed.hpp"

#ifdef NORVI_AE01_R
#include "NorviOledDisplay.hpp"
#include "NorviButtonHandler.hpp"
#endif

#include "Config.hpp"

namespace PoolController {

#ifdef NORVI_AE01_R
// Shared OneWire bus for NORVI AE01-R (both DS18B20 sensors on GPIO25)
OneWire sharedOneWire(PIN_DS_SOLAR);
DallasTemperature sharedDallasSensor(&sharedOneWire);
DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", &sharedDallasSensor, 0, TEMP_READ_INTERVAL);
DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", &sharedDallasSensor, 1, TEMP_READ_INTERVAL);
#else
DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVAL);
DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVAL);
#endif
ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVAL);
#ifdef NORVI_AE01_R
// NORVI AE01-R uses active-HIGH relays (HIGH = relay ON, LOW = relay OFF)
RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL, false);
RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR, false);
#else
// Standard external relay modules use active-LOW (LOW = relay ON, HIGH = relay OFF)
RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);
#endif

OperationModeNode operationModeNode("operation-mode", "Operation Mode");

static uint32_t _measurementInterval = 10;
static uint32_t _lastMeasurement = 0;

static PoolControllerContext *Self = nullptr;

// ── Sensor address mapping helpers ──────────────────────────────────────────

/** @brief Check if an 8-byte address is all zeros. */
static bool isAddressZero(const uint8_t addr[8]) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] != 0) {
      return false;
    }
  }
  return true;
}

/** @brief Format an 8-byte address into a hex string buffer (17 chars). */
static void addressToString(const uint8_t addr[8], char *buf, size_t size) {
  snprintf(buf, size, "%02X%02X%02X%02X%02X%02X%02X%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
}

/**
 * @brief Load DS18B20 sensor address mapping from NVS.
 *
 * Reads the stored ROM addresses for solar and pool sensors from the
 * `ds18b20` Preferences namespace and applies them as address filters
 * on the corresponding DallasTemperatureNode instances.
 *
 * Should be called once before initializeController() so that begin()
 * can resolve the address filter during bus scan.
 */
static void loadSensorAddressMapping() {
  Preferences prefs;
  prefs.begin("ds18b20", true);  // read-only

  uint8_t solarAddr[8], poolAddr[8];
  size_t slen = prefs.getBytes("solar_adr", solarAddr, 8);
  size_t plen = prefs.getBytes("pool_adr", poolAddr, 8);

  prefs.end();

  if (slen == 8 && !isAddressZero(solarAddr)) {
    solarTemperatureNode.setAddressFilter(solarAddr);
    char buf[17];
    addressToString(solarAddr, buf, sizeof(buf));
    LOG_INFO("• Sensor mapping: Solar address loaded [%s]\n", buf);
  }

  if (plen == 8 && !isAddressZero(poolAddr)) {
    poolTemperatureNode.setAddressFilter(poolAddr);
    char buf[17];
    addressToString(poolAddr, buf, sizeof(buf));
    LOG_INFO("• Sensor mapping: Pool address loaded [%s]\n", buf);
  }

  if ((slen == 8 && !isAddressZero(solarAddr)) || (plen == 8 && !isAddressZero(poolAddr))) {
    LOG_INFO("• Sensor mapping: address filters applied (one or both sensors)\n");
  } else {
    LOG_WARN("• Sensor mapping: no addresses configured — using default device indices\n");
    LOG_INFO("  ℹ To configure: long-press Button 1 → Sensor Setup page → assign sensors\n");
  }
}

/**
 * @brief Construct the singleton context.
 * Stores the instance pointer for internal access. All subsystems are
 * initialized later in setup() and initializeController().
 */
PoolControllerContext::PoolControllerContext() {
  assert(!Self);
  Self = this;
}

/**
 * @brief Destroy the context and clear the instance pointer.
 */
PoolControllerContext::~PoolControllerContext() {
  assert(Self);
  Self = nullptr;
}

auto PoolControllerContext::initializeController() -> void {
  // Validate pin configuration — check for conflicts
  const uint8_t pins[] = {PIN_DS_SOLAR, PIN_DS_POOL, PIN_RELAY_POOL, PIN_RELAY_SOLAR};
  const char *pinNames[] = {"Solar Temp", "Pool Temp", "Pool Relay", "Solar Relay"};
  bool pinConflict = false;

  for (size_t i = 0; i < 4; i++) {
    for (size_t j = i + 1; j < 4; j++) {
      if (pins[i] == pins[j]) {
#ifdef NORVI_AE01_R
        // Shared OneWire bus is intentional on NORVI (both DS on GPIO25)
        if (pins[i] == PIN_DS_SOLAR && pins[j] == PIN_DS_POOL) {
          continue;
        }
#endif
        LOG_ERROR("✖ PIN CONFLICT: %s (pin %d) and %s (pin %d) use same pin!\n", pinNames[i], pins[i], pinNames[j], pins[j]);
        pinConflict = true;
      }
    }
  }

  if (pinConflict) {
    LOG_ERROR("✖ FATAL: Pin configuration conflicts detected!\n");
    LOG_WARN("  System will reboot in 5 seconds to try and recover...\n");
    Serial.flush();
    delay(5000);
    ESP.restart();  // F27 Fix! Clean restart instead of blocking WDT loop
  } else {
    LOG_INFO("✓ Pin configuration validated - no conflicts (optimierte Belegung)\n");
    LOG_INFO("  Solar Temp (DS18B20): GPIO %d\n", PIN_DS_SOLAR);
#ifdef NORVI_AE01_R
    LOG_INFO("  Pool Temp  (DS18B20): GPIO %d (shared bus via GPIO25)\n", PIN_DS_POOL);
#else
    LOG_INFO("  Pool Temp  (DS18B20): GPIO %d\n", PIN_DS_POOL);
#endif
    LOG_INFO("  Pool Pump  (Relay):   GPIO %d\n", PIN_RELAY_POOL);
    LOG_INFO("  Solar Pump (Relay):   GPIO %d\n", PIN_RELAY_SOLAR);
#ifdef LED_BUILTIN
    LOG_INFO("  Status LED:           GPIO %d (LED_BUILTIN)\n", PIN_LED_STATUS);
#else
    LOG_INFO("  Status LED:           GPIO %d\n", PIN_LED_STATUS);
#endif
  }

  // Set measurement intervals and propagate to all nodes
  _measurementInterval = ConfigManager::getSettings().loopInterval;
  solarTemperatureNode.setMeasurementInterval(_measurementInterval);
  poolTemperatureNode.setMeasurementInterval(_measurementInterval);
  ctrlTemperatureNode.setMeasurementInterval(_measurementInterval);
  poolPumpNode.setMeasurementInterval(_measurementInterval);
  solarPumpNode.setMeasurementInterval(_measurementInterval);
  operationModeNode.setMeasurementInterval(_measurementInterval);

  // Initialize NTP Client
  timeClientSetup(ConfigManager::getNtp().server.c_str());

  // Configure time degradation limits
  setTimeDegradationGreenHours(static_cast<uint8_t>(ConfigManager::getSettings().timeLossGreenHours));
  setTimeDegradationRedHours(static_cast<uint8_t>(ConfigManager::getSettings().timeLossRedHours));

  // Set Timezone index
  setTimezoneIndex(ConfigManager::getSettings().timezoneIndex);

#ifdef NORVI_AE01_R
  // Initialize the shared OneWire bus before individual node begin() calls.
  // Both DS18B20 sensors live on the same GPIO25 bus — the shared sensor
  // instance scans all devices and each node reads by device index.
  sharedDallasSensor.begin();
#endif

  // Initialize Temperature and Relay node drivers
  solarTemperatureNode.begin();
  poolTemperatureNode.begin();
  ctrlTemperatureNode.begin();
  poolPumpNode.begin();
  solarPumpNode.begin();
  operationModeNode.begin();

  // Load properties into Operation Mode
  operationModeNode.setMode(ConfigManager::getSettings().opMode.c_str(), "boot:config");
  operationModeNode.setPoolMaxTemperature(ConfigManager::getSettings().tempMaxPool);
  operationModeNode.setSolarMinTemperature(ConfigManager::getSettings().tempMinSolar);
  operationModeNode.setTemperatureHysteresis(ConfigManager::getSettings().tempHysteresis);

  // TimerSetting is loaded from NVS by OperationModeNode::begin() — no override needed

  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

  // Add the rules (OperationModeNode vector unique_ptr will own and delete them)
  operationModeNode.addRule(new RuleAuto(&solarPumpNode, &poolPumpNode));
  operationModeNode.addRule(new RuleManu());
  operationModeNode.addRule(new RuleBoost(&solarPumpNode, &poolPumpNode));
  operationModeNode.addRule(new RuleTimer(&solarPumpNode, &poolPumpNode));

  _lastMeasurement = 0;
}

/**
 * @brief Full initialization sequence called once at boot.
 *
 * Order:
 *   1. StateManager (NVS), SystemMonitor, DegradationManager
 *   2. Boot-loop detection — forces safe mode if detected
 *   3. ConfigManager (LittleFS config.json)
 *   4. NetworkManager (WiFi + MQTT)
 *   5. WebPortal (HTTP server + captive portal)
 *   6. MqttPublisher (HA Discovery)
 *   7. OtaUpdater (GitHub release check)
 *   8. initializeController() — pins, nodes, rules
 *   9. Load persisted operational state from NVS
 */
auto PoolControllerContext::setup() -> void {
  // Initialize Preferences (NVS), System Monitor and Degradation tracker
  StateManager::begin();
  SystemMonitor::begin();
  DegradationManager::begin();

  // Initialize Status-LED with Homie-compatible blink codes
  StatusLed::begin();

  // Load persistent sensor address mapping from NVS early.
  // Must run before NorviOledDisplay::begin() so that first-boot detection
  // (needsSensorMapping) correctly checks whether sensors are assigned.
  loadSensorAddressMapping();

  // Initialize persisted config (WiFi, MQTT, NTP, settings).
  // Must run before NorviOledDisplay::begin() for needsWiFiSetup() check.
  ConfigManager::begin();

#ifdef NORVI_AE01_R
  // Initialize NORVI-specific peripherals (OLED display + front buttons)
  NorviOledDisplay::begin();
  NorviButtonHandler::begin();

  // Wire button callbacks (S1=UP, S2=DOWN, S3=ACTION)
  // ── S1 (UP) ───────────────────────────────────────────────────────────
  NorviButtonHandler::onButton1Press([]() {
    if (NorviOledDisplay::isMenuActive()) {
      NorviOledDisplay::menuPrevious();
    } else if (NorviOledDisplay::isSelectSensorStep()) {
      NorviOledDisplay::setupSelectPrevious();
      NorviOledDisplay::requestRedraw();
    } else if (NorviOledDisplay::isSelectRoleStep()) {
      NorviOledDisplay::setupSelectSolar();
      NorviOledDisplay::requestRedraw();
    } else {
      NorviOledDisplay::previousPage();
    }
  });
  // ── S2 (DOWN) ─────────────────────────────────────────────────────────
  NorviButtonHandler::onButton2Press([]() {
    if (NorviOledDisplay::isMenuActive()) {
      NorviOledDisplay::menuNext();
    } else if (NorviOledDisplay::isSelectSensorStep()) {
      NorviOledDisplay::setupSelectNext();
      NorviOledDisplay::requestRedraw();
    } else if (NorviOledDisplay::isSelectRoleStep()) {
      NorviOledDisplay::setupSelectPool();
      NorviOledDisplay::requestRedraw();
    } else {
      NorviOledDisplay::nextPage();
    }
  });
  // ── S3 (CONFIRM) ──────────────────────────────────────────────────────
  NorviButtonHandler::onButton3Press([]() {
    if (NorviOledDisplay::isMenuActive()) {
      // Execute selected menu action, then return to MAIN
      NorviOledDisplay::Page prevPage = NorviOledDisplay::getCurrentPage();
      switch (NorviOledDisplay::getMenuSelection()) {
      case NorviOledDisplay::MenuItem::MODE: {
        // Cycle operation mode
        const String &currentMode = operationModeNode.getMode();
        if (currentMode == "auto") {
          operationModeNode.setMode("manu", "button:S3/menu");
        } else if (currentMode == "manu") {
          operationModeNode.setMode("boost", "button:S3/menu");
        } else if (currentMode == "boost") {
          operationModeNode.setMode("timer", "button:S3/menu");
        } else {
          operationModeNode.setMode("auto", "button:S3/menu");
        }
        LOG_INFO("→ Mode switched to: %s\n", operationModeNode.getMode().c_str());
        break;
      }
      case NorviOledDisplay::MenuItem::PUMP:
        // Toggle pool pump
        poolPumpNode.setSwitch(!poolPumpNode.getSwitch());
        LOG_INFO("→ Pump toggled: %s\n", poolPumpNode.getSwitch() ? "ON" : "OFF");
        break;
      case NorviOledDisplay::MenuItem::EXIT:
        // No action — just exit
        break;
      }
      NorviOledDisplay::exitMenu();
    } else if (NorviOledDisplay::getCurrentPage() == NorviOledDisplay::Page::MAIN) {
      // MAIN page: open action menu
      NorviOledDisplay::enterMenu();
    } else if (NorviOledDisplay::getCurrentPage() == NorviOledDisplay::Page::SENSOR_SETUP) {
      // Sensor setup: advance the wizard
      NorviOledDisplay::confirmAction();
    }
    // Other info pages: S3 intentionally does nothing
  });
  // ── S3 long-press: save sensor mapping & reboot ───────────────────────
  NorviButtonHandler::onButton3LongPress([]() -> bool {
    if (NorviOledDisplay::isMappingComplete()) {
      uint8_t solarAddr[8], poolAddr[8];
      NorviOledDisplay::getMapping(solarAddr, poolAddr);
      ConfigManager::saveSensorMapping(solarAddr, poolAddr);
      LOG_INFO("→ Sensor mapping saved — rebooting...\n");
      NetworkManager::restart();
      return true;
    }
    return false;
  });
#endif

  // --- Boot-loop detection ---
  bootLoopDetected_ = SystemMonitor::detectBootLoop();
  if (bootLoopDetected_) {
    LOG_ERROR("✖ SAFE MODE ACTIVE — all relays forced OFF\n");
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

  // Print address mapping status after node begin() resolved the filters
  if (solarTemperatureNode.hasAddressFilter() || poolTemperatureNode.hasAddressFilter()) {
    char buf[17];
    solarTemperatureNode.getDeviceAddressString(buf, sizeof(buf));
    LOG_INFO("  ◦ Solar node → device [%s] (status: %s)\n", buf, solarTemperatureNode.isSensorFound() ? "✓" : "✖");
    poolTemperatureNode.getDeviceAddressString(buf, sizeof(buf));
    LOG_INFO("  ◦ Pool node  → device [%s] (status: %s)\n", buf, poolTemperatureNode.isSensorFound() ? "✓" : "✖");
  }

  OperationModeNode::suppressPersist(false);

  // Load operational settings from NVS Preferences
  operationModeNode.loadState();

  // OTA safety: detect version transition and verify config integrity
  ConfigManager::logOtaTransition();

  LOG_INFO("✓ Controller setup completed. Free heap: %u B\n", ESP.getFreeHeap());
}

/**
 * @brief Main control loop — runs every iteration of Arduino loop().
 *
 * Order:
 *   1. Feed watchdog + check free memory (SystemMonitor)
 *   2. Evaluate degradation levels (DegradationManager)
 *   3. Clear boot-loop counter after 5 min stable uptime
 *   4. Run managers: NetworkManager, WebPortal, OtaUpdater
 *   5. Run nodes: sensors, relays, operation mode (triggers rule engine)
 *   6. Publish HA Discovery + states on MQTT (re)connect
 *   7. Periodically publish telemetry states to MQTT (every loopInterval s)
 */
auto PoolControllerContext::loop() -> void {
  // Feed watchdog and check memory thresholds
  SystemMonitor::feedWatchdog();
  SystemMonitor::checkMemory();

  // Evaluate degradation levels and health
  DegradationManager::evaluate();

  // Stable bootloop counter cleanup after 5 minutes
  static uint32_t lastBootClear = 0;
  static bool bootCounterCleared = false;
  if (!bootCounterCleared &&
    (millis() - lastBootClear) > static_cast<uint32_t>(SystemMonitor::BOOT_LOOP_CLEAR_AFTER_SEC) * 1000UL) {
    bootCounterCleared = true;
    SystemMonitor::clearBootLoopCounter();
    if (bootLoopDetected_) {
      LOG_INFO("→ Safe-mode: 5 min stable — boot-loop counter cleared\n");
      DegradationManager::unforceSafeMode();
      bootLoopDetected_ = false;
    }
    lastBootClear = millis();
  }

  // Run managers
  NetworkManager::loop();
  WebPortal::loop();
  OtaUpdater::loop();

  // --- Status-LED: Pattern an Systemzustand anpassen (Homie-Convention) ---
  if (OtaUpdater::isUpdateInProgress()) {
    StatusLed::setPattern(StatusLedPattern::OTA_UPDATE);
  } else if (bootLoopDetected_ || DegradationManager::isSafe()) {
    StatusLed::setPattern(StatusLedPattern::SAFE_MODE);
  } else if (NetworkManager::isApMode()) {
    StatusLed::setPattern(StatusLedPattern::AP_MODE);
  } else if (!NetworkManager::isWiFiConnected()) {
    StatusLed::setPattern(StatusLedPattern::CONNECTING);
  } else if (!NetworkManager::isMqttConnected()) {
    StatusLed::setPattern(StatusLedPattern::CONNECTED_NO_MQTT);
  } else {
    StatusLed::setPattern(StatusLedPattern::ONLINE);
  }
  StatusLed::loop();

#ifdef NORVI_AE01_R
  // Update NORVI OLED display and read front-panel buttons
  NorviOledDisplay::loop();
  NorviButtonHandler::loop();
#endif

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
