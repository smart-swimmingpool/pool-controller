// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include <cstring>

#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "Nodes.hpp"
#include "WebPortal.hpp"
#include "WebServer.h"

extern WebServerCapture wsCapture;
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
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_STREQ(a, b) ASSERT_TRUE(std::strcmp((a), (b)) == 0)

namespace {
PoolController::ControllerCommand capturedCommand{};
PoolController::ControllerCommandResult configuredResult{};
bool dispatchCalled = false;
bool dispatchAccepted = true;

bool captureDispatch(const PoolController::ControllerCommand &command, PoolController::ControllerCommandResult *result) {
  dispatchCalled = true;
  capturedCommand = command;
  if (result != nullptr) {
    *result = configuredResult;
  }
  return dispatchAccepted;
}

void resetRequest() {
  wsCapture.clear();
  PoolController::ConfigManager::setConfigured(false);
  PoolController::NetworkManager::setApMode(true);
  PoolController::WebPortal::setCommandDispatcher(captureDispatch);
  dispatchCalled = false;
  dispatchAccepted = true;
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
  test_begin("WebRuntimeCommands", "owner rejection maps to the manual-mode HTTP error");
  resetRequest();
  wsCapture.args["pump"] = "pool";
  dispatchAccepted = false;
  ASSERT_TRUE(PoolController::WebPortal::invokeRouteForTest("/api/pump", HTTP_POST));
  ASSERT_EQ(wsCapture.lastStatusCode, 400);
  ASSERT_STREQ(wsCapture.lastBody.c_str(), "{\"status\":\"error\",\"message\":\"Pump control only available in manual mode\"}");
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
  int (*tests[])() = {test_mode_validation, test_absolute_mode, test_cycle_mode, test_pump_validation, test_relative_pumps,
    test_owner_rejection, test_adapter_does_not_mutate_nodes};
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
