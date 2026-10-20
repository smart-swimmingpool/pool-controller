// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include <cstring>

#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "Nodes.hpp"
#include "WebPortal.hpp"
#include "WebServer.h"
#include "test_assert.h"

extern WebServerCapture wsCapture;
extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_STREQ(a, b) ASSERT_TRUE(std::strcmp((a), (b)) == 0)

namespace {
using PoolController::CommandStatus;
using PoolController::ControllerCommand;
using PoolController::ControllerCommandType;

constexpr std::size_t kMaxCaptured = 32;
ControllerCommand capturedCommands[kMaxCaptured]{};
std::size_t capturedCount = 0;
ControllerCommand &capturedCommand = capturedCommands[0];
PoolController::ControllerCommandResult configuredResult{};
bool dispatchCalled = false;
CommandStatus dispatchStatus = CommandStatus::APPLIED;

CommandStatus captureDispatch(
  const ControllerCommand *commands, std::size_t count, PoolController::ControllerCommandResult *result) {
  dispatchCalled = true;
  capturedCount = count;
  for (std::size_t i = 0; i < count && i < kMaxCaptured; ++i) {
    capturedCommands[i] = commands[i];
  }
  if (result != nullptr) {
    *result = configuredResult;
  }
  return dispatchStatus;
}

const ControllerCommand *findCaptured(ControllerCommandType type) {
  for (std::size_t i = 0; i < capturedCount; ++i) {
    if (capturedCommands[i].type == type) {
      return &capturedCommands[i];
    }
  }
  return nullptr;
}

void resetRequest() {
  wsCapture.clear();
  PoolController::ConfigManager::setConfigured(false);
  PoolController::ConfigManager::_recordInvalid = false;
  PoolController::NetworkManager::setApMode(true);
  PoolController::WebPortal::setCommandDispatcher(captureDispatch);
  dispatchCalled = false;
  dispatchStatus = CommandStatus::APPLIED;
  capturedCount = 0;
  capturedCommand = {};
  configuredResult = {};
}

int test_mode_validation() {
  test_begin("WebRuntimeCommands", "mode validation preserves HTTP compatibility");
  resetRequest();
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/mode", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 400);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"error\",\"message\":\"Missing mode\"}");
  ASSERT_TRUE(!dispatchCalled);
  resetRequest();
  wsCapture.args["mode"] = "invalid";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/mode", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 400);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"error\",\"message\":\"Invalid mode\"}");
  ASSERT_TRUE(!dispatchCalled);
  return 0;
}

int test_absolute_mode() {
  test_begin("WebRuntimeCommands", "absolute mode becomes a WEB typed command");
  resetRequest();
  wsCapture.args["mode"] = "boost";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/mode", HTTP_POST));
  ASSERT_TRUE(dispatchCalled);
  ASSERT_EQ(capturedCommand.type, PoolController::ControllerCommandType::SET_MODE);
  ASSERT_EQ(capturedCommand.source, PoolController::CommandSource::WEB);
  ASSERT_EQ(capturedCommand.mode, PoolController::OperationMode::BOOST);
  ASSERT_EQ(wsCapture.lastStatusCode, 200);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"ok\",\"mode\":\"boost\"}");
  return 0;
}

int test_invalid_config_record_keeps_setup_ap_behind_login() {
  test_begin("WebRuntimeCommands", "invalid config record keeps the setup AP behind the admin login");
  resetRequest();
  PoolController::ConfigManager::_recordInvalid = true;
  wsCapture.args["mode"] = "boost";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/mode", HTTP_POST));
  ASSERT_TRUE(!dispatchCalled);
  PoolController::ConfigManager::_recordInvalid = false;
  return 0;
}

