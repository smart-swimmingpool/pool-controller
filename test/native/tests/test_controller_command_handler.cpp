// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "ControllerCommandHandler.hpp"

#include <cstring>

#include "ConfigManager.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
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

using PoolController::CommandSource;
using PoolController::ControllerCommand;
using PoolController::ControllerCommandHandler;
using PoolController::ControllerCommandType;
using PoolController::OperationMode;
using PoolController::SensorRole;

struct HandlerFixture {
  OperationModeNode operation{"operation-mode", "Operation Mode", 300};
  RelayModuleNode poolPump{"pool", "Pool", 1};
  RelayModuleNode solarPump{"solar", "Solar", 2};
  DallasTemperatureNode solarTemperature{"solar-temp", "Solar Temperature", 3};
  DallasTemperatureNode poolTemperature{"pool-temp", "Pool Temperature", 4};
  ESP32TemperatureNode controllerTemperature{"controller-temp", "Controller Temperature"};
  ControllerCommandHandler handler{ControllerCommandHandler::Dependencies{
    operation, poolPump, solarPump, solarTemperature, poolTemperature, controllerTemperature}};
};

static int test_mode_and_setpoint_are_applied_through_one_handler() {
  test_begin("ControllerCommandHandler", "mode and setpoint update runtime and persisted settings");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.opMode = "auto";
  settings.tempMaxPool = 28.0F;

  ControllerCommand mode{};
  mode.type = ControllerCommandType::SET_MODE;
  mode.source = CommandSource::MQTT;
  mode.mode = OperationMode::BOOST;
  ASSERT_TRUE(fixture.handler.handle(mode));
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::BOOST);
  ASSERT_EQ(settings.opMode.compare("boost"), 0);

  ControllerCommand setpoint{};
  setpoint.type = ControllerCommandType::SET_POOL_MAX_TEMPERATURE;
  setpoint.source = CommandSource::MQTT;
  setpoint.value = 29.5F;
  ASSERT_TRUE(fixture.handler.handle(setpoint));
  ASSERT_EQ(fixture.operation.getPoolMaxTemperature(), 29.5F);
  ASSERT_EQ(settings.tempMaxPool, 29.5F);

  setpoint.value = 50.0F;
  ASSERT_TRUE(!fixture.handler.handle(setpoint));
  ASSERT_EQ(fixture.operation.getPoolMaxTemperature(), 29.5F);
  return 0;
}

static int test_persistence_failure_rolls_back_setpoints() {
  test_begin("ControllerCommandHandler", "failed persistence rolls back setpoints and hysteresis");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();

  fixture.operation.setPoolMaxTemperature(28.0F);
  fixture.operation.setSolarMinTemperature(35.0F);
  fixture.operation.setTemperatureHysteresis(1.0F);
  settings.tempMaxPool = 28.0F;
  settings.tempMinSolar = 35.0F;
  settings.tempHysteresis = 1.0F;
  ControllerCommand command{};
  command.type = ControllerCommandType::SET_POOL_MAX_TEMPERATURE;
  command.value = 30.0F;
  PoolController::ConfigManager::_saveFails = true;
  const bool poolAccepted = fixture.handler.handle(command);
  PoolController::ConfigManager::_saveFails = false;
  ASSERT_TRUE(!poolAccepted);
  ASSERT_EQ(fixture.operation.getPoolMaxTemperature(), 28.0F);
  ASSERT_EQ(settings.tempMaxPool, 28.0F);

  command.type = ControllerCommandType::SET_SOLAR_MIN_TEMPERATURE;
  command.value = 45.0F;
  PoolController::ConfigManager::_saveFails = true;
  const bool solarAccepted = fixture.handler.handle(command);
  PoolController::ConfigManager::_saveFails = false;
  ASSERT_TRUE(!solarAccepted);
  ASSERT_EQ(fixture.operation.getSolarMinTemperature(), 35.0F);
  ASSERT_EQ(settings.tempMinSolar, 35.0F);

  command.type = ControllerCommandType::SET_TEMPERATURE_HYSTERESIS;
  command.value = 2.0F;
  PoolController::ConfigManager::_saveFails = true;
  const bool hysteresisAccepted = fixture.handler.handle(command);
  PoolController::ConfigManager::_saveFails = false;
  ASSERT_TRUE(!hysteresisAccepted);
  ASSERT_EQ(fixture.operation.getTemperatureHysteresis(), 1.0F);
  ASSERT_EQ(settings.tempHysteresis, 1.0F);
  return 0;
}

