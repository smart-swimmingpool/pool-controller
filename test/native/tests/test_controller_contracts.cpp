// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_controller_contracts.cpp
 * @brief Native tests for typed controller commands and read-model contracts.
 */

#include <cstring>
#include <type_traits>

#include "ControllerCommand.hpp"
#include "ControllerSnapshot.hpp"
#include "OperationMode.hpp"

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

using PoolController::CommandSource;
using PoolController::ControllerCommand;
using PoolController::ControllerCommandType;
using PoolController::OperationMode;
using PoolController::SensorSnapshot;
using PoolController::SystemSnapshot;
using PoolController::TimeDegradationState;

static int test_operation_mode_wire_values() {
  test_begin("ControllerContracts", "operation modes preserve stable wire values");
  ASSERT_EQ(std::strcmp(PoolController::toString(OperationMode::AUTO), "auto"), 0);
  ASSERT_EQ(std::strcmp(PoolController::toString(OperationMode::MANUAL), "manu"), 0);
  ASSERT_EQ(std::strcmp(PoolController::toString(OperationMode::BOOST), "boost"), 0);
  ASSERT_EQ(std::strcmp(PoolController::toString(OperationMode::TIMER), "timer"), 0);
  return 0;
}

static int test_operation_mode_parser() {
  test_begin("ControllerContracts", "operation mode parser rejects unknown values without allocation");
  OperationMode mode = OperationMode::TIMER;
  ASSERT_TRUE(PoolController::tryParseOperationMode("auto", mode));
  ASSERT_EQ(mode, OperationMode::AUTO);
  ASSERT_TRUE(PoolController::tryParseOperationMode("manu", mode));
  ASSERT_EQ(mode, OperationMode::MANUAL);
  ASSERT_TRUE(!PoolController::tryParseOperationMode("manual", mode));
  ASSERT_TRUE(!PoolController::tryParseOperationMode(nullptr, mode));
  return 0;
}

static int test_commands_are_fixed_size_values() {
  test_begin("ControllerContracts", "controller commands are trivially copyable fixed-size values");
  ControllerCommand command{};
  command.type = ControllerCommandType::SET_MODE;
  command.source = CommandSource::MQTT;
  command.mode = OperationMode::BOOST;

  ControllerCommand copy = command;
  ASSERT_EQ(copy.type, ControllerCommandType::SET_MODE);
  ASSERT_EQ(copy.source, CommandSource::MQTT);
  ASSERT_EQ(copy.mode, OperationMode::BOOST);
  ASSERT_TRUE(std::is_trivially_copyable<ControllerCommand>::value);
  return 0;
}

static int test_commands_cover_circulation_settings() {
  test_begin("ControllerContracts", "command contract covers all temperature circulation setters");
  ControllerCommand command{};

  command.type = ControllerCommandType::SET_TEMPERATURE_CIRCULATION_THRESHOLD;
  command.value = 24.5F;
  ASSERT_EQ(command.type, ControllerCommandType::SET_TEMPERATURE_CIRCULATION_THRESHOLD);
  ASSERT_EQ(command.value, 24.5F);

  command.type = ControllerCommandType::SET_TEMPERATURE_CIRCULATION_FACTOR;
  command.integerValue = 45;
  ASSERT_EQ(command.type, ControllerCommandType::SET_TEMPERATURE_CIRCULATION_FACTOR);
  ASSERT_EQ(command.integerValue, 45);

  command.type = ControllerCommandType::SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME;
  command.integerValue = 360;
  ASSERT_EQ(command.type, ControllerCommandType::SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME);
  ASSERT_EQ(command.integerValue, 360);
  return 0;
}

static int test_timer_commands_preserve_partial_updates() {
  test_begin("ControllerContracts", "timer start and end are independent commands");
  ControllerCommand start{};
  start.type = ControllerCommandType::SET_TIMER_START;
  start.hour = 7;
  start.minute = 30;

  ControllerCommand end{};
  end.type = ControllerCommandType::SET_TIMER_END;
  end.hour = 19;
  end.minute = 15;

  ASSERT_EQ(start.type, ControllerCommandType::SET_TIMER_START);
  ASSERT_EQ(start.hour, 7U);
  ASSERT_EQ(start.minute, 30U);
  ASSERT_EQ(end.type, ControllerCommandType::SET_TIMER_END);
  ASSERT_EQ(end.hour, 19U);
  ASSERT_EQ(end.minute, 15U);
  return 0;
}

