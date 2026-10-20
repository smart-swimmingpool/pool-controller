// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "ControllerCommandHandler.hpp"

#include <cstring>

#include "ConfigManager.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "OperationModeNode.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"
#include "test_assert.h"

extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

using PoolController::CommandSource;
using PoolController::CommandStatus;
using PoolController::ControllerCommand;
using PoolController::ControllerCommandHandler;
using PoolController::ControllerCommandType;
using PoolController::OperationMode;
using PoolController::SensorRole;

struct HandlerFixture {
  OperationModeNode operation{"operation-mode", "Operation Mode", 300};
  RelayModuleNode poolPump{"pool", "Pool", 1};
  RelayModuleNode solarPump{"solar", "Solar", 2};
  DallasTemperatureNode solarTemperature{PoolController::SensorId::SOLAR, "solar-temp", "Solar Temperature", 3};
  DallasTemperatureNode poolTemperature{PoolController::SensorId::POOL, "pool-temp", "Pool Temperature", 4};
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

static int test_sensor_mapping_read_failure_does_not_overwrite_existing_role() {
  test_begin("ControllerCommandHandler", "sensor mapping update rejects transient cache read failure");
  HandlerFixture fixture;

  const std::uint8_t empty[8] = {0};
  const std::uint8_t existingPool[8] = {0x28, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7};
  const std::uint8_t newSolar[8] = {0x28, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7};
  PoolController::ConfigManager::seedSensorMappingForTest(empty, false, existingPool, true);

  ControllerCommand command{};
  command.type = ControllerCommandType::SET_SENSOR_MAPPING;
  command.sensorRole = SensorRole::SOLAR;
  std::memcpy(command.sensorAddress.data(), newSolar, 8);

  PoolController::ConfigManager::_sensorMappingReadFails = true;
  const bool accepted = fixture.handler.handle(command);
  PoolController::ConfigManager::_sensorMappingReadFails = false;
  ASSERT_TRUE(!accepted);

  std::uint8_t savedSolar[8] = {0};
  std::uint8_t savedPool[8] = {0};
  ASSERT_TRUE(PoolController::ConfigManager::loadSensorMapping(savedSolar, savedPool));
  ASSERT_EQ(std::memcmp(savedSolar, empty, 8), 0);
  ASSERT_EQ(std::memcmp(savedPool, existingPool, 8), 0);
  return 0;
}

static int test_loop_interval_persistence_failure_keeps_runtime_unchanged() {
  test_begin("ControllerCommandHandler", "failed loop interval persistence leaves runtime intervals unchanged");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.loopInterval = 30;
  fixture.solarTemperature.setMeasurementInterval(30);
  fixture.poolTemperature.setMeasurementInterval(30);
  fixture.controllerTemperature.setMeasurementInterval(30);
  fixture.poolPump.setMeasurementInterval(30);
  fixture.solarPump.setMeasurementInterval(30);
  fixture.operation.setMeasurementInterval(30);

  ControllerCommand command{};
  command.type = ControllerCommandType::SET_LOOP_INTERVAL;
  command.integerValue = 42;
  PoolController::ConfigManager::_saveFails = true;
  const bool accepted = fixture.handler.handle(command);
  PoolController::ConfigManager::_saveFails = false;

  ASSERT_TRUE(!accepted);
  ASSERT_EQ(settings.loopInterval, 30);
  ASSERT_EQ(fixture.solarTemperature.getMeasurementInterval(), 30UL);
  ASSERT_EQ(fixture.poolTemperature.getMeasurementInterval(), 30UL);
  ASSERT_EQ(fixture.controllerTemperature.getMeasurementInterval(), 30UL);
  ASSERT_EQ(fixture.poolPump.getMeasurementInterval(), 30UL);
  ASSERT_EQ(fixture.solarPump.getMeasurementInterval(), 30UL);
  ASSERT_EQ(fixture.operation.getMeasurementInterval(), 30UL);
  return 0;
}

static int test_time_loss_thresholds_preserve_ordering() {
  test_begin("ControllerCommandHandler", "time-loss thresholds preserve 1 <= green < red <= 72");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.timeLossGreenHours = 2;
  settings.timeLossRedHours = 24;

  ControllerCommand command{};
  command.type = ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS;
  command.integerValue = 24;
  ASSERT_TRUE(!fixture.handler.handle(command));
  ASSERT_EQ(settings.timeLossGreenHours, 2);

  command.type = ControllerCommandType::SET_TIME_LOSS_RED_HOURS;
  command.integerValue = 2;
  ASSERT_TRUE(!fixture.handler.handle(command));
  ASSERT_EQ(settings.timeLossRedHours, 24);

  command.integerValue = 25;
  ASSERT_TRUE(fixture.handler.handle(command));
  ASSERT_EQ(settings.timeLossRedHours, 25);
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

namespace {
/** @brief Run one command with NVS writes failing, restoring the hook afterwards. */
CommandStatus executeWithFailingSave(ControllerCommandHandler &handler, const ControllerCommand *commands, std::size_t count) {
  PoolController::ConfigManager::_saveFails = true;
  const CommandStatus status = handler.execute(commands, count);
  PoolController::ConfigManager::_saveFails = false;
  return status;
}
}  // namespace

static int test_status_separates_invalid_rejected_and_persist_failures() {
  test_begin("ControllerCommandHandler", "status distinguishes invalid values, policy rejections and NVS failures");
  HandlerFixture fixture;
  ControllerCommand setpoint{};
  setpoint.type = ControllerCommandType::SET_SOLAR_MIN_TEMPERATURE;
  setpoint.value = 101.0F;
  ASSERT_EQ(fixture.handler.execute(setpoint), CommandStatus::INVALID);

  fixture.operation.setMode(OperationMode::AUTO);
  ControllerCommand pump{};
  pump.type = ControllerCommandType::SET_SOLAR_PUMP_MANUAL;
  pump.enabled = true;
  ASSERT_EQ(fixture.handler.execute(pump), CommandStatus::REJECTED);

  setpoint.value = 40.0F;
  ASSERT_EQ(executeWithFailingSave(fixture.handler, &setpoint, 1), CommandStatus::PERSIST_FAILED);
  ASSERT_EQ(fixture.handler.execute(setpoint), CommandStatus::APPLIED);
  return 0;
}

static int test_batch_is_all_or_nothing() {
  test_begin("ControllerCommandHandler", "one invalid command leaves the whole batch unapplied");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.opMode = "auto";
  settings.tempMaxPool = 28.0F;
  settings.loopInterval = 30;
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.operation.setPoolMaxTemperature(28.0F);

  ControllerCommand batch[3]{};
  batch[0].type = ControllerCommandType::SET_MODE;
  batch[0].mode = OperationMode::BOOST;
  batch[1].type = ControllerCommandType::SET_POOL_MAX_TEMPERATURE;
  batch[1].value = 30.0F;
  batch[2].type = ControllerCommandType::SET_LOOP_INTERVAL;
  batch[2].integerValue = 0;  // out of range
  ASSERT_EQ(fixture.handler.execute(batch, 3), CommandStatus::INVALID);
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::AUTO);
  ASSERT_EQ(fixture.operation.getPoolMaxTemperature(), 28.0F);
  ASSERT_EQ(settings.opMode.compare("auto"), 0);
  ASSERT_EQ(settings.tempMaxPool, 28.0F);
  ASSERT_EQ(settings.loopInterval, 30);

  batch[2].integerValue = 15;
  ASSERT_EQ(fixture.handler.execute(batch, 3), CommandStatus::APPLIED);
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::BOOST);
  ASSERT_EQ(settings.tempMaxPool, 30.0F);
  ASSERT_EQ(settings.loopInterval, 15);
  return 0;
}