static int test_green_time_loss_rejects_zero() {
  test_begin("ControllerCommandHandler", "green time-loss threshold rejects zero hours");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.timeLossGreenHours = 2;

  ControllerCommand command{};
  command.type = ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS;
  command.integerValue = 0;

  ASSERT_TRUE(!fixture.handler.handle(command));
  ASSERT_EQ(settings.timeLossGreenHours, 2);
  return 0;
}

static int test_timer_updates_are_independent() {
  test_begin("ControllerCommandHandler", "timer start and end updates preserve the opposite half");
  HandlerFixture fixture;
  TimerSetting timer{};
  timer.timerStartHour = 6;
  timer.timerStartMinutes = 15;
  timer.timerEndHour = 18;
  timer.timerEndMinutes = 45;
  fixture.operation.setTimerSetting(timer);

  ControllerCommand start{};
  start.type = ControllerCommandType::SET_TIMER_START;
  start.hour = 7;
  start.minute = 30;
  ASSERT_TRUE(fixture.handler.handle(start));
  timer = fixture.operation.getTimerSetting();
  ASSERT_EQ(timer.timerStartHour, 7);
  ASSERT_EQ(timer.timerStartMinutes, 30);
  ASSERT_EQ(timer.timerEndHour, 18);
  ASSERT_EQ(timer.timerEndMinutes, 45);

  ControllerCommand end{};
  end.type = ControllerCommandType::SET_TIMER_END;
  end.hour = 20;
  end.minute = 5;
  ASSERT_TRUE(fixture.handler.handle(end));
  timer = fixture.operation.getTimerSetting();
  ASSERT_EQ(timer.timerStartHour, 7);
  ASSERT_EQ(timer.timerStartMinutes, 30);
  ASSERT_EQ(timer.timerEndHour, 20);
  ASSERT_EQ(timer.timerEndMinutes, 5);
  return 0;
}

static int test_manual_pump_guard_is_application_policy() {
  test_begin("ControllerCommandHandler", "manual pump writes are rejected outside manual mode");
  HandlerFixture fixture;
  ControllerCommand command{};
  command.type = ControllerCommandType::SET_POOL_PUMP_MANUAL;
  command.enabled = true;

  fixture.operation.setMode(OperationMode::AUTO);
  ASSERT_TRUE(!fixture.handler.handle(command));
  ASSERT_TRUE(!fixture.poolPump.getSwitch());

  fixture.operation.setMode(OperationMode::MANUAL);
  ASSERT_TRUE(fixture.handler.handle(command));
  ASSERT_TRUE(fixture.poolPump.getSwitch());
  return 0;
}

