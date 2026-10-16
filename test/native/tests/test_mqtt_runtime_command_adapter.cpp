// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "MqttRuntimeCommandAdapter.hpp"

#include "ConfigManager.hpp"
#include "ControllerCommandHandler.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "MqttPublisher.hpp"
#include "NetworkManager.hpp"
#include "OperationModeNode.hpp"
#include "RelayModuleNode.hpp"

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

using PoolController::ControllerCommandHandler;
using PoolController::MqttRuntimeCommandAdapter;
using PoolController::OperationMode;

struct AdapterFixture {
  OperationModeNode operation{"operation-mode", "Operation Mode", 300};
  RelayModuleNode poolPump{"pool", "Pool", 1};
  RelayModuleNode solarPump{"solar", "Solar", 2};
  DallasTemperatureNode solarTemperature{PoolController::SensorId::SOLAR, "solar-temp", "Solar Temperature", 3};
  DallasTemperatureNode poolTemperature{PoolController::SensorId::POOL, "pool-temp", "Pool Temperature", 4};
  ESP32TemperatureNode controllerTemperature{"controller-temp", "Controller Temperature"};
  ControllerCommandHandler handler{ControllerCommandHandler::Dependencies{
    operation, poolPump, solarPump, solarTemperature, poolTemperature, controllerTemperature}};
  MqttRuntimeCommandAdapter adapter{handler};

  void begin() {
    // AsyncMqttClient appends callbacks with no removal API: clear stale
    // registrations from previous fixtures before wiring this instance.
    mqttCapture.clear();
    PoolController::NetworkManager::clearMqttCallback();
    // Production order (main.cpp): the adapter claims the callback before
    // context.setup() connects MQTT and runs MqttPublisher::begin().
    adapter.begin();
    PoolController::MqttPublisher::begin();
  }
};

static int test_callback_does_not_mutate_controller_state() {
  test_begin("MqttRuntimeCommandAdapter", "Async MQTT callback only queues; loop applies typed command");
  AdapterFixture fixture;
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.begin();

  PoolController::NetworkManager::getClient().simulateMessage("homeassistant/select/pool-controller/mode/set", "boost");
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::AUTO);

  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::BOOST);
  ASSERT_EQ(PoolController::ConfigManager::getSettings().opMode.compare("boost"), 0);
  return 0;
}

static int test_commands_remain_bounded_per_loop() {
  test_begin("MqttRuntimeCommandAdapter", "runtime adapter drains at most two MQTT messages per loop");
  AdapterFixture fixture;
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.begin();

  auto &client = PoolController::NetworkManager::getClient();
  client.simulateMessage("homeassistant/select/pool-controller/mode/set", "manu");
  client.simulateMessage("homeassistant/select/pool-controller/mode/set", "boost");
  client.simulateMessage("homeassistant/select/pool-controller/mode/set", "timer");

  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::BOOST);
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::TIMER);
  return 0;
}

static int test_timer_start_and_end_are_independent_mqtt_commands() {
  test_begin("MqttRuntimeCommandAdapter", "MQTT timer updates preserve both independently queued halves");
  AdapterFixture fixture;
  TimerSetting timer{};
  timer.timerStartHour = 6;
  timer.timerStartMinutes = 0;
  timer.timerEndHour = 18;
  timer.timerEndMinutes = 0;
  fixture.operation.setTimerSetting(timer);
  fixture.begin();

  auto &client = PoolController::NetworkManager::getClient();
  client.simulateMessage("homeassistant/time/pool-controller/timer-start/set", "07:30:00");
  client.simulateMessage("homeassistant/time/pool-controller/timer-end/set", "20:15:00");
  fixture.adapter.processPendingCommands();

  timer = fixture.operation.getTimerSetting();
  ASSERT_EQ(timer.timerStartHour, 7);
  ASSERT_EQ(timer.timerStartMinutes, 30);
  ASSERT_EQ(timer.timerEndHour, 20);
  ASSERT_EQ(timer.timerEndMinutes, 15);
  return 0;
}

