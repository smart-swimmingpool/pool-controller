from pathlib import Path
import re


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one match, got {count}: {old[:120]!r}")
    p.write_text(text.replace(old, new, 1))


def regex_once(path: str, pattern: str, replacement: str) -> None:
    p = Path(path)
    text = p.read_text()
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.S)
    if count != 1:
        raise RuntimeError(f"{path}: regex expected one match, got {count}: {pattern[:120]!r}")
    p.write_text(updated)


# Controller command handler: synchronous adapters can consume owner-resolved state.
replace_once(
    "src/ControllerCommandHandler.hpp",
    "namespace PoolController {\n\n/**\n * @brief Applies validated runtime commands to the legacy controller model.",
    "namespace PoolController {\n\n/** @brief Optional resolved owner state returned to synchronous adapters. */\n"
    "struct ControllerCommandResult final {\n"
    "  bool hasMode{false};\n"
    "  OperationMode mode{OperationMode::AUTO};\n"
    "  bool hasPumpState{false};\n"
    "  bool pumpState{false};\n"
    "};\n\n"
    "/**\n * @brief Applies validated runtime commands to the legacy controller model."
)
replace_once(
    "src/ControllerCommandHandler.hpp",
    "  bool handle(const ControllerCommand &command);",
    "  bool handle(const ControllerCommand &command) { return handle(command, nullptr); }\n"
    "  bool handle(const ControllerCommand &command, ControllerCommandResult *result);"
)
replace_once(
    "src/ControllerCommandHandler.cpp",
    "bool inRange(float value, float minimum, float maximum) {\n  return value >= minimum && value <= maximum;\n}\n\n}  // namespace",
    "bool inRange(float value, float minimum, float maximum) {\n  return value >= minimum && value <= maximum;\n}\n\n"
    "OperationMode nextOperationMode(OperationMode current) {\n"
    "  switch (current) {\n"
    "  case OperationMode::AUTO:\n    return OperationMode::MANUAL;\n"
    "  case OperationMode::MANUAL:\n    return OperationMode::BOOST;\n"
    "  case OperationMode::BOOST:\n    return OperationMode::TIMER;\n"
    "  case OperationMode::TIMER:\n    return OperationMode::AUTO;\n"
    "  }\n  return OperationMode::AUTO;\n}\n\n}  // namespace"
)
replace_once(
    "src/ControllerCommandHandler.cpp",
    "bool ControllerCommandHandler::handle(const ControllerCommand &command) {",
    "bool ControllerCommandHandler::handle(const ControllerCommand &command, ControllerCommandResult *result) {"
)
regex_once(
    "src/ControllerCommandHandler.cpp",
    r"  case ControllerCommandType::SET_MODE:\n.*?    return saveSettings\(\);",
    """  case ControllerCommandType::SET_MODE:
    if (!dependencies_.operationMode.setMode(command.mode, sourceName(command.source))) {
      return false;
    }
    settings.opMode = toString(command.mode);
    if (!saveSettings()) {
      return false;
    }
    if (result != nullptr) {
      result->hasMode = true;
      result->mode = command.mode;
    }
    return true;

  case ControllerCommandType::CYCLE_MODE: {
    const OperationMode next = nextOperationMode(dependencies_.operationMode.getTypedMode());
    if (!dependencies_.operationMode.setMode(next, sourceName(command.source))) {
      return false;
    }
    settings.opMode = toString(next);
    if (!saveSettings()) {
      return false;
    }
    if (result != nullptr) {
      result->hasMode = true;
      result->mode = next;
    }
    return true;
  }"""
)
replace_once(
    "src/ControllerCommandHandler.cpp",
    "  case ControllerCommandType::FACTORY_RESET: {",
    """  case ControllerCommandType::TOGGLE_POOL_PUMP:
  case ControllerCommandType::TOGGLE_SOLAR_PUMP: {
    const OperationMode currentMode = dependencies_.operationMode.getTypedMode();
    if (command.toggleModePolicy == PumpToggleModePolicy::REQUIRE_MANUAL && currentMode != OperationMode::MANUAL) {
      LOG_WARN("Controller command: Web pump toggle rejected outside manual mode\\n");
      return false;
    }
    if (command.toggleModePolicy == PumpToggleModePolicy::ENTER_MANUAL && currentMode != OperationMode::MANUAL) {
      if (!dependencies_.operationMode.setMode(OperationMode::MANUAL, sourceName(command.source))) {
        return false;
      }
      settings.opMode = toString(OperationMode::MANUAL);
      if (!saveSettings()) {
        return false;
      }
    }

    RelayModuleNode &pump =
      command.type == ControllerCommandType::TOGGLE_POOL_PUMP ? dependencies_.poolPump : dependencies_.solarPump;
    const bool newState = !pump.getSwitch();
    pump.setSwitch(newState);
    if (result != nullptr) {
      result->hasPumpState = true;
      result->pumpState = newState;
    }
    return true;
  }

  case ControllerCommandType::FACTORY_RESET: {"""
)