static int test_sensor_mapping_is_persisted_and_applied() {
  test_begin("ControllerCommandHandler", "sensor mapping command updates one logical role");
  HandlerFixture fixture;
  ControllerCommand command{};
  command.type = ControllerCommandType::SET_SENSOR_MAPPING;
  command.sensorRole = SensorRole::SOLAR;
  const std::uint8_t address[8] = {0x28, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  std::memcpy(command.sensorAddress.data(), address, sizeof(address));

  ASSERT_TRUE(fixture.handler.handle(command));
  ASSERT_TRUE(fixture.solarTemperature.hasAddressFilter());
  ASSERT_EQ(std::memcmp(fixture.solarTemperature.getDeviceAddress(), address, sizeof(address)), 0);

  std::uint8_t savedSolar[8] = {0};
  std::uint8_t savedPool[8] = {0};
  ASSERT_TRUE(PoolController::ConfigManager::loadSensorMapping(savedSolar, savedPool));
  ASSERT_EQ(std::memcmp(savedSolar, address, sizeof(address)), 0);

  command.type = ControllerCommandType::CLEAR_SENSOR_MAPPING;
  ASSERT_TRUE(fixture.handler.handle(command));
  ASSERT_TRUE(!fixture.solarTemperature.hasAddressFilter());
  return 0;
}

static int test_setting_missing_sensor_role_preserves_existing_mapping() {
  test_begin("ControllerCommandHandler", "setting missing sensor role preserves independently stored mapping");
  HandlerFixture fixture;

  const std::uint8_t empty[8] = {0};
  const std::uint8_t existingPool[8] = {0x28, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7};
  const std::uint8_t newSolar[8] = {0x28, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7};
  PoolController::ConfigManager::seedSensorMappingForTest(empty, false, existingPool, true);

  ControllerCommand command{};
  command.type = ControllerCommandType::SET_SENSOR_MAPPING;
  command.sensorRole = SensorRole::SOLAR;
  std::memcpy(command.sensorAddress.data(), newSolar, 8);
  ASSERT_TRUE(fixture.handler.handle(command));

  std::uint8_t savedSolar[8] = {0};
  std::uint8_t savedPool[8] = {0};
  ASSERT_TRUE(PoolController::ConfigManager::loadSensorMapping(savedSolar, savedPool));
  ASSERT_EQ(std::memcmp(savedSolar, newSolar, 8), 0);
  ASSERT_EQ(std::memcmp(savedPool, existingPool, 8), 0);
  return 0;
}

static int test_loop_interval_updates_all_runtime_nodes() {
  test_begin("ControllerCommandHandler", "loop interval is propagated to every runtime node");
  HandlerFixture fixture;
  ControllerCommand command{};
  command.type = ControllerCommandType::SET_LOOP_INTERVAL;
  command.integerValue = 42;

  ASSERT_TRUE(fixture.handler.handle(command));
  ASSERT_EQ(PoolController::ConfigManager::getSettings().loopInterval, 42);
  ASSERT_EQ(fixture.solarTemperature.getMeasurementInterval(), 42UL);
  ASSERT_EQ(fixture.poolTemperature.getMeasurementInterval(), 42UL);
  ASSERT_EQ(fixture.controllerTemperature.getMeasurementInterval(), 42UL);
  ASSERT_EQ(fixture.poolPump.getMeasurementInterval(), 42UL);
  ASSERT_EQ(fixture.solarPump.getMeasurementInterval(), 42UL);
  ASSERT_EQ(fixture.operation.getMeasurementInterval(), 42UL);
  return 0;
}

static int test_factory_reset_owns_runtime_reset_mutation() {
  test_begin("ControllerCommandHandler", "factory reset delegates configuration reset through the handler");
  HandlerFixture fixture;
  PoolController::ConfigManager::setConfigured(true);

  ControllerCommand command{};
  command.type = ControllerCommandType::FACTORY_RESET;
  ASSERT_TRUE(fixture.handler.handle(command));
  ASSERT_TRUE(!PoolController::ConfigManager::isConfigured());
  return 0;
}

int run_controller_command_handler_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_mode_and_setpoint_are_applied_through_one_handler, test_persistence_failure_rolls_back_setpoints,
    test_green_time_loss_rejects_zero, test_timer_updates_are_independent, test_manual_pump_guard_is_application_policy,
    test_sensor_mapping_is_persisted_and_applied, test_setting_missing_sensor_role_preserves_existing_mapping,
    test_loop_interval_updates_all_runtime_nodes, test_factory_reset_owns_runtime_reset_mutation};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("ControllerCommandHandler", passed, failed);
  return failed;
}
