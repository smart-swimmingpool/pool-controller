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
    test_commands_cover_runtime_controller_settings, test_snapshots_are_coherent_value_objects};
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