static int test_persist_failure_rolls_back_every_setting_kind() {
  test_begin("ControllerCommandHandler", "failed persistence keeps timezone, buttons, NTP and mode unchanged");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  auto &ntp = PoolController::ConfigManager::getNtp();
  settings.timezoneIndex = 0;
  settings.btn2Min = 3520;
  settings.opMode = "auto";
  ntp.server = "pool.ntp.org";
  fixture.operation.setMode(OperationMode::AUTO);

  ControllerCommand batch[4]{};
  batch[0].type = ControllerCommandType::SET_TIMEZONE;
  batch[0].integerValue = 0;
  batch[1].type = ControllerCommandType::SET_BUTTON_2_MIN;
  batch[1].integerValue = 3000;
  batch[2].type = ControllerCommandType::SET_NTP_SERVER;
  ASSERT_TRUE(batch[2].ntpServer.assign("time.example.org", 16));
  batch[3].type = ControllerCommandType::CYCLE_MODE;
  ASSERT_EQ(executeWithFailingSave(fixture.handler, batch, 4), CommandStatus::PERSIST_FAILED);
  ASSERT_EQ(settings.btn2Min, 3520);
  ASSERT_EQ(ntp.server.compare("pool.ntp.org"), 0);
  ASSERT_EQ(settings.opMode.compare("auto"), 0);
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::AUTO);
  return 0;
}