int test_sensor_mapping_empty_role_clears_old_assignment() {
  test_begin("WebRuntimeCommands", "moving a probe to the other role clears the emptied role");
  resetRequest();
  wsCapture.args["solar_addr"] = "";
  wsCapture.args["pool_addr"] = "28AA000000000001";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/sensors/map", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 200);
  ASSERT_EQ(capturedCount, static_cast<std::size_t>(2));
  ASSERT_EQ(capturedCommands[0].type, ControllerCommandType::CLEAR_SENSOR_MAPPING);
  ASSERT_EQ(capturedCommands[0].sensorRole, PoolController::SensorRole::SOLAR);
  ASSERT_EQ(capturedCommands[1].type, ControllerCommandType::SET_SENSOR_MAPPING);
  ASSERT_EQ(capturedCommands[1].sensorRole, PoolController::SensorRole::POOL);

  // Only empty fields would change nothing useful and stay rejected.
  resetRequest();
  wsCapture.args["solar_addr"] = "";
  wsCapture.args["pool_addr"] = "";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/sensors/map", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 400);
  ASSERT_TRUE(!dispatchCalled);
  return 0;
}

int test_cycle_mode() {
  test_begin("WebRuntimeCommands", "cycle stays relative until the owner resolves it");
  resetRequest();
  wsCapture.args["mode"] = "cycle";
  configuredResult.hasMode = true;
  configuredResult.mode = PoolController::OperationMode::TIMER;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/mode", HTTP_POST));
  ASSERT_EQ(capturedCommand.type, PoolController::ControllerCommandType::CYCLE_MODE);
  ASSERT_EQ(capturedCommand.source, PoolController::CommandSource::WEB);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"ok\",\"mode\":\"timer\"}");
  return 0;
}

int test_pump_validation() {
  test_begin("WebRuntimeCommands", "pump validation preserves HTTP compatibility");
  resetRequest();
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/pump", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 400);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"error\",\"message\":\"Missing pump parameter\"}");
  ASSERT_TRUE(!dispatchCalled);
  resetRequest();
  wsCapture.args["pump"] = "invalid";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/pump", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 400);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"error\",\"message\":\"Invalid pump. Use 'pool' or 'solar'\"}");
  ASSERT_TRUE(!dispatchCalled);
  return 0;
}

int test_relative_pumps() {
  test_begin("WebRuntimeCommands", "pool and solar toggles stay relative and owner-resolved");
  resetRequest();
  wsCapture.args["pump"] = "pool";
  configuredResult.hasPumpState = true;
  configuredResult.pumpState = true;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/pump", HTTP_POST));
  ASSERT_EQ(capturedCommand.type, PoolController::ControllerCommandType::TOGGLE_POOL_PUMP);
  ASSERT_EQ(capturedCommand.source, PoolController::CommandSource::WEB);
  ASSERT_EQ(capturedCommand.toggleModePolicy, PoolController::PumpToggleModePolicy::REQUIRE_MANUAL);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"ok\",\"state\":true}");
  resetRequest();
  wsCapture.args["pump"] = "solar";
  configuredResult.hasPumpState = true;
  configuredResult.pumpState = false;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/pump", HTTP_POST));
  ASSERT_EQ(capturedCommand.type, PoolController::ControllerCommandType::TOGGLE_SOLAR_PUMP);
  ASSERT_EQ(capturedCommand.source, PoolController::CommandSource::WEB);
  ASSERT_EQ(capturedCommand.toggleModePolicy, PoolController::PumpToggleModePolicy::REQUIRE_MANUAL);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"ok\",\"state\":false}");
  return 0;
}

int test_owner_rejection() {
  test_begin("WebRuntimeCommands", "owner rejection maps to 409 with the manual-mode message");
  resetRequest();
  wsCapture.args["pump"] = "pool";
  dispatchStatus = CommandStatus::REJECTED;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/pump", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 409);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"error\",\"message\":\"Pump control only available in manual mode\"}");
  return 0;
}