# WebPortal: inject application-owned dispatcher and migrate only /api/mode and /api/pump.
replace_once(
    "src/WebPortal.hpp",
    '#include "LogCapture.hpp"',
    '#include "ControllerCommandHandler.hpp"\n#include "LogCapture.hpp"'
)
replace_once(
    "src/WebPortal.hpp",
    "  WebPortal() = default;\n\n  /** @brief Start the HTTP server and optional DNS captive portal. @return true if server started successfully. */",
    "  WebPortal() = default;\n\n"
    "  using CommandDispatcher = bool (*)(const ControllerCommand &, ControllerCommandResult *);\n"
    "  static void setCommandDispatcher(CommandDispatcher dispatcher) { commandDispatcher_ = dispatcher; }\n"
    "#if !defined(ESP32) && !defined(ARDUINO_ARCH_ESP32)\n"
    "  static bool invokeRouteForTest(const char *uri, int method) { return server_.invokeRoute(uri, method); }\n"
    "#endif\n\n"
    "  /** @brief Start the HTTP server and optional DNS captive portal. @return true if server started successfully. */"
)
replace_once(
    "src/WebPortal.hpp",
    "  static WebServer server_;\n  static DNSServer dnsServer_;",
    "  static WebServer server_;\n  static DNSServer dnsServer_;\n  static CommandDispatcher commandDispatcher_;"
)
replace_once(
    "src/WebPortal.cpp",
    "WebServer WebPortal::server_(80);\nDNSServer WebPortal::dnsServer_;",
    "WebServer WebPortal::server_(80);\nDNSServer WebPortal::dnsServer_;\nWebPortal::CommandDispatcher WebPortal::commandDispatcher_ = nullptr;"
)
regex_once(
    "src/WebPortal.cpp",
    r"void WebPortal::apiSetMode\(\) \{.*?\n\}\n\nvoid WebPortal::apiTogglePump\(\) \{.*?\n\}\n\nbool WebPortal::isLoginLockedOut\(\)",
    r'''void WebPortal::apiSetMode() {
  if (!server_.hasArg("mode")) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing mode\"}");
    return;
  }

  const String requestedMode = server_.arg("mode");
  ControllerCommand command{};
  command.source = CommandSource::WEB;

  if (requestedMode == "cycle") {
    command.type = ControllerCommandType::CYCLE_MODE;
  } else {
    OperationMode parsedMode;
    if (!tryParseOperationMode(requestedMode.c_str(), parsedMode)) {
      server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid mode\"}");
      return;
    }
    command.type = ControllerCommandType::SET_MODE;
    command.mode = parsedMode;
  }

  ControllerCommandResult result{};
  if (commandDispatcher_ == nullptr || !commandDispatcher_(command, &result)) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid mode\"}");
    return;
  }

  const char *resolvedMode = requestedMode.c_str();
  if (command.type == ControllerCommandType::CYCLE_MODE) {
    if (!result.hasMode) {
      server_.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Missing command result\"}");
      return;
    }
    resolvedMode = toString(result.mode);
  }
  server_.send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"" + String(resolvedMode) + "\"}");
}

void WebPortal::apiTogglePump() {
  if (!server_.hasArg("pump")) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing pump parameter\"}");
    return;
  }

  const String pump = server_.arg("pump");
  ControllerCommand command{};
  command.source = CommandSource::WEB;
  command.toggleModePolicy = PumpToggleModePolicy::REQUIRE_MANUAL;
  if (pump == "pool") {
    command.type = ControllerCommandType::TOGGLE_POOL_PUMP;
  } else if (pump == "solar") {
    command.type = ControllerCommandType::TOGGLE_SOLAR_PUMP;
  } else {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid pump. Use 'pool' or 'solar'\"}");
    return;
  }

  ControllerCommandResult result{};
  if (commandDispatcher_ == nullptr || !commandDispatcher_(command, &result)) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Pump control only available in manual mode\"}");
    return;
  }
  if (!result.hasPumpState) {
    server_.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Missing command result\"}");
    return;
  }

  String json = "{\"status\":\"ok\",\"state\":" + String(result.pumpState ? "true" : "false") + "}";
  server_.send(200, "application/json", json);
}

bool WebPortal::isLoginLockedOut()'''
)