static int test_enter_manual_toggle_is_atomic() {
  test_begin("ControllerCommandHandler", "Olimex toggle enters manual mode and toggles only if the mode was saved");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.opMode = "auto";
  fixture.operation.setMode(OperationMode::AUTO);
  fixture.poolPump.setSwitch(false);

  ControllerCommand toggle{};
  toggle.type = ControllerCommandType::TOGGLE_POOL_PUMP;
  toggle.toggleModePolicy = PoolController::PumpToggleModePolicy::ENTER_MANUAL;
  ASSERT_EQ(executeWithFailingSave(fixture.handler, &toggle, 1), CommandStatus::PERSIST_FAILED);
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::AUTO);
  ASSERT_FALSE(fixture.poolPump.getSwitch());

  PoolController::ControllerCommandResult result{};
  ASSERT_EQ(fixture.handler.execute(toggle, &result), CommandStatus::APPLIED);
  ASSERT_EQ(fixture.operation.getTypedMode(), OperationMode::MANUAL);
  ASSERT_EQ(settings.opMode.compare("manu"), 0);
  ASSERT_TRUE(fixture.poolPump.getSwitch());
  ASSERT_TRUE(result.hasPumpState && result.pumpState);
  return 0;
}

static int test_time_loss_bounds_can_move_together() {
  test_begin("ControllerCommandHandler", "one batch may raise green above the old red bound");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.timeLossGreenHours = 2;
  settings.timeLossRedHours = 24;

  ControllerCommand batch[2]{};
  batch[0].type = ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS;
  batch[0].integerValue = 30;
  batch[1].type = ControllerCommandType::SET_TIME_LOSS_RED_HOURS;
  batch[1].integerValue = 48;
  ASSERT_EQ(fixture.handler.execute(batch, 2), CommandStatus::APPLIED);
  ASSERT_EQ(settings.timeLossGreenHours, 30);
  ASSERT_EQ(settings.timeLossRedHours, 48);
  return 0;
}

static int test_time_loss_runtime_follows_validated_pair_in_any_order() {
  test_begin("ControllerCommandHandler", "RED listed before GREEN still leaves the runtime on the persisted pair");
  HandlerFixture fixture;
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.timeLossGreenHours = 30;
  settings.timeLossRedHours = 48;
  setTimeDegradationGreenHours(30);
  setTimeDegradationRedHours(48);

  ControllerCommand batch[2]{};
  batch[0].type = ControllerCommandType::SET_TIME_LOSS_RED_HOURS;
  batch[0].integerValue = 12;
  batch[1].type = ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS;
  batch[1].integerValue = 10;
  ASSERT_EQ(fixture.handler.execute(batch, 2), CommandStatus::APPLIED);
  ASSERT_EQ(settings.timeLossGreenHours, 10);
  ASSERT_EQ(settings.timeLossRedHours, 12);
  ASSERT_EQ(getTimeDegradationGreenHours(), 10);
  ASSERT_EQ(getTimeDegradationRedHours(), 12);
  return 0;
}

