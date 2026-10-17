// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "ControllerCommandHandler.hpp"

#include <Preferences.h>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "ConfigManager.hpp"
#include "ControllerSettingLimits.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "LogCapture.hpp"
#include "OperationModeNode.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"

#ifdef NORVI_AE01_R
#include "NorviButtonHandler.hpp"
#endif

namespace PoolController {

namespace {

using Settings = std::remove_reference<decltype(ConfigManager::getSettings())>::type;
using NtpSettings = std::remove_reference<decltype(ConfigManager::getNtp())>::type;

template <typename Field, typename Value> CommandStatus stageSetting(Field &field, Value value, const SettingRange &range) {
  if (!range.contains(static_cast<double>(value))) {
    return CommandStatus::INVALID;
  }
  field = static_cast<Field>(value);
  return CommandStatus::APPLIED;
}

std::uint16_t *buttonThreshold(Settings &settings, ControllerCommandType type) {
  switch (type) {
  case ControllerCommandType::SET_BUTTON_1_MIN:
    return &settings.btn1Min;
  case ControllerCommandType::SET_BUTTON_1_MAX:
    return &settings.btn1Max;
  case ControllerCommandType::SET_BUTTON_2_MIN:
    return &settings.btn2Min;
  case ControllerCommandType::SET_BUTTON_2_MAX:
    return &settings.btn2Max;
  case ControllerCommandType::SET_BUTTON_3_MIN:
    return &settings.btn3Min;
  case ControllerCommandType::SET_BUTTON_3_MAX:
    return &settings.btn3Max;
  case ControllerCommandType::SET_BUTTON_NO_PRESS:
    return &settings.btnNoPress;
  default:
    return nullptr;
  }
}

bool isValidTime(const ControllerCommand &command) {
  return command.hour <= 23 && command.minute <= 59;
}

}  // namespace

/** @brief Staged state of one batch; becomes live only after a successful save. */
struct ControllerCommandHandler::Plan final {
  Plan(const Settings &liveSettings, const NtpSettings &liveNtp, OperationMode liveMode)
      : settings(liveSettings), ntp(liveNtp), mode(liveMode) {}