static int test_manual_pump_policy_is_enforced_after_translation() {
  test_begin("MqttRuntimeCommandAdapter", "typed pump command is rejected outside manual mode");
  AdapterFixture fixture;
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.poolPump.setSwitch(false);
  fixture.begin();

  PoolController::NetworkManager::getClient().simulateMessage("homeassistant/switch/pool-controller/pool-pump/set", "ON");
  fixture.adapter.processPendingCommands();
  ASSERT_TRUE(!fixture.poolPump.getSwitch());

  fixture.operation.setMode(OperationMode::MANUAL);
  PoolController::NetworkManager::getClient().simulateMessage("homeassistant/switch/pool-controller/pool-pump/set", "ON");
  fixture.adapter.processPendingCommands();
  ASSERT_TRUE(fixture.poolPump.getSwitch());
  return 0;
}

static int test_non_numeric_setpoint_payload_is_rejected() {
  test_begin("MqttRuntimeCommandAdapter", "non-numeric setpoint payload is rejected instead of becoming 0");
  AdapterFixture fixture;
  fixture.operation.setPoolMaxTemperature(28.0F);
  fixture.begin();
  auto &client = PoolController::NetworkManager::getClient();
  const char *topic = "homeassistant/number/pool-controller/pool-max-temp/set";

  client.simulateMessage(topic, "abc");
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getPoolMaxTemperature(), 28.0F);

  client.simulateMessage(topic, "29.5abc");
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getPoolMaxTemperature(), 28.0F);

  client.simulateMessage(topic, "29.5");
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getPoolMaxTemperature(), 29.5F);
  return 0;
}

extern MqttClientCapture mqttCapture;

static int test_second_registration_replaces_first_callback() {
  test_begin("MqttRuntimeCommandAdapter", "adapter begin() replaces instead of appending the MQTT callback");
  AdapterFixture fixture;
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.begin();
  auto &client = PoolController::NetworkManager::getClient();
  // AsyncMqttClient appends callbacks: the publisher's and the adapter's
  // registrations must not both dispatch the same message.
  ASSERT_EQ(mqttCapture.onMessageCbs.size(), static_cast<size_t>(1));
  client.simulateMessage("homeassistant/select/pool-controller/mode/set", "boost");
  fixture.adapter.processPendingCommands();
  // Exactly one queued translation: mode applied once, generation advanced once.
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::BOOST);
  return 0;
}

static int test_publisher_begin_does_not_take_the_callback() {
  test_begin("MqttRuntimeCommandAdapter", "MqttPublisher::begin() leaves the adapter as the only MQTT callback");
  AdapterFixture fixture;
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.begin();
  // A second publisher begin (e.g. reconnect path) must not swap the handler
  // while the AsyncTCP task may be dispatching messages.
  PoolController::MqttPublisher::begin();
  PoolController::NetworkManager::getClient().simulateMessage("homeassistant/select/pool-controller/mode/set", "boost");
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::BOOST);
  return 0;
}

static int test_climate_topics_map_to_operation_modes() {
  test_begin("MqttRuntimeCommandAdapter", "thermostat mode and preset topics map to operation modes");
  AdapterFixture fixture;
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.begin();
  auto &client = PoolController::NetworkManager::getClient();

  client.simulateMessage("homeassistant/climate/pool-controller/thermostat/mode/set", "heat");
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::BOOST);

  client.simulateMessage("homeassistant/climate/pool-controller/thermostat/preset/set", "schedule");
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::TIMER);

  client.simulateMessage("homeassistant/climate/pool-controller/thermostat/mode/set", "cool");
  fixture.adapter.processPendingCommands();
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::TIMER);
  return 0;
}

static int test_ntp_server_is_a_typed_command() {
  test_begin("MqttRuntimeCommandAdapter", "NTP server topic becomes a typed command");
  AdapterFixture fixture;
  fixture.begin();
  auto &client = PoolController::NetworkManager::getClient();
  client.simulateMessage("homeassistant/text/pool-controller/ntp-server/set", "time.example.org");
  fixture.adapter.processPendingCommands();
  const bool serverApplied = PoolController::ConfigManager::getNtp().server == "time.example.org";
  ASSERT_TRUE(serverApplied);
  return 0;
}

int run_mqtt_runtime_command_adapter_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_callback_does_not_mutate_controller_state, test_commands_remain_bounded_per_loop,
    test_timer_start_and_end_are_independent_mqtt_commands, test_manual_pump_policy_is_enforced_after_translation,
    test_second_registration_replaces_first_callback, test_non_numeric_setpoint_payload_is_rejected,
    test_publisher_begin_does_not_take_the_callback, test_climate_topics_map_to_operation_modes,
    test_ntp_server_is_a_typed_command};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("MqttRuntimeCommandAdapter", passed, failed);
  return failed;
}