static int test_timer_persist_failure_is_reported_and_keeps_runtime() {
  test_begin("ControllerCommandHandler", "a failed timer write reports PERSIST_FAILED and keeps the previous timer");
  HandlerFixture fixture;
  TimerSetting before = fixture.operation.getTimerSetting();

  ControllerCommand command{};
  command.type = ControllerCommandType::SET_TIMER_START;
  command.hour = (before.timerStartHour + 1) % 24;
  command.minute = 5;
  Preferences::s_failingPutKeys.insert("timerStartH");
  const CommandStatus failed = fixture.handler.execute(command);
  Preferences::s_failingPutKeys.clear();
  ASSERT_EQ(failed, CommandStatus::PERSIST_FAILED);
  ASSERT_EQ(fixture.operation.getTimerSetting().timerStartHour, before.timerStartHour);
  ASSERT_EQ(fixture.operation.getTimerSetting().timerStartMinutes, before.timerStartMinutes);

  ASSERT_EQ(fixture.handler.execute(command), CommandStatus::APPLIED);
  ASSERT_EQ(fixture.operation.getTimerSetting().timerStartHour, command.hour);
  ASSERT_EQ(fixture.operation.getTimerSetting().timerStartMinutes, 5);
  return 0;
}

static int test_factory_reset_keeps_relay_state_when_save_fails() {
  test_begin("ControllerCommandHandler", "factory reset clears relay/mode NVS only after the defaults were saved");
  HandlerFixture fixture;
  Preferences relayState;
  relayState.begin("pool-pump", false);
  relayState.putString("state", "on");
  relayState.end();

  ControllerCommand reset{};
  reset.type = ControllerCommandType::FACTORY_RESET;
  ASSERT_EQ(executeWithFailingSave(fixture.handler, &reset, 1), CommandStatus::PERSIST_FAILED);
  relayState.begin("pool-pump", true);
  const bool relayStateKept = relayState.getString("state") == "on";
  ASSERT_TRUE(relayStateKept);
  relayState.end();

  ControllerCommand batch[2]{};
  batch[0].type = ControllerCommandType::FACTORY_RESET;
  batch[1].type = ControllerCommandType::SET_LOOP_INTERVAL;
  batch[1].integerValue = 10;
  ASSERT_EQ(fixture.handler.execute(batch, 2), CommandStatus::INVALID);

  ASSERT_EQ(fixture.handler.execute(reset), CommandStatus::APPLIED);
  relayState.begin("pool-pump", true);
  ASSERT_EQ(relayState.getString("state").length(), 0U);
  relayState.end();
  return 0;
}