static int test_commands_cover_runtime_controller_settings() {
  test_begin("ControllerContracts", "commands cover mutable runtime controller settings");
  const ControllerCommandType types[] = {ControllerCommandType::SET_LOOP_INTERVAL, ControllerCommandType::SET_TIMEZONE,
    ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS, ControllerCommandType::SET_TIME_LOSS_RED_HOURS,
    ControllerCommandType::SET_BUTTON_1_MIN, ControllerCommandType::SET_BUTTON_1_MAX, ControllerCommandType::SET_BUTTON_2_MIN,
    ControllerCommandType::SET_BUTTON_2_MAX, ControllerCommandType::SET_BUTTON_3_MIN, ControllerCommandType::SET_BUTTON_3_MAX,
    ControllerCommandType::SET_BUTTON_NO_PRESS};

  ControllerCommand command{};
  command.integerValue = 1234;
  for (const auto type : types) {
    command.type = type;
    ASSERT_EQ(command.type, type);
    ASSERT_EQ(command.integerValue, 1234);
  }
  return 0;
}

static int test_ntp_text_boundaries() {
  test_begin("ControllerContracts", "NTP text rejects malformed input without truncation or mutation");
  PoolController::NtpServerValue server{};
  ASSERT_TRUE(!server.valid());
  ASSERT_TRUE(!server.assign(nullptr, 5));
  ASSERT_TRUE(!server.assign("", 0));

  // A callback buffer need not be NUL terminated. All 127 bytes must survive.
  std::array<char, 128> input{};
  input.fill('a');
  ASSERT_TRUE(server.assign(input.data(), 127));
  ASSERT_TRUE(server.valid());
  ASSERT_EQ(server.text[126], 'a');
  ASSERT_EQ(server.text[127], '\0');
  const auto previous = server.text;
  ASSERT_TRUE(!server.assign(input.data(), 128));
  ASSERT_TRUE(!server.assign(input.data(), 10000));
  ASSERT_TRUE(!server.assign(nullptr, 1));
  ASSERT_TRUE(!server.assign("", 0));
  const char embeddedNul[] = {'a', '\0', 'b'};
  ASSERT_TRUE(!server.assign(embeddedNul, sizeof(embeddedNul)));
  ASSERT_EQ(server.text, previous);

  ASSERT_TRUE(server.assign("pool.ntp.org", 12));
  ASSERT_EQ(std::strcmp(server.text.data(), "pool.ntp.org"), 0);
  for (std::size_t i = 12; i < server.text.size(); ++i) {
    ASSERT_EQ(server.text[i], '\0');
  }
  ASSERT_TRUE(server.assign(server.text.data() + 5, 7));
  ASSERT_EQ(std::strcmp(server.text.data(), "ntp.org"), 0);
  ASSERT_TRUE(server.assign("x", 1));
  ASSERT_TRUE(server.valid());
  server.text.fill('x');
  ASSERT_TRUE(!server.valid());

  // Noncanonical raw queued values must be rejected: a terminator followed by
  // nonzero bytes is not a value assign() could have produced.
  PoolController::NtpServerValue raw{};
  ASSERT_TRUE(!raw.valid());
  const char noncanonical[] = {'a', '\0', 'b'};
  std::memcpy(raw.text.data(), noncanonical, sizeof(noncanonical));
  ASSERT_TRUE(!raw.valid());
  raw.text.fill('a');
  ASSERT_TRUE(!raw.valid());
  raw.text[127] = '\0';
  ASSERT_TRUE(raw.valid());
  raw.text[0] = '\0';
  raw.text[1] = 'a';
  ASSERT_TRUE(!raw.valid());
  ASSERT_TRUE(raw.assign("pool.ntp.org", 12));
  ASSERT_TRUE(raw.valid());
  return 0;
}

