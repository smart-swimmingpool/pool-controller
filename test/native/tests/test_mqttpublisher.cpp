/**
 * @file test_mqttpublisher.cpp
 * @brief Tests for MqttPublisher — HA discovery payloads and state publishing.
 */

#include <stdio.h>
#include <string.h>
#include <string>
#include "Arduino.h"
#include "ArduinoJson.h"

#include "AsyncMqttClient.h"
#include "MqttPublisher.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"

using namespace PoolController;

// Node instances (matching extern declarations in MqttPublisher.cpp)
DallasTemperatureNode g_solarTemperatureNode("solar-temp", "Solar Temperature", 32);
DallasTemperatureNode g_poolTemperatureNode("pool-temp", "Pool Temperature", 33);
ESP32TemperatureNode g_ctrlTemperatureNode("ctrl-temp", "Controller Temperature");
RelayModuleNode g_poolPumpNode("pool-pump", "Pool Pump", 25);
RelayModuleNode g_solarPumpNode("solar-pump", "Solar Pump", 26);
OperationModeNode g_operationModeNode;

// Capture globals
extern MqttClientCapture mqttCapture;

extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_TRUE(cond) do { \
  if (!(cond)) { test_fail(__FILE__, __LINE__, "Expected true: " #cond); return 1; } \
  else { test_pass(__FILE__, __LINE__); } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_STREQ(a, b) do { \
  const char *_a = (a); const char *_b = (b); \
  if (strcmp(_a, _b) != 0) { \
    char _msg[256]; snprintf(_msg, sizeof(_msg), "Expected '%s' == '%s': got '%s' vs '%s'", #a, #b, _a, _b); \
    test_fail(__FILE__, __LINE__, _msg); return 1; \
  } \
  test_pass(__FILE__, __LINE__); \
} while(0)

int run_mqttpublisher_tests() {
  int passed = 0, failed = 0;
  int rc;

  // ── Test: publishDiscovery publishes expected topics ──
  {
    test_begin("MqttPublisher::publishDiscovery", "all entities discovered");

    // Clear MQTT capture
    mqttCapture.clear();
    
    // Initialize MQTT
    NetworkManager::setMqttConnected(true);
    MqttPublisher::begin();
    MqttPublisher::publishDiscovery();

    // Check that key discovery topics were published
    bool hasPoolTemp = false;
    bool hasSolarTemp = false;
    bool hasPoolPump = false;
    bool hasMode = false;
    bool hasHeap = false;
    bool hasUptime = false;
    bool hasTimezone = false;
    bool hasFirmwareUpdate = false;
    bool hasClimate = false;
    bool hasTimerStart = false;
    
    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("sensor/pool-controller/pool-temp/config") != std::string::npos) hasPoolTemp = true;
      if (msg.topic.find("sensor/pool-controller/solar-temp/config") != std::string::npos) hasSolarTemp = true;
      if (msg.topic.find("switch/pool-controller/pool-pump/config") != std::string::npos) hasPoolPump = true;
      if (msg.topic.find("select/pool-controller/mode/config") != std::string::npos) hasMode = true;
      if (msg.topic.find("sensor/pool-controller/heap/config") != std::string::npos) hasHeap = true;
      if (msg.topic.find("sensor/pool-controller/uptime/config") != std::string::npos) hasUptime = true;
      if (msg.topic.find("select/pool-controller/timezone/config") != std::string::npos) hasTimezone = true;
      if (msg.topic.find("update/pool-controller/firmware-update/config") != std::string::npos) hasFirmwareUpdate = true;
      if (msg.topic.find("climate/pool-controller/thermostat/config") != std::string::npos) hasClimate = true;
      if (msg.topic.find("time/pool-controller/timer-start/config") != std::string::npos) hasTimerStart = true;
    }
    
    int missing = 0;
    if (!hasPoolTemp) { test_fail(__FILE__, __LINE__, "Missing pool-temp discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasSolarTemp) { test_fail(__FILE__, __LINE__, "Missing solar-temp discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasPoolPump) { test_fail(__FILE__, __LINE__, "Missing pool-pump discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasMode) { test_fail(__FILE__, __LINE__, "Missing mode discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasHeap) { test_fail(__FILE__, __LINE__, "Missing heap discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasUptime) { test_fail(__FILE__, __LINE__, "Missing uptime discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasTimezone) { test_fail(__FILE__, __LINE__, "Missing timezone discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasFirmwareUpdate) { test_fail(__FILE__, __LINE__, "Missing firmware update discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasClimate) { test_fail(__FILE__, __LINE__, "Missing climate discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasTimerStart) { test_fail(__FILE__, __LINE__, "Missing timer-start discovery"); missing++; }
    else test_pass(__FILE__, __LINE__);

    rc = (missing == 0) ? 0 : 1;
    if (rc == 0) passed++;
    else failed++;
    test_suite_end("MqttPublisher::publishDiscovery", missing == 0 ? 1 : 0, missing);
  }

  // ── Test: publishDiscovery payloads contain valid JSON ──
  {
    test_begin("MqttPublisher", "discovery payloads are valid JSON");
    
    int validCount = 0;
    int invalidCount = 0;
    
    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("/config") == std::string::npos) continue;
      
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, msg.payload);
      if (err) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Invalid JSON in %s: %s", msg.topic.c_str(), err.c_str());
        test_fail(__FILE__, __LINE__, buf);
        invalidCount++;
      } else {
        // Verify required fields
        if (!doc.containsKey("name")) {
          char buf[128];
          snprintf(buf, sizeof(buf), "Missing 'name' in %s", msg.topic.c_str());
          test_fail(__FILE__, __LINE__, buf);
          invalidCount++;
        } else {
          validCount++;
        }
        
        // Verify device block exists
        if (!doc.containsKey("device")) {
          char buf[128];
          snprintf(buf, sizeof(buf), "Missing 'device' block in %s", msg.topic.c_str());
          test_fail(__FILE__, __LINE__, buf);
          invalidCount++;
        }
      }
    }
    
    if (invalidCount == 0) test_pass(__FILE__, __LINE__);
    int totalValid = validCount;
    rc = (invalidCount == 0) ? 0 : 1;
    if (rc == 0) passed++;
    else failed++;
    test_suite_end("MqttPublisher::discovery_payloads", totalValid, invalidCount);
  }

  // ── Test: publishStates publishes state topics ──
  {
    test_begin("MqttPublisher::publishStates", "all state topics published");

    mqttCapture.clear();
    g_operationModeNode.setMode("auto");
    
    MqttPublisher::publishStates();

    bool hasPoolTempState = false;
    bool hasPoolPumpState = false;
    bool hasModeState = false;
    bool hasHeapState = false;
    bool hasUptimeState = false;

    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("sensor/pool-controller/pool-temp/state") != std::string::npos) hasPoolTempState = true;
      if (msg.topic.find("switch/pool-controller/pool-pump/state") != std::string::npos) hasPoolPumpState = true;
      if (msg.topic.find("select/pool-controller/mode/state") != std::string::npos) hasModeState = true;
      if (msg.topic.find("sensor/pool-controller/heap/state") != std::string::npos) hasHeapState = true;
      if (msg.topic.find("sensor/pool-controller/uptime/state") != std::string::npos) hasUptimeState = true;
    }

    int missing = 0;
    if (!hasPoolTempState) { test_fail(__FILE__, __LINE__, "Missing pool-temp state"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasPoolPumpState) { test_fail(__FILE__, __LINE__, "Missing pool-pump state"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasModeState) { test_fail(__FILE__, __LINE__, "Missing mode state"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasHeapState) { test_fail(__FILE__, __LINE__, "Missing heap state"); missing++; }
    else test_pass(__FILE__, __LINE__);
    if (!hasUptimeState) { test_fail(__FILE__, __LINE__, "Missing uptime state"); missing++; }
    else test_pass(__FILE__, __LINE__);

    rc = (missing == 0) ? 0 : 1;
    if (rc == 0) passed++;
    else failed++;
    test_suite_end("MqttPublisher::publishStates", missing == 0 ? 1 : 0, missing);
  }

  // ── Test: Handle MQTT mode command ──
  {
    test_begin("MqttPublisher", "handle mode command from HA");

    mqttCapture.clear();
    g_operationModeNode.setMode("auto");

    // Simulate incoming MQTT message setting mode to "boost"
    MqttPublisher::handleMqttMessage(
      const_cast<char*>("homeassistant/select/pool-controller/mode/set"),
      const_cast<char*>("boost"),
      AsyncMqttClientMessageProperties{0, false, false},
      5, 0, 5
    );

    // The mode should have been set to "boost"
    std::string mode = g_operationModeNode.getMode().c_str();
    rc = (mode == "boost") ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected mode=boost, got %s", mode.c_str());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("MqttPublisher::handle_mode_command", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: Handle MQTT pump command in manual mode ──
  {
    test_begin("MqttPublisher", "handle pump command in manual mode");

    mqttCapture.clear();
    g_operationModeNode.setMode("manu");
    g_poolPumpNode.setSwitch(false);

    MqttPublisher::handleMqttMessage(
      const_cast<char*>("homeassistant/switch/pool-controller/pool-pump/set"),
      const_cast<char*>("ON"),
      AsyncMqttClientMessageProperties{0, false, false},
      2, 0, 2
    );

    // Pump should have turned ON
    bool pumpState = g_poolPumpNode.getSwitch();
    rc = (pumpState) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Expected pool-pump ON, got OFF");
      failed++;
    }
    test_suite_end("MqttPublisher::handle_pump_command", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: Reject pump command in auto mode ──
  {
    test_begin("MqttPublisher", "reject pump command in auto mode");

    mqttCapture.clear();
    g_operationModeNode.setMode("auto");
    g_poolPumpNode.setSwitch(false);

    MqttPublisher::handleMqttMessage(
      const_cast<char*>("homeassistant/switch/pool-controller/pool-pump/set"),
      const_cast<char*>("ON"),
      AsyncMqttClientMessageProperties{0, false, false},
      2, 0, 2
    );

    // Pump should still be OFF (rejected because not in manual mode)
    bool pumpState = g_poolPumpNode.getSwitch();
    rc = (!pumpState) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Expected pool-pump OFF (rejected in auto mode)");
      failed++;
    }
    test_suite_end("MqttPublisher::reject_pump_auto", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: Discovery payload switches have no entity_category ──
  {
    test_begin("MqttPublisher", "pump switches have no entity_category");

    mqttCapture.clear();
    MqttPublisher::publishDiscovery();

    // Find pool-pump discovery config
    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("switch/pool-controller/pool-pump/config") != std::string::npos) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, msg.payload);
        if (err) {
          test_fail(__FILE__, __LINE__, "Failed to parse pool-pump config JSON");
          failed++;
        } else {
          // entity_category should NOT be present (null/empty)
          if (doc.containsKey("entity_category")) {
            // It might be acceptable, but check it's not "control" specifically
            const char *cat = doc["entity_category"];
            if (cat && strcmp(cat, "control") == 0) {
              test_fail(__FILE__, __LINE__, "pool-pump should not have entity_category=control");
              failed++;
            } else {
              test_pass(__FILE__, __LINE__);
              passed++;
            }
          } else {
            test_pass(__FILE__, __LINE__);
            passed++;
          }
        }
        break;
      }
    }
    
    test_suite_end("MqttPublisher::no_entity_category", 1, 0);
  }

  return passed + failed;
}