  Settings settings;
  NtpSettings ntp;
  OperationMode mode;
  const char *modeSource{nullptr};
  bool settingsChanged{false};
  bool modeChanged{false};
  bool timeLossChanged{false};
  bool mappingLoaded{false};
  bool mappingChanged{false};
  std::uint8_t solarAddress[8]{};
  std::uint8_t poolAddress[8]{};
};

const char *ControllerCommandHandler::sourceName(CommandSource source) {
  switch (source) {
  case CommandSource::MQTT:
    return "mqtt:typed-command";
  case CommandSource::WEB:
    return "web:typed-command";
  case CommandSource::LOCAL_UI:
    return "local-ui:typed-command";
  case CommandSource::INTERNAL:
  default:
    return "internal:typed-command";
  }
}

CommandStatus ControllerCommandHandler::execute(
  const ControllerCommand *commands, std::size_t count, ControllerCommandResult *result) {
  if (commands == nullptr || count == 0) {
    return CommandStatus::INVALID;
  }
  for (std::size_t i = 0; i < count; ++i) {
    if (commands[i].type == ControllerCommandType::FACTORY_RESET) {
      return count == 1 ? factoryReset() : CommandStatus::INVALID;
    }
  }

  Settings &liveSettings = ConfigManager::getSettings();
  NtpSettings &liveNtp = ConfigManager::getNtp();
  Plan plan{liveSettings, liveNtp, dependencies_.operationMode.getTypedMode()};

  for (std::size_t i = 0; i < count; ++i) {
    const CommandStatus status = stage(commands[i], plan);
    if (status != CommandStatus::APPLIED) {
      return status;
    }
  }
  if (plan.timeLossChanged && plan.settings.timeLossGreenHours >= plan.settings.timeLossRedHours) {
    LOG_WARN("Controller command: time-loss hours must satisfy 1 <= green < red <= 72\n");
    return CommandStatus::INVALID;
  }

  // The sensor mapping and the settings live in separate NVS records. A batch
  // that changes both could be left half stored by a power loss between the
  // two writes, so it is rejected; no client sends such a batch.
  if (plan.mappingChanged && plan.settingsChanged) {
    LOG_WARN("Controller command: sensor mapping and settings must be sent in separate batches\n");
    return CommandStatus::INVALID;
  }

  // Persist before touching the runtime, so a failed write changes nothing.
  if (plan.mappingChanged) {
    if (!ConfigManager::saveSensorMapping(plan.solarAddress, plan.poolAddress)) {
      LOG_ERROR("Controller command: failed to persist sensor mapping\n");
      return CommandStatus::PERSIST_FAILED;
    }
  }
  if (plan.settingsChanged) {
    const Settings previousSettings = liveSettings;
    const NtpSettings previousNtp = liveNtp;
    liveSettings = plan.settings;
    liveNtp = plan.ntp;
    if (!ConfigManager::save()) {
      LOG_ERROR("Controller command: failed to persist runtime settings\n");
      liveSettings = previousSettings;
      liveNtp = previousNtp;
      // A single failed put may have stored part of the batch; rewrite the
      // previous values so NVS matches the unchanged runtime again.
      if (!ConfigManager::save()) {
        LOG_ERROR("Controller command: could not restore previous settings in NVS\n");
      }
      return CommandStatus::PERSIST_FAILED;
    }
  }

  if (plan.modeChanged) {
    dependencies_.operationMode.setMode(plan.mode, plan.modeSource);
    if (result != nullptr) {
      result->hasMode = true;
      result->mode = plan.mode;
    }
  }
  for (std::size_t i = 0; i < count; ++i) {
    applyRuntime(commands[i], result);
  }
  for (std::size_t i = 0; i < count; ++i) {
    applySensorMapping(commands[i], plan);
  }
  return CommandStatus::APPLIED;
}

CommandStatus ControllerCommandHandler::stage(const ControllerCommand &command, Plan &plan) const {
  Settings &settings = plan.settings;

  switch (command.type) {
  case ControllerCommandType::SET_MODE:
  case ControllerCommandType::CYCLE_MODE:
    plan.mode = command.type == ControllerCommandType::SET_MODE ? command.mode : nextOperationMode(plan.mode);
    plan.modeSource = sourceName(command.source);
    plan.modeChanged = true;
    settings.opMode = toString(plan.mode);
    plan.settingsChanged = true;
    return CommandStatus::APPLIED;

  case ControllerCommandType::SET_POOL_MAX_TEMPERATURE:
    plan.settingsChanged = true;
    return stageSetting(settings.tempMaxPool, command.value, SettingLimits::kPoolMaxTemperature);
  case ControllerCommandType::SET_SOLAR_MIN_TEMPERATURE:
    plan.settingsChanged = true;
    return stageSetting(settings.tempMinSolar, command.value, SettingLimits::kSolarMinTemperature);
  case ControllerCommandType::SET_TEMPERATURE_HYSTERESIS:
    plan.settingsChanged = true;
    return stageSetting(settings.tempHysteresis, command.value, SettingLimits::kTemperatureHysteresis);
  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_THRESHOLD:
    plan.settingsChanged = true;
    return stageSetting(settings.tempCircThreshold, command.value, SettingLimits::kCirculationThreshold);
  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_FACTOR:
    plan.settingsChanged = true;
    return stageSetting(settings.tempCircFactor, command.integerValue, SettingLimits::kCirculationFactor);
  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME:
    plan.settingsChanged = true;
    return stageSetting(settings.tempCircMaxRuntime, command.integerValue, SettingLimits::kCirculationMaxRuntime);
  case ControllerCommandType::SET_LOOP_INTERVAL:
    plan.settingsChanged = true;
    return stageSetting(settings.loopInterval, command.integerValue, SettingLimits::kLoopInterval);

  case ControllerCommandType::SET_TIMEZONE:
    plan.settingsChanged = true;
    return stageSetting(settings.timezoneIndex, command.integerValue, SettingRange{0.0, getTzCount() - 1.0});

  // green < red is checked once the whole batch is staged, so one batch may move both bounds.
  case ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS:
    plan.settingsChanged = true;
    plan.timeLossChanged = true;
    return stageSetting(settings.timeLossGreenHours, command.integerValue, SettingLimits::kTimeLossGreenHours);
  case ControllerCommandType::SET_TIME_LOSS_RED_HOURS:
    plan.settingsChanged = true;
    plan.timeLossChanged = true;
    return stageSetting(settings.timeLossRedHours, command.integerValue, SettingLimits::kTimeLossRedHours);

  case ControllerCommandType::SET_BUTTON_1_MIN:
  case ControllerCommandType::SET_BUTTON_1_MAX:
  case ControllerCommandType::SET_BUTTON_2_MIN:
  case ControllerCommandType::SET_BUTTON_2_MAX:
  case ControllerCommandType::SET_BUTTON_3_MIN:
  case ControllerCommandType::SET_BUTTON_3_MAX:
  case ControllerCommandType::SET_BUTTON_NO_PRESS:
    plan.settingsChanged = true;
    return stageSetting(*buttonThreshold(settings, command.type), command.integerValue, SettingLimits::kButtonAdc);

  case ControllerCommandType::SET_NTP_SERVER:
    if (!command.ntpServer.valid()) {
      LOG_WARN("Controller command: rejecting invalid NTP server\n");
      return CommandStatus::INVALID;
    }
    plan.ntp.server = command.ntpServer.text.data();
    plan.settingsChanged = true;
    return CommandStatus::APPLIED;

  case ControllerCommandType::SET_TIMER_START:
  case ControllerCommandType::SET_TIMER_END:
    return isValidTime(command) ? CommandStatus::APPLIED : CommandStatus::INVALID;

  case ControllerCommandType::SET_SENSOR_MAPPING:
  case ControllerCommandType::CLEAR_SENSOR_MAPPING:
    return stageSensorMapping(command, plan);

  case ControllerCommandType::SET_POOL_PUMP_MANUAL:
  case ControllerCommandType::SET_SOLAR_PUMP_MANUAL:
  case ControllerCommandType::TOGGLE_POOL_PUMP:
  case ControllerCommandType::TOGGLE_SOLAR_PUMP:
    return stagePumpCommand(command, plan);

  case ControllerCommandType::FACTORY_RESET:
    // Handled before staging; it never shares a batch.
    return CommandStatus::INVALID;
  }
  return CommandStatus::INVALID;
}

CommandStatus ControllerCommandHandler::stageSensorMapping(const ControllerCommand &command, Plan &plan) const {
  if (!plan.mappingLoaded) {
    if (!ConfigManager::loadSensorMappingCached(plan.solarAddress, plan.poolAddress)) {
      LOG_ERROR("Controller command: failed to load sensor mapping before update\n");
      return CommandStatus::PERSIST_FAILED;
    }
    plan.mappingLoaded = true;
  }
  std::uint8_t *target = command.sensorRole == SensorRole::SOLAR ? plan.solarAddress : plan.poolAddress;
  if (command.type == ControllerCommandType::CLEAR_SENSOR_MAPPING) {
    std::memset(target, 0, command.sensorAddress.size());
  } else {
    std::memcpy(target, command.sensorAddress.data(), command.sensorAddress.size());
  }
  plan.mappingChanged = true;
  return CommandStatus::APPLIED;
}

CommandStatus ControllerCommandHandler::stagePumpCommand(const ControllerCommand &command, Plan &plan) const {
  const bool isToggle =
    command.type == ControllerCommandType::TOGGLE_POOL_PUMP || command.type == ControllerCommandType::TOGGLE_SOLAR_PUMP;
  const PumpToggleModePolicy policy = isToggle ? command.toggleModePolicy : PumpToggleModePolicy::REQUIRE_MANUAL;

  if (plan.mode == OperationMode::MANUAL || policy == PumpToggleModePolicy::KEEP_MODE) {
    return CommandStatus::APPLIED;
  }
  if (policy == PumpToggleModePolicy::REQUIRE_MANUAL) {
    LOG_WARN("Controller command: manual pump request rejected outside manual mode\n");
    return CommandStatus::REJECTED;
  }
  // ENTER_MANUAL: the mode switch is part of the same atomic batch.
  plan.mode = OperationMode::MANUAL;
  plan.modeSource = sourceName(command.source);
  plan.modeChanged = true;
  plan.settings.opMode = toString(OperationMode::MANUAL);
  plan.settingsChanged = true;
  return CommandStatus::APPLIED;
}

void ControllerCommandHandler::applyRuntime(const ControllerCommand &command, ControllerCommandResult *result) {
  OperationModeNode &operation = dependencies_.operationMode;

  switch (command.type) {
  case ControllerCommandType::SET_POOL_MAX_TEMPERATURE:
    operation.setPoolMaxTemperature(command.value);
    break;
  case ControllerCommandType::SET_SOLAR_MIN_TEMPERATURE:
    operation.setSolarMinTemperature(command.value);
    break;
  case ControllerCommandType::SET_TEMPERATURE_HYSTERESIS:
    operation.setTemperatureHysteresis(command.value);
    break;
  case ControllerCommandType::SET_LOOP_INTERVAL:
    applyLoopInterval(command.integerValue);
    break;
  case ControllerCommandType::SET_TIMEZONE:
    setTimezoneIndex(command.integerValue);
    break;
  case ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS:
    setTimeDegradationGreenHours(static_cast<std::uint8_t>(command.integerValue));
    break;
  case ControllerCommandType::SET_TIME_LOSS_RED_HOURS:
    setTimeDegradationRedHours(static_cast<std::uint8_t>(command.integerValue));
    break;
  case ControllerCommandType::SET_BUTTON_1_MIN:
  case ControllerCommandType::SET_BUTTON_1_MAX:
  case ControllerCommandType::SET_BUTTON_2_MIN:
  case ControllerCommandType::SET_BUTTON_2_MAX:
  case ControllerCommandType::SET_BUTTON_3_MIN:
  case ControllerCommandType::SET_BUTTON_3_MAX:
  case ControllerCommandType::SET_BUTTON_NO_PRESS:
#ifdef NORVI_AE01_R
    NorviButtonHandler::applySettings();
#endif
    break;
  case ControllerCommandType::SET_NTP_SERVER:
    timeClientSetup(command.ntpServer.text.data());
    break;
  case ControllerCommandType::SET_TIMER_START:
  case ControllerCommandType::SET_TIMER_END: {
    TimerSetting timer = operation.getTimerSetting();
    if (command.type == ControllerCommandType::SET_TIMER_START) {
      timer.timerStartHour = command.hour;
      timer.timerStartMinutes = command.minute;
    } else {
      timer.timerEndHour = command.hour;
      timer.timerEndMinutes = command.minute;
    }
    operation.setTimerSetting(timer);
    break;
  }
  case ControllerCommandType::SET_POOL_PUMP_MANUAL:
    dependencies_.poolPump.setSwitch(command.enabled);
    break;
  case ControllerCommandType::SET_SOLAR_PUMP_MANUAL:
    dependencies_.solarPump.setSwitch(command.enabled);
    break;
  case ControllerCommandType::TOGGLE_POOL_PUMP:
  case ControllerCommandType::TOGGLE_SOLAR_PUMP:
    togglePump(command, result);
    break;
  case ControllerCommandType::SET_MODE:
  case ControllerCommandType::CYCLE_MODE:
  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_THRESHOLD:
  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_FACTOR:
  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME:
  case ControllerCommandType::SET_SENSOR_MAPPING:
  case ControllerCommandType::CLEAR_SENSOR_MAPPING:
  case ControllerCommandType::FACTORY_RESET:
    // Mode is applied once per batch, circulation settings are read from
    // ConfigManager by the rules, and sensor mapping is applied afterwards.
    break;
  }
}

void ControllerCommandHandler::applyLoopInterval(long seconds) {
  dependencies_.solarTemperature.setMeasurementInterval(seconds);
  dependencies_.poolTemperature.setMeasurementInterval(seconds);
  dependencies_.controllerTemperature.setMeasurementInterval(seconds);
  dependencies_.poolPump.setMeasurementInterval(seconds);
  dependencies_.solarPump.setMeasurementInterval(seconds);
  dependencies_.operationMode.setMeasurementInterval(seconds);
}

void ControllerCommandHandler::applySensorMapping(const ControllerCommand &command, const Plan &plan) {
  if (command.type != ControllerCommandType::SET_SENSOR_MAPPING && command.type != ControllerCommandType::CLEAR_SENSOR_MAPPING) {
    return;
  }
  const bool solar = command.sensorRole == SensorRole::SOLAR;
  DallasTemperatureNode &node = solar ? dependencies_.solarTemperature : dependencies_.poolTemperature;
  if (command.type == ControllerCommandType::CLEAR_SENSOR_MAPPING) {
    node.clearAddressFilter();
  } else {
    node.setAddressFilter(solar ? plan.solarAddress : plan.poolAddress);
  }
}

void ControllerCommandHandler::togglePump(const ControllerCommand &command, ControllerCommandResult *result) {
  RelayModuleNode &pump =
    command.type == ControllerCommandType::TOGGLE_POOL_PUMP ? dependencies_.poolPump : dependencies_.solarPump;
  const bool newState = !pump.getSwitch();
  pump.setSwitch(newState);
  if (result != nullptr) {
    result->hasPumpState = true;
    result->pumpState = newState;
  }
}

CommandStatus ControllerCommandHandler::factoryReset() {
  // Persist the defaults first: if that fails, the relay/mode namespaces are
  // left alone so the device does not end up half reset.
  if (!ConfigManager::resetToDefaults()) {
    LOG_ERROR("Controller command: factory reset could not persist defaults\n");
    return CommandStatus::PERSIST_FAILED;
  }
  static const char *const kOperationalNamespaces[] = {"pool-pump", "solar-pump", "pool-controller"};
  Preferences preferences;
  for (const char *name : kOperationalNamespaces) {
    if (preferences.begin(name, false)) {
      preferences.clear();
      preferences.end();
    }
  }
  return CommandStatus::APPLIED;
}

}  // namespace PoolController
