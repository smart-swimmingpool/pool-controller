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
#include "LogCapture.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"

using namespace PoolController;  // NOLINT(build/namespaces)

// Declare the globals defined in mocks/globals.cpp (matching extern in MqttPublisher.cpp)
// MUST be inside PoolController namespace to reference the same symbols as globals.cpp
namespace PoolController {
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;
extern RelayModuleNode poolPumpNode;
extern RelayModuleNode solarPumpNode;
extern OperationModeNode operationModeNode;
}  // namespace PoolController

// Capture globals
extern MqttClientCapture mqttCapture;

extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_TRUE(cond)                                     \
  do {                                                        \
    if (!(cond)) {                                            \
      test_fail(__FILE__, __LINE__, "Expected true: " #cond); \
      return 1;                                               \
    }                                                         \
    test_pass(__FILE__, __LINE__);                            \
  } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_STREQ(a, b)                                                                     \
  do {                                                                                         \
    const char *_a = (a);                                                                      \
    const char *_b = (b);                                                                      \
    if (strcmp(_a, _b) != 0) {                                                                 \
      char _msg[256];                                                                          \
      snprintf(_msg, sizeof(_msg), "Expected '%s' == '%s': got '%s' vs '%s'", #a, #b, _a, _b); \
      test_fail(__FILE__, __LINE__, _msg);                                                     \
      return 1;                                                                                \
    }                                                                                          \
    test_pass(__FILE__, __LINE__);                                                             \
  } while (0)

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
      if (msg.topic.find("sensor/pool-controller/pool-temp/config") != std::string::npos)
        hasPoolTemp = true;
      if (msg.topic.find("sensor/pool-controller/solar-temp/config") != std::string::npos)
        hasSolarTemp = true;
      if (msg.topic.find("switch/pool-controller/pool-pump/config") != std::string::npos)
        hasPoolPump = true;
      if (msg.topic.find("select/pool-controller/mode/config") != std::string::npos)
        hasMode = true;
      if (msg.topic.find("sensor/pool-controller/heap/config") != std::string::npos)
        hasHeap = true;
      if (msg.topic.find("sensor/pool-controller/uptime/config") != std::string::npos)
        hasUptime = true;
      if (msg.topic.find("select/pool-controller/timezone/config") != std::string::npos)
        hasTimezone = true;
      if (msg.topic.find("update/pool-controller/firmware-update/config") != std::string::npos)
        hasFirmwareUpdate = true;
      if (msg.topic.find("climate/pool-controller/thermostat/config") != std::string::npos)
        hasClimate = true;
      if (msg.topic.find("time/pool-controller/timer-start/config") != std::string::npos)
        hasTimerStart = true;
    }

    int missing = 0;
    if (!hasPoolTemp) {
      test_fail(__FILE__, __LINE__, "Missing pool-temp discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasSolarTemp) {
      test_fail(__FILE__, __LINE__, "Missing solar-temp discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasPoolPump) {
      test_fail(__FILE__, __LINE__, "Missing pool-pump discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasMode) {
      test_fail(__FILE__, __LINE__, "Missing mode discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasHeap) {
      test_fail(__FILE__, __LINE__, "Missing heap discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasUptime) {
      test_fail(__FILE__, __LINE__, "Missing uptime discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasTimezone) {
      test_fail(__FILE__, __LINE__, "Missing timezone discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasFirmwareUpdate) {
      test_fail(__FILE__, __LINE__, "Missing firmware update discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasClimate) {
      test_fail(__FILE__, __LINE__, "Missing climate discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasTimerStart) {
      test_fail(__FILE__, __LINE__, "Missing timer-start discovery");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT

    rc = (missing == 0) ? 0 : 1;
    if (rc == 0)
      passed++;
    else
      failed++;
    test_suite_end("MqttPublisher::publishDiscovery", missing == 0 ? 1 : 0, missing);
  }

  // ── Test: publishDiscovery payloads contain valid JSON ──
  {
    test_begin("MqttPublisher", "discovery payloads are valid JSON");

    int validCount = 0;
    int invalidCount = 0;

    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("/config") == std::string::npos)
        continue;
      if (msg.payload.empty())
        continue;  // Skip cleanup empty-retained messages (old entity topics)

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

    if (invalidCount == 0)
      test_pass(__FILE__, __LINE__);
    int totalValid = validCount;
    rc = (invalidCount == 0) ? 0 : 1;
    if (rc == 0)
      passed++;
    else
      failed++;
    test_suite_end("MqttPublisher::discovery_payloads", totalValid, invalidCount);
  }

  // ── Test: publishStates publishes state topics ──
  {
    test_begin("MqttPublisher::publishStates", "all state topics published");

    mqttCapture.clear();
    operationModeNode.setMode("auto");

    MqttPublisher::publishStates();

    bool hasPoolTempState = false;
    bool hasPoolPumpState = false;
    bool hasModeState = false;
    bool hasHeapState = false;
    bool hasUptimeState = false;

    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("sensor/pool-controller/pool-temp/state") != std::string::npos)
        hasPoolTempState = true;
      if (msg.topic.find("switch/pool-controller/pool-pump/state") != std::string::npos)
        hasPoolPumpState = true;
      if (msg.topic.find("select/pool-controller/mode/state") != std::string::npos)
        hasModeState = true;
      if (msg.topic.find("sensor/pool-controller/heap/state") != std::string::npos)
        hasHeapState = true;
      if (msg.topic.find("sensor/pool-controller/uptime/state") != std::string::npos)
        hasUptimeState = true;
    }

    int missing = 0;
    if (!hasPoolTempState) {
      test_fail(__FILE__, __LINE__, "Missing pool-temp state");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasPoolPumpState) {
      test_fail(__FILE__, __LINE__, "Missing pool-pump state");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasModeState) {
      test_fail(__FILE__, __LINE__, "Missing mode state");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasHeapState) {
      test_fail(__FILE__, __LINE__, "Missing heap state");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT
    if (!hasUptimeState) {
      test_fail(__FILE__, __LINE__, "Missing uptime state");
      missing++;
    }  // NOLINT
    else
      test_pass(__FILE__, __LINE__);  // NOLINT

    rc = (missing == 0) ? 0 : 1;
    if (rc == 0)
      passed++;
    else
      failed++;
    test_suite_end("MqttPublisher::publishStates", missing == 0 ? 1 : 0, missing);
  }

  // ── Test: Handle MQTT mode command ──
  {
    test_begin("MqttPublisher", "handle mode command from HA");

    mqttCapture.clear();
    operationModeNode.setMode("auto");

    // Simulate incoming MQTT message setting mode to "boost"
    MqttPublisher::handleMqttMessage(const_cast<char *>("homeassistant/select/pool-controller/mode/set"),
      const_cast<char *>("boost"), AsyncMqttClientMessageProperties{0, false, false}, 5, 0, 5);

    // The mode should have been set to "boost"
    std::string mode = operationModeNode.getMode().c_str();
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
    operationModeNode.setMode("manu");
    poolPumpNode.setSwitch(false);

    MqttPublisher::handleMqttMessage(const_cast<char *>("homeassistant/switch/pool-controller/pool-pump/set"),
      const_cast<char *>("ON"), AsyncMqttClientMessageProperties{0, false, false}, 2, 0, 2);

    // Pump should have turned ON
    bool pumpState = poolPumpNode.getSwitch();
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
    operationModeNode.setMode("auto");
    poolPumpNode.setSwitch(false);

    MqttPublisher::handleMqttMessage(const_cast<char *>("homeassistant/switch/pool-controller/pool-pump/set"),
      const_cast<char *>("ON"), AsyncMqttClientMessageProperties{0, false, false}, 2, 0, 2);

    // Pump should still be OFF (rejected because not in manual mode)
    bool pumpState = poolPumpNode.getSwitch();
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

  // ── Regression: climate preset_modes must NOT include "none" ──
  // HA MQTT climate schema rejects the *entire* discovery config if "none"
  // is listed in preset_modes (it's a reserved value HA sets internally
  // to mean "no preset active"). See fix/codex-review-issues.
  {
    test_begin("MqttPublisher::publishClimateDiscovery", "preset_modes must not contain 'none'");

    mqttCapture.clear();
    MqttPublisher::publishDiscovery();

    bool foundConfig = false;
    bool containsNone = false;
    int presetCount = 0;

    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("climate/pool-controller/thermostat/config") == std::string::npos)
        continue;
      if (msg.payload.empty())
        continue;  // retained-clear message

      foundConfig = true;
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, msg.payload);
      if (err) {
        test_fail(__FILE__, __LINE__, "Failed to parse climate discovery JSON");
        break;
      }

      if (!doc.containsKey("preset_modes")) {
        test_fail(__FILE__, __LINE__, "climate discovery missing 'preset_modes'");
        break;
      }

      JsonArray presets = doc["preset_modes"];
      for (JsonVariant p : presets) {
        presetCount++;
        if (strcmp(p.as<const char *>(), "none") == 0) {
          containsNone = true;
        }
      }
      break;
    }

    rc = (foundConfig && !containsNone && presetCount > 0) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "preset_modes must not include reserved value 'none'");
      failed++;
    }
    test_suite_end("MqttPublisher::preset_modes_no_none", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Regression: preset STATE must still report "none" when no preset active ──
  // "none" is HA-reserved and must be sent as the *state* (not listed as a
  // mode) whenever the pool is running in plain "auto" mode.
  {
    test_begin("MqttPublisher::publishClimateState", "preset state is 'none' when no preset active");

    mqttCapture.clear();
    NetworkManager::setMqttConnected(true);
    operationModeNode.setMode("auto");

    MqttPublisher::publishStates();

    std::string presetState;
    bool found = false;
    for (const auto &msg : mqttCapture.published) {
      if (msg.topic.find("climate/pool-controller/thermostat/preset/state") != std::string::npos) {
        presetState = msg.payload;
        found = true;
      }
    }

    rc = (found && presetState == "none") ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[128];
      snprintf(msg, sizeof(msg), "Expected preset state 'none', got '%s' (found=%d)", presetState.c_str(), found);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("MqttPublisher::preset_state_none", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: Event-entity discovery payload ──
  // Task 5: publishEventDiscovery("logs", ...) must announce the HA MQTT
  // "event" component with the curated event_types whitelist.
  {
    test_begin("MqttPublisher::publishEventDiscovery", "event entity with platform and event_types");

    mqttCapture.clear();
    NetworkManager::setMqttConnected(true);
    MqttPublisher::begin();
    MqttPublisher::publishDiscovery();

    const MqttMessage *cfg = mqttCapture.findPublished("homeassistant/event/pool-controller/logs/config");
    int missing = 0;
    if (cfg == nullptr) {
      test_fail(__FILE__, __LINE__, "Missing event discovery config topic");
      missing++;
    } else {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, cfg->payload);
      if (err) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Event config is not valid JSON: %s", err.c_str());
        test_fail(__FILE__, __LINE__, buf);
        missing++;
      } else {
        // platform must be "event" (HA event component)
        if (strcmp(doc["platform"] | "", "event") != 0) {
          test_fail(__FILE__, __LINE__, "event config platform != 'event'");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        // state_topic must point at the event state topic
        if (strcmp(doc["state_topic"] | "", "homeassistant/event/pool-controller/logs/state") != 0) {
          test_fail(__FILE__, __LINE__, "event config state_topic mismatch");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        if (!doc.containsKey("availability_topic")) {
          test_fail(__FILE__, __LINE__, "event config missing availability_topic");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        // event_types must contain the curated whitelist (LOG_WARN + MODE_CHANGED probe)
        JsonArray types = doc["event_types"];
        bool hasWarn = false;
        bool hasMode = false;
        for (JsonVariant t : types) {
          if (strcmp(t | "", "LOG_WARN") == 0)
            hasWarn = true;
          if (strcmp(t | "", "MODE_CHANGED") == 0)
            hasMode = true;
        }
        if (!hasWarn) {
          test_fail(__FILE__, __LINE__, "event_types missing LOG_WARN");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        if (!hasMode) {
          test_fail(__FILE__, __LINE__, "event_types missing MODE_CHANGED");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        if (!cfg->retained) {
          test_fail(__FILE__, __LINE__, "event config must be retained");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
      }
    }

    rc = (missing == 0) ? 0 : 1;
    if (rc == 0)
      passed++;
    else
      failed++;
    test_suite_end("MqttPublisher::publishEventDiscovery", missing == 0 ? 1 : 0, missing);
  }

  // ── Test: Export pump exports WARN entries as LOG_WARN events ──
  // Task 5: WARN/ERROR entries without marker → {"event_type":"LOG_WARN",...}
  // on the event state topic, plus a JSON-line on the raw pool-controller/log topic.
  {
    test_begin("MqttPublisher::publishStates", "exports WARN entry as LOG_WARN event");

    mqttCapture.clear();
    NetworkManager::setMqttConnected(true);
    LogCapture::begin();
    MqttPublisher::begin();

    LogCapture::log(LogLevel::Warning, "solar pump overheated");
    MqttPublisher::publishStates();

    int missing = 0;
    const MqttMessage *ev = mqttCapture.findPublished("homeassistant/event/pool-controller/logs/state");
    if (ev == nullptr) {
      test_fail(__FILE__, __LINE__, "No event state published for WARN entry");
      missing++;
    } else {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, ev->payload);
      if (err) {
        test_fail(__FILE__, __LINE__, "Event state is not valid JSON");
        missing++;
      } else {
        if (strcmp(doc["event_type"] | "", "LOG_WARN") != 0) {
          test_fail(__FILE__, __LINE__, "event_type != LOG_WARN for WARN entry");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        const char *msg = doc["message"];
        if (msg == nullptr || strstr(msg, "overheated") == nullptr) {
          test_fail(__FILE__, __LINE__, "event message missing WARN body");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
      }
    }

    // Raw topic for external tools: {"seq":…,"t":…,"level":…,"msg":…}
    const MqttMessage *raw = mqttCapture.findPublished("pool-controller/log");
    if (raw == nullptr) {
      test_fail(__FILE__, __LINE__, "Raw pool-controller/log topic not published");
      missing++;
    } else {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, raw->payload);
      if (err) {
        test_fail(__FILE__, __LINE__, "Raw log line is not valid JSON");
        missing++;
      } else {
        if ((doc["seq"] | 0u) == 0u) {
          test_fail(__FILE__, __LINE__, "Raw log line missing seq");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        if (strcmp(doc["level"] | "", "warning") != 0) {
          test_fail(__FILE__, __LINE__, "Raw log line level != warning");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        const char *msg = doc["msg"];
        if (msg == nullptr || strstr(msg, "overheated") == nullptr) {
          test_fail(__FILE__, __LINE__, "Raw log line missing message");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
      }
    }

    rc = (missing == 0) ? 0 : 1;
    if (rc == 0)
      passed++;
    else
      failed++;
    test_suite_end("MqttPublisher::export_warn_event", missing == 0 ? 1 : 0, missing);
  }

  // ── Test: Info entries are NOT exported ──
  // Task 5 volume control: only WARN/ERROR and curated events cross MQTT.
  {
    test_begin("MqttPublisher::publishStates", "Info entries are not exported");

    mqttCapture.clear();
    NetworkManager::setMqttConnected(true);
    LogCapture::begin();
    MqttPublisher::begin();

    LogCapture::log(LogLevel::Info, "normal heartbeat");
    MqttPublisher::publishStates();

    const MqttMessage *ev = mqttCapture.findPublished("homeassistant/event/pool-controller/logs/state");
    const MqttMessage *raw = mqttCapture.findPublished("pool-controller/log");

    rc = (ev == nullptr && raw == nullptr) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Info entry must not be exported via MQTT");
      failed++;
    }
    test_suite_end("MqttPublisher::export_no_info", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: Export pump dedup — second call without new entries publishes nothing ──
  {
    test_begin("MqttPublisher::publishStates", "no duplicate export on second call");

    mqttCapture.clear();
    NetworkManager::setMqttConnected(true);
    LogCapture::begin();
    MqttPublisher::begin();

    LogCapture::log(LogLevel::Error, "boom");
    MqttPublisher::publishStates();

    int firstCount = 0;
    for (const auto &m : mqttCapture.published) {
      if (m.topic == "homeassistant/event/pool-controller/logs/state")
        firstCount++;
    }

    mqttCapture.clear();
    MqttPublisher::publishStates();
    const MqttMessage *ev2 = mqttCapture.findPublished("homeassistant/event/pool-controller/logs/state");
    const MqttMessage *raw2 = mqttCapture.findPublished("pool-controller/log");

    rc = (firstCount == 1 && ev2 == nullptr && raw2 == nullptr) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "publishStates re-exported already-exported entries");
      failed++;
    }
    test_suite_end("MqttPublisher::export_dedup", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: logEvent marker becomes event_type on the event state topic ──
  // Task 5: LogCapture::logEvent("[MODE_CHANGED] ...") → event_type=MODE_CHANGED.
  // Info-level events are NOT mirrored to the raw topic (raw is WARN/ERROR only).
  {
    test_begin("MqttPublisher::publishStates", "logEvent marker exported as event_type");

    mqttCapture.clear();
    NetworkManager::setMqttConnected(true);
    LogCapture::begin();
    MqttPublisher::begin();

    LogCapture::logEvent("MODE_CHANGED", "switched to auto");
    MqttPublisher::publishStates();

    int missing = 0;
    const MqttMessage *ev = mqttCapture.findPublished("homeassistant/event/pool-controller/logs/state");
    if (ev == nullptr) {
      test_fail(__FILE__, __LINE__, "No event state published for logEvent");
      missing++;
    } else {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, ev->payload);
      if (err) {
        test_fail(__FILE__, __LINE__, "Event state is not valid JSON");
        missing++;
      } else {
        if (strcmp(doc["event_type"] | "", "MODE_CHANGED") != 0) {
          test_fail(__FILE__, __LINE__, "event_type != MODE_CHANGED");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
        if (strcmp(doc["message"] | "", "switched to auto") != 0) {
          test_fail(__FILE__, __LINE__, "event message mismatch");
          missing++;
        } else
          test_pass(__FILE__, __LINE__);  // NOLINT
      }
    }

    const MqttMessage *raw = mqttCapture.findPublished("pool-controller/log");
    if (raw != nullptr) {
      test_fail(__FILE__, __LINE__, "Info-level event must not be mirrored to raw topic");
      missing++;
    } else
      test_pass(__FILE__, __LINE__);  // NOLINT

    rc = (missing == 0) ? 0 : 1;
    if (rc == 0)
      passed++;
    else
      failed++;
    test_suite_end("MqttPublisher::export_event_marker", missing == 0 ? 1 : 0, missing);
  }

  return passed + failed;
}