static int test_sensor_mapping_persist_failure_leaves_runtime_unchanged() {
  test_begin("ControllerCommandHandler", "failed mapping write is reported and not applied");
  HandlerFixture fixture;
  const std::uint8_t empty[8] = {0};
  PoolController::ConfigManager::seedSensorMappingForTest(empty, false, empty, false);

  ControllerCommand command{};
  command.type = ControllerCommandType::SET_SENSOR_MAPPING;
  command.sensorRole = SensorRole::POOL;
  const std::uint8_t address[8] = {0x28, 0xAA, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  std::memcpy(command.sensorAddress.data(), address, sizeof(address));

  PoolController::ConfigManager::_sensorMappingSaveFails = true;
  const CommandStatus status = fixture.handler.execute(command);
  PoolController::ConfigManager::_sensorMappingSaveFails = false;
  ASSERT_EQ(status, CommandStatus::PERSIST_FAILED);
  ASSERT_TRUE(!fixture.poolTemperature.hasAddressFilter());
  std::uint8_t savedSolar[8] = {0};
  std::uint8_t savedPool[8] = {0};
  ASSERT_TRUE(!PoolController::ConfigManager::loadSensorMapping(savedSolar, savedPool));
  return 0;
}

static int test_mixed_mapping_and_settings_batch_is_rejected() {
  test_begin("ControllerCommandHandler", "a batch changing mapping and settings is rejected unchanged");
  HandlerFixture fixture;
  const std::uint8_t oldSolar[8] = {0x28, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
  const std::uint8_t oldPool[8] = {0x28, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
  PoolController::ConfigManager::seedSensorMappingForTest(oldSolar, true, oldPool, true);
  auto &settings = PoolController::ConfigManager::getSettings();
  settings.btn1Min = 1200;

  ControllerCommand batch[2]{};
  batch[0].type = ControllerCommandType::SET_SENSOR_MAPPING;
  batch[0].sensorRole = SensorRole::SOLAR;
  const std::uint8_t newSolar[8] = {0x28, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09};
  std::memcpy(batch[0].sensorAddress.data(), newSolar, sizeof(newSolar));
  batch[1].type = ControllerCommandType::SET_BUTTON_1_MIN;
  batch[1].integerValue = 1000;

  ASSERT_EQ(fixture.handler.execute(batch, 2), CommandStatus::INVALID);
  std::uint8_t savedSolar[8] = {0};
  std::uint8_t savedPool[8] = {0};
  ASSERT_TRUE(PoolController::ConfigManager::loadSensorMapping(savedSolar, savedPool));
  ASSERT_EQ(std::memcmp(savedSolar, oldSolar, sizeof(oldSolar)), 0);
  ASSERT_EQ(std::memcmp(savedPool, oldPool, sizeof(oldPool)), 0);
  ASSERT_EQ(settings.btn1Min, 1200);
  ASSERT_TRUE(!fixture.solarTemperature.hasAddressFilter());
  return 0;
}

static int test_factory_reset_failure_keeps_runtime_configuration() {
  test_begin("ControllerCommandHandler", "failed factory reset keeps the running configuration");
  HandlerFixture fixture;
  PoolController::ConfigManager::setConfigured(true);
  ControllerCommand reset{};
  reset.type = ControllerCommandType::FACTORY_RESET;
  ASSERT_EQ(executeWithFailingSave(fixture.handler, &reset, 1), CommandStatus::PERSIST_FAILED);
  ASSERT_TRUE(PoolController::ConfigManager::isConfigured());
  return 0;
}

int run_controller_command_handler_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_mode_and_setpoint_are_applied_through_one_handler, test_persistence_failure_rolls_back_setpoints,
    test_green_time_loss_rejects_zero, test_timer_updates_are_independent, test_manual_pump_guard_is_application_policy,
    test_sensor_mapping_is_persisted_and_applied, test_setting_missing_sensor_role_preserves_existing_mapping,
    test_sensor_mapping_read_failure_does_not_overwrite_existing_role, test_loop_interval_updates_all_runtime_nodes,
    test_loop_interval_persistence_failure_keeps_runtime_unchanged, test_time_loss_thresholds_preserve_ordering,
    test_factory_reset_owns_runtime_reset_mutation, test_status_separates_invalid_rejected_and_persist_failures,
    test_batch_is_all_or_nothing, test_persist_failure_rolls_back_every_setting_kind, test_enter_manual_toggle_is_atomic,
    test_time_loss_bounds_can_move_together, test_time_loss_runtime_follows_validated_pair_in_any_order,
    test_timer_persist_failure_is_reported_and_keeps_runtime, test_factory_reset_keeps_relay_state_when_save_fails,
    test_sensor_mapping_persist_failure_leaves_runtime_unchanged, test_mixed_mapping_and_settings_batch_is_rejected,
    test_factory_reset_failure_keeps_runtime_configuration};
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