static int test_ntp_command_and_snapshot_own_text() {
  test_begin("ControllerContracts", "NTP command and snapshot own independent bounded text");
  ControllerCommand command{};
  command.type = ControllerCommandType::SET_NTP_SERVER;
  command.source = CommandSource::WEB;
  char callbackBuffer[] = "pool.ntp.org";
  ASSERT_TRUE(command.ntpServer.assign(callbackBuffer, sizeof(callbackBuffer) - 1));
  callbackBuffer[0] = 'X';

  const ControllerCommand queued = command;
  SystemSnapshot snapshot{};
  snapshot.settings.ntpServer = queued.ntpServer;
  const SystemSnapshot published = snapshot;
  ASSERT_TRUE(command.ntpServer.assign("time.example", 12));
  ASSERT_TRUE(snapshot.settings.ntpServer.assign("time.example", 12));
  ASSERT_EQ(queued.type, ControllerCommandType::SET_NTP_SERVER);
  ASSERT_EQ(queued.source, CommandSource::WEB);
  ASSERT_EQ(std::strcmp(queued.ntpServer.text.data(), "pool.ntp.org"), 0);
  ASSERT_EQ(published.settings.ntpServer.text, queued.ntpServer.text);
  ASSERT_TRUE(published.settings.ntpServer.valid());
  ASSERT_TRUE(std::is_trivially_copyable<PoolController::NtpServerValue>::value);
  return 0;
}

static int test_relative_commands_preserve_intent() {
  test_begin("ControllerContracts", "relative actions remain distinct from absolute setters in value transport");
  const ControllerCommandType relative[] = {
    ControllerCommandType::TOGGLE_POOL_PUMP, ControllerCommandType::TOGGLE_SOLAR_PUMP, ControllerCommandType::CYCLE_MODE};
  const ControllerCommandType absolute[] = {
    ControllerCommandType::SET_POOL_PUMP_MANUAL, ControllerCommandType::SET_SOLAR_PUMP_MANUAL, ControllerCommandType::SET_MODE};
  for (std::size_t i = 0; i < 3; ++i) {
    ControllerCommand action{};
    action.type = relative[i];
    action.source = CommandSource::LOCAL_UI;
    const std::array<ControllerCommand, 2> pending{{action, action}};
    ASSERT_TRUE(pending[0].type != absolute[i]);
    ASSERT_EQ(pending[0].type, relative[i]);
    ASSERT_EQ(pending[1].type, relative[i]);
    ASSERT_EQ(pending[1].source, CommandSource::LOCAL_UI);
  }
  ControllerCommand toggle{};
  toggle.type = ControllerCommandType::TOGGLE_POOL_PUMP;
  ASSERT_EQ(toggle.toggleModePolicy, PoolController::PumpToggleModePolicy::REQUIRE_MANUAL);
  toggle.toggleModePolicy = PoolController::PumpToggleModePolicy::ENTER_MANUAL;
  const auto olimex = toggle;
  toggle.toggleModePolicy = PoolController::PumpToggleModePolicy::KEEP_MODE;
  ASSERT_EQ(olimex.toggleModePolicy, PoolController::PumpToggleModePolicy::ENTER_MANUAL);
  ASSERT_EQ(toggle.toggleModePolicy, PoolController::PumpToggleModePolicy::KEEP_MODE);
  // Live-state execution and two-toggle/four-cycle regressions belong to the
  // command handler in #218; this PR only establishes the transport contract.
  return 0;
}

static int test_detected_inventory_capacity_and_copy() {
  test_begin("ControllerContracts", "sensor inventory preserves all 20 devices and independent reading validity");
  SensorSnapshot sensors{};
  ASSERT_EQ(sensors.detectedCount, 0U);
  ASSERT_EQ(sensors.detected.size(), 20U);
  ASSERT_TRUE(!sensors.detected[0].temperature.valid);
  // The inventory includes devices that have not been assigned to either role.
  ASSERT_TRUE(!sensors.poolMapping.configured);
  ASSERT_TRUE(!sensors.solarMapping.configured);
  sensors.generation = 23;
  sensors.measuredAtMs = 4567;
  for (std::size_t i = 0; i < sensors.detected.size(); ++i) {
    auto &device = sensors.detected[i];
    device.address = {0x28, 1, 2, 3, 4, 5, 6, static_cast<std::uint8_t>(i)};
    device.temperature.value = 20.0F + static_cast<float>(i);
    device.temperature.valid = i % 2 == 0;
    ++sensors.detectedCount;
  }
  SystemSnapshot snapshot{};
  snapshot.sensors = sensors;
  const auto published = snapshot;
  sensors = SensorSnapshot{};
  snapshot = SystemSnapshot{};
  ASSERT_EQ(published.sensors.detectedCount, 20U);
  ASSERT_EQ(published.sensors.generation, 23U);
  ASSERT_EQ(published.sensors.measuredAtMs, 4567U);
  for (std::size_t i = 0; i < published.sensors.detectedCount; ++i) {
    const auto &device = published.sensors.detected[i];
    ASSERT_EQ(device.address[0], 0x28U);
    ASSERT_EQ(device.address[7], i);
    ASSERT_EQ(device.temperature.value, 20.0F + static_cast<float>(i));
    ASSERT_EQ(device.temperature.valid, i % 2 == 0);
  }
  ASSERT_EQ(snapshot.sensors.detectedCount, 0U);
  return 0;
}