int test_status_codes_are_distinct() {
  test_begin("WebRuntimeCommands", "invalid, rejected and persist failures map to 400, 409 and 500");
  const struct {
    CommandStatus status;
    int httpCode;
    const char *body;
  } cases[] = {
    {CommandStatus::INVALID, 400, "{\"status\":\"error\",\"message\":\"Invalid mode\"}"},
    {CommandStatus::REJECTED, 409, "{\"status\":\"error\",\"message\":\"Mode change rejected\"}"},
    {CommandStatus::PERSIST_FAILED, 500, "{\"status\":\"error\",\"message\":\"Failed to save mode\"}"},
  };
  for (const auto &c : cases) {
    resetRequest();
    wsCapture.args["mode"] = "boost";
    dispatchStatus = c.status;
    ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/mode", HTTP_POST));
    ASSERT_EQ(wsCapture.lastStatusCode, c.httpCode);
    ASSERT_STREQ(wsCapture.lastBody.c_str(), c.body);
  }
  return 0;
}

int test_settings_form_is_one_batch() {
  test_begin("WebRuntimeCommands", "settings form becomes one atomic WEB command batch");
  resetRequest();
  wsCapture.args["type"] = "settings";
  wsCapture.args["mode"] = "timer";
  wsCapture.args["interval"] = "15";
  wsCapture.args["max_pool"] = "27.5";
  wsCapture.args["green"] = "3";
  wsCapture.args["red"] = "30";
  wsCapture.args["btn_no_press"] = "4096";
  wsCapture.args["ntp_server"] = "time.example.org";
  wsCapture.args["timer_start_h"] = "7";
  wsCapture.args["circ_factor"] = "";  // empty means "keep", like a missing field
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/config", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 200);
  ASSERT_EQ(capturedCount, 8U);
  for (std::size_t i = 0; i < capturedCount; ++i) {
    ASSERT_EQ(capturedCommands[i].source, PoolController::CommandSource::WEB);
  }
  const ControllerCommand *mode = findCaptured(ControllerCommandType::SET_MODE);
  ASSERT_TRUE(mode != nullptr && mode->mode == PoolController::OperationMode::TIMER);
  const ControllerCommand *interval = findCaptured(ControllerCommandType::SET_LOOP_INTERVAL);
  ASSERT_TRUE(interval != nullptr && interval->integerValue == 15);
  const ControllerCommand *maxPool = findCaptured(ControllerCommandType::SET_POOL_MAX_TEMPERATURE);
  ASSERT_TRUE(maxPool != nullptr);
  ASSERT_NEAR(maxPool->value, 27.5F, 0.001F);
  const ControllerCommand *ntp = findCaptured(ControllerCommandType::SET_NTP_SERVER);
  ASSERT_TRUE(ntp != nullptr);
  ASSERT_STREQ(ntp->ntpServer.text.data(), "time.example.org");
  // Only the hour was sent: the minutes keep the running timer value.
  const ControllerCommand *timerStart = findCaptured(ControllerCommandType::SET_TIMER_START);
  ASSERT_TRUE(timerStart != nullptr);
  ASSERT_EQ(timerStart->hour, 7);
  ASSERT_EQ(timerStart->minute, PoolController::operationModeNode.getTimerSetting().timerStartMinutes);
  ASSERT_TRUE(findCaptured(ControllerCommandType::SET_TIMER_END) == nullptr);
  ASSERT_TRUE(findCaptured(ControllerCommandType::SET_TEMPERATURE_CIRCULATION_FACTOR) == nullptr);
  return 0;
}

int test_settings_form_rejects_malformed_numbers() {
  test_begin("WebRuntimeCommands", "a malformed settings field fails the whole form before dispatch");
  const char *badValues[][2] = {{"max_pool", "abc"}, {"interval", "15s"}, {"timer_end_m", "300"}, {"mode", "turbo"}};
  for (const auto &bad : badValues) {
    resetRequest();
    wsCapture.args["type"] = "settings";
    wsCapture.args["hysteresis"] = "1.5";
    wsCapture.args[bad[0]] = bad[1];
    ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/config", HTTP_POST));
    ASSERT_EQ(wsCapture.lastStatusCode, 400);
    ASSERT_FALSE(dispatchCalled);
  }
  resetRequest();
  wsCapture.args["type"] = "settings";
  wsCapture.args["hysteresis"] = "1.5";
  dispatchStatus = CommandStatus::PERSIST_FAILED;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/config", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 500);
  return 0;
}

