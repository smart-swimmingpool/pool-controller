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

using PoolController::CommandSource;
using PoolController::ControllerCommand;
using PoolController::ControllerCommandType;
using PoolController::OperationMode;
using PoolController::SensorSnapshot;
using PoolController::SystemSnapshot;

static int test_operation_mode_wire_values() {
  test_begin("ControllerContracts", "operation modes preserve stable wire values");
  ASSERT_TRUE(std::strcmp(PoolController::toString(OperationMode::AUTO), "auto") == 0);
  ASSERT_TRUE(std::strcmp(PoolController::toString(OperationMode::MANUAL), "manu") == 0);
  ASSERT_TRUE(std::strcmp(PoolController::toString(OperationMode::BOOST), "boost") == 0);
  ASSERT_TRUE(std::strcmp(PoolController::toString(OperationMode::TIMER), "timer") == 0);
  return 0;
}

static int test_operation_mode_parser() {
  test_begin("ControllerContracts", "operation mode parser rejects unknown values without allocation");
  OperationMode mode = OperationMode::TIMER;
  ASSERT_TRUE(PoolController::tryParseOperationMode("auto", mode));
  ASSERT_TRUE(mode == OperationMode::AUTO);
  ASSERT_TRUE(PoolController::tryParseOperationMode("manu", mode));
  ASSERT_TRUE(mode == OperationMode::MANUAL);
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
  ASSERT_TRUE(copy.type == ControllerCommandType::SET_MODE);
  ASSERT_TRUE(copy.source == CommandSource::MQTT);
  ASSERT_TRUE(copy.mode == OperationMode::BOOST);
  ASSERT_TRUE(std::is_trivially_copyable<ControllerCommand>::value);
  return 0;
}

static int test_snapshots_are_coherent_value_objects() {
  test_begin("ControllerContracts", "snapshots carry generation and application projection");
  SystemSnapshot snapshot{};
  snapshot.sensors.pool.value = 26.5F;
  snapshot.sensors.pool.valid = true;
  snapshot.sensors.generation = 42;
  snapshot.mode = OperationMode::AUTO;
  snapshot.poolPumpOn = true;

  const SystemSnapshot copy = snapshot;
  ASSERT_TRUE(copy.sensors.pool.valid);
  ASSERT_TRUE(copy.sensors.pool.value == 26.5F);
  ASSERT_TRUE(copy.sensors.generation == 42U);
  ASSERT_TRUE(copy.mode == OperationMode::AUTO);
  ASSERT_TRUE(copy.poolPumpOn);
  ASSERT_TRUE(std::is_trivially_copyable<SensorSnapshot>::value);
  ASSERT_TRUE(std::is_trivially_copyable<SystemSnapshot>::value);
  return 0;
}

int run_controller_contract_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_operation_mode_wire_values, test_operation_mode_parser, test_commands_are_fixed_size_values,
    test_snapshots_are_coherent_value_objects};
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