static int test_snapshots_are_coherent_value_objects() {
  test_begin("ControllerContracts", "snapshots carry complete adapter-facing application projection");
  SystemSnapshot snapshot{};
  snapshot.sensors.pool.value = 26.5F;
  snapshot.sensors.pool.valid = true;
  snapshot.sensors.generation = 42;
  snapshot.sensors.solarMapping.configured = true;
  snapshot.sensors.solarMapping.found = false;
  snapshot.sensors.solarMapping.address = {0x28, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  snapshot.mode = OperationMode::AUTO;
  snapshot.poolPumpOn = true;
  snapshot.health.timeValid = true;
  snapshot.health.timeDegradation = TimeDegradationState::YELLOW;
  snapshot.circulation.threshold = 23.0F;
  snapshot.circulation.factorMinutesPerDegree = 30;
  snapshot.circulation.maxRuntimeMinutes = 480;
  snapshot.timer.startHour = 8;
  snapshot.timer.startMinute = 15;
  snapshot.timer.endHour = 18;
  snapshot.timer.endMinute = 45;
  snapshot.timer.effectiveRuntimeMinutes = 210;
  snapshot.timer.circulationExtensionMinutes = 30;
  snapshot.timer.activeEndMinutes = 1155;
  snapshot.settings.loopInterval = 10;
  snapshot.settings.timezoneIndex = 1;
  snapshot.settings.timeLossGreenHours = 1;
  snapshot.settings.timeLossRedHours = 24;
  snapshot.settings.button1Min = 3100;
  snapshot.settings.buttonNoPress = 4096;

  const SystemSnapshot copy = snapshot;
  ASSERT_TRUE(copy.sensors.pool.valid);
  ASSERT_EQ(copy.sensors.pool.value, 26.5F);
  ASSERT_EQ(copy.sensors.generation, 42U);
  ASSERT_TRUE(copy.sensors.solarMapping.configured);
  ASSERT_TRUE(!copy.sensors.solarMapping.found);
  ASSERT_EQ(copy.sensors.solarMapping.address[0], 0x28U);
  ASSERT_EQ(copy.sensors.solarMapping.address[7], 0x07U);
  ASSERT_EQ(copy.mode, OperationMode::AUTO);
  ASSERT_TRUE(copy.poolPumpOn);
  ASSERT_EQ(copy.health.timeDegradation, TimeDegradationState::YELLOW);
  ASSERT_EQ(copy.circulation.threshold, 23.0F);
  ASSERT_EQ(copy.circulation.factorMinutesPerDegree, 30U);
  ASSERT_EQ(copy.circulation.maxRuntimeMinutes, 480U);
  ASSERT_EQ(copy.timer.startHour, 8U);
  ASSERT_EQ(copy.timer.endMinute, 45U);
  ASSERT_EQ(copy.timer.effectiveRuntimeMinutes, 210U);
  ASSERT_EQ(copy.timer.circulationExtensionMinutes, 30U);
  ASSERT_EQ(copy.timer.activeEndMinutes, 1155U);
  ASSERT_EQ(copy.settings.loopInterval, 10U);
  ASSERT_EQ(copy.settings.timezoneIndex, 1);
  ASSERT_EQ(copy.settings.timeLossRedHours, 24U);
  ASSERT_EQ(copy.settings.button1Min, 3100U);
  ASSERT_EQ(copy.settings.buttonNoPress, 4096U);
  ASSERT_TRUE(std::is_trivially_copyable<SensorSnapshot>::value);
  ASSERT_TRUE(std::is_trivially_copyable<SystemSnapshot>::value);
  return 0;
}

int run_controller_contract_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_operation_mode_wire_values, test_operation_mode_parser, test_commands_are_fixed_size_values,
    test_commands_cover_circulation_settings, test_timer_commands_preserve_partial_updates,
    test_commands_cover_runtime_controller_settings, test_ntp_text_boundaries, test_ntp_command_and_snapshot_own_text,
    test_relative_commands_preserve_intent, test_detected_inventory_capacity_and_copy, test_snapshots_are_coherent_value_objects};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("ControllerContracts", passed, failed);
  return failed;
}