# Composition root wires Web to the same owner as MQTT.
replace_once("src/main.cpp", '#include "PoolController.hpp"', '#include "PoolController.hpp"\n#include "WebPortal.hpp"')
replace_once(
    "src/main.cpp",
    "PoolController::MqttRuntimeCommandAdapter &mqttRuntimeCommandAdapter() {",
    "bool dispatchWebCommand(const PoolController::ControllerCommand &command, PoolController::ControllerCommandResult *result) {\n"
    "  return controllerCommandHandler().handle(command, result);\n}\n\n"
    "PoolController::MqttRuntimeCommandAdapter &mqttRuntimeCommandAdapter() {"
)
replace_once(
    "src/main.cpp",
    "  context.setup();\n\n  // MqttPublisher::begin()",
    "  PoolController::WebPortal::setCommandDispatcher(dispatchWebCommand);\n  context.setup();\n\n  // MqttPublisher::begin()"
)

# Focused native route-level tests.
Path("test/native/tests/test_web_runtime_commands.cpp").write_text(r'''// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
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
  int (*tests[])() = {test_mode_validation, test_absolute_mode, test_cycle_mode, test_pump_validation,
    test_relative_pumps, test_owner_rejection, test_adapter_does_not_mutate_nodes};
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
''')

replace_once(
    "test/native/CMakeLists.txt",
    "  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_webportal_json.cpp\n",
    "  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_webportal_json.cpp\n  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_web_runtime_commands.cpp\n"
)
replace_once(
    "test/native/tests/test_main.cpp",
    "extern int run_webportal_json_tests();\n",
    "extern int run_webportal_json_tests();\nextern int run_web_runtime_command_tests();\n"
)
replace_once(
    "test/native/tests/test_main.cpp",
    "  total += run_webportal_json_tests();\n",
    "  total += run_webportal_json_tests();\n  total += run_web_runtime_command_tests();\n"
)

replace_once(
    "openspec/changes/adapter-command-snapshot-migration/tasks.md",
    "- [ ] Route Web runtime mutations through the same command handler.",
    "- [x] Route `/api/mode` and `/api/pump` Web mutations through the same command handler using owner-resolved relative actions.\n"
    "- [ ] Route the remaining Web runtime mutations through the same command handler."
)

# Ownership gate for the migrated handlers.
web = Path("src/WebPortal.cpp").read_text()
mode = web[web.index("void WebPortal::apiSetMode()"):web.index("void WebPortal::apiTogglePump()")]
pump = web[web.index("void WebPortal::apiTogglePump()"):web.index("bool WebPortal::isLoginLockedOut()")]
for forbidden in ("operationModeNode", "poolPumpNode", "solarPumpNode", "ConfigManager::getSettings()"):
    if forbidden in mode or forbidden in pump:
        raise RuntimeError(f"migrated Web handler still accesses mutable state directly: {forbidden}")