int test_factory_reset_restarts_only_when_applied() {
  test_begin("WebRuntimeCommands", "factory reset is a command and reboots only after it was applied");
  resetRequest();
  const int restartsBefore = ESP.s_restartCount;
  dispatchStatus = CommandStatus::PERSIST_FAILED;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/factory_reset", HTTP_GET));
  ASSERT_EQ(capturedCount, 1U);
  ASSERT_EQ(capturedCommand.type, ControllerCommandType::FACTORY_RESET);
  ASSERT_EQ(wsCapture.lastStatusCode, 500);
  ASSERT_EQ(ESP.s_restartCount, restartsBefore);

  resetRequest();
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/factory_reset", HTTP_GET));
  ASSERT_EQ(wsCapture.lastStatusCode, 200);
  ASSERT_EQ(ESP.s_restartCount, restartsBefore + 1);
  return 0;
}

int test_sensor_mapping_is_a_command_batch() {
  test_begin("WebRuntimeCommands", "sensor mapping form becomes typed mapping commands");
  resetRequest();
  wsCapture.args["solar_addr"] = "28aabbccddeeff11";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/sensors/map", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 200);
  ASSERT_EQ(capturedCount, 1U);
  ASSERT_EQ(capturedCommand.type, ControllerCommandType::SET_SENSOR_MAPPING);
  ASSERT_EQ(capturedCommand.sensorRole, PoolController::SensorRole::SOLAR);
  ASSERT_EQ(capturedCommand.sensorAddress[0], 0x28);
  ASSERT_EQ(capturedCommand.sensorAddress[7], 0x11);

  resetRequest();
  wsCapture.args["solar_addr"] = "28AABBCCDDEEFF11";
  wsCapture.args["pool_addr"] = "28AABBCCDDEEFFZZ";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/sensors/map", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 400);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"error\",\"message\":\"Invalid pool address format\"}");
  ASSERT_FALSE(dispatchCalled);
  return 0;
}

int test_adapter_does_not_mutate_nodes() {
  test_begin("WebRuntimeCommands", "Web adapter itself does not mutate mode or relay nodes");
  PoolController::operationModeNode.setMode("auto");
  PoolController::poolPumpNode.setSwitch(false);
  PoolController::solarPumpNode.setSwitch(true);
  resetRequest();
  wsCapture.args["mode"] = "boost";
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/mode", HTTP_POST));
  ASSERT_STREQ(PoolController::operationModeNode.getMode().c_str(), "auto");
  resetRequest();
  wsCapture.args["pump"] = "pool";
  configuredResult.hasPumpState = true;
  configuredResult.pumpState = true;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/pump", HTTP_POST));
  ASSERT_TRUE(!PoolController::poolPumpNode.getSwitch());
  ASSERT_TRUE(PoolController::solarPumpNode.getSwitch());
  return 0;
}
}  // namespace

int run_web_runtime_command_tests() {
  PoolController::WebPortal::begin();
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_mode_validation, test_absolute_mode, test_invalid_config_record_keeps_setup_ap_behind_login,
    test_sensor_mapping_empty_role_clears_old_assignment, test_cycle_mode, test_pump_validation, test_relative_pumps,
    test_owner_rejection, test_status_codes_are_distinct, test_settings_form_is_one_batch,
    test_settings_form_rejects_malformed_numbers, test_factory_reset_restarts_only_when_applied,
    test_sensor_mapping_is_a_command_batch, test_adapter_does_not_mutate_nodes};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("WebRuntimeCommands", passed, failed);
  return failed;
}
