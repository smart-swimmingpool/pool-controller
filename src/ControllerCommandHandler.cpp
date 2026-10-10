// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "ControllerCommandHandler.hpp"

#include <Preferences.h>
#include <cstdint>

#include "ConfigManager.hpp"
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

bool inRange(std::int32_t value, std::int32_t minimum, std::int32_t maximum) {
  return value >= minimum && value <= maximum;
}

bool inRange(float value, float minimum, float maximum) {
  return value >= minimum && value <= maximum;
}

OperationMode nextOperationMode(OperationMode current) {
  switch (current) {
  case OperationMode::AUTO:
    return OperationMode::MANUAL;
  case OperationMode::MANUAL:
    return OperationMode::BOOST;
  case OperationMode::BOOST:
    return OperationMode::TIMER;
  case OperationMode::TIMER:
    return OperationMode::AUTO;
  }
  return OperationMode::AUTO;
}

}  // namespace

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

bool ControllerCommandHandler::saveSettings() const {
  if (ConfigManager::save()) {
    return true;
  }
  LOG_ERROR("Controller command: failed to persist runtime settings\n");
  return false;
}

bool ControllerCommandHandler::applySensorMapping(const ControllerCommand &command, bool clear) {
  std::uint8_t solarAddress[8] = {0};
  std::uint8_t poolAddress[8] = {0};
  ConfigManager::loadSensorMapping(solarAddress, poolAddress);

  std::uint8_t *target = command.sensorRole == SensorRole::SOLAR ? solarAddress : poolAddress;
  for (std::size_t i = 0; i < command.sensorAddress.size(); ++i) {
    target[i] = clear ? 0 : command.sensorAddress[i];
  }

  ConfigManager::saveSensorMapping(solarAddress, poolAddress);

  DallasTemperatureNode &node =
    command.sensorRole == SensorRole::SOLAR ? dependencies_.solarTemperature : dependencies_.poolTemperature;
  if (clear) {
    node.clearAddressFilter();
  } else {
    node.setAddressFilter(target);
  }
  return true;
}

bool ControllerCommandHandler::handle(const ControllerCommand &command, ControllerCommandResult *result) {
  auto &settings = ConfigManager::getSettings();

  switch (command.type) {
  case ControllerCommandType::SET_MODE:
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
  }

  case ControllerCommandType::SET_POOL_MAX_TEMPERATURE:
    if (!inRange(command.value, 0.0F, 40.0F)) {
      return false;
    }
    dependencies_.operationMode.setPoolMaxTemperature(command.value);
    settings.tempMaxPool = command.value;
    return saveSettings();

  case ControllerCommandType::SET_SOLAR_MIN_TEMPERATURE:
    if (!inRange(command.value, 0.0F, 100.0F)) {
      return false;
    }
    dependencies_.operationMode.setSolarMinTemperature(command.value);
    settings.tempMinSolar = command.value;
    return saveSettings();

  case ControllerCommandType::SET_TEMPERATURE_HYSTERESIS:
    if (!inRange(command.value, 0.0F, 10.0F)) {
      return false;
    }
    dependencies_.operationMode.setTemperatureHysteresis(command.value);
    settings.tempHysteresis = command.value;
    return saveSettings();

  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_THRESHOLD:
    if (!inRange(command.value, 0.0F, 40.0F)) {
      return false;
    }
    settings.tempCircThreshold = command.value;
    return saveSettings();

  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_FACTOR:
    if (!inRange(command.integerValue, 0, 120)) {
      return false;
    }
    settings.tempCircFactor = static_cast<std::uint16_t>(command.integerValue);
    return saveSettings();

  case ControllerCommandType::SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME:
    if (!inRange(command.integerValue, 60, 1440)) {
      return false;
    }
    settings.tempCircMaxRuntime = static_cast<std::uint16_t>(command.integerValue);
    return saveSettings();

  case ControllerCommandType::SET_TIMER_START: {
    if (command.hour > 23 || command.minute > 59) {
      return false;
    }
    TimerSetting timer = dependencies_.operationMode.getTimerSetting();
    timer.timerStartHour = command.hour;
    timer.timerStartMinutes = command.minute;
    dependencies_.operationMode.setTimerSetting(timer);
    return true;
  }

  case ControllerCommandType::SET_TIMER_END: {
    if (command.hour > 23 || command.minute > 59) {
      return false;
    }
    TimerSetting timer = dependencies_.operationMode.getTimerSetting();
    timer.timerEndHour = command.hour;
    timer.timerEndMinutes = command.minute;
    dependencies_.operationMode.setTimerSetting(timer);
    return true;
  }

  case ControllerCommandType::SET_LOOP_INTERVAL:
    if (!inRange(command.integerValue, 1, 3600)) {
      return false;
    }
    settings.loopInterval = command.integerValue;
    dependencies_.solarTemperature.setMeasurementInterval(command.integerValue);
    dependencies_.poolTemperature.setMeasurementInterval(command.integerValue);
    dependencies_.controllerTemperature.setMeasurementInterval(command.integerValue);
    dependencies_.poolPump.setMeasurementInterval(command.integerValue);
    dependencies_.solarPump.setMeasurementInterval(command.integerValue);
    dependencies_.operationMode.setMeasurementInterval(command.integerValue);
    return saveSettings();

  case ControllerCommandType::SET_TIMEZONE:
    if (!inRange(command.integerValue, 0, getTzCount() - 1)) {
      return false;
    }
    settings.timezoneIndex = command.integerValue;
    if (!saveSettings()) {
      return false;
    }
    setTimezoneIndex(command.integerValue);
    return true;

  case ControllerCommandType::SET_TIME_LOSS_GREEN_HOURS:
    if (!inRange(command.integerValue, 0, 255)) {
      return false;
    }
    settings.timeLossGreenHours = command.integerValue;
    if (!saveSettings()) {
      return false;
    }
    setTimeDegradationGreenHours(static_cast<std::uint8_t>(command.integerValue));
    return true;

  case ControllerCommandType::SET_TIME_LOSS_RED_HOURS:
    if (!inRange(command.integerValue, 1, 255)) {
      return false;
    }
    settings.timeLossRedHours = command.integerValue;
    if (!saveSettings()) {
      return false;
    }
    setTimeDegradationRedHours(static_cast<std::uint8_t>(command.integerValue));
    return true;

  case ControllerCommandType::SET_BUTTON_1_MIN:
  case ControllerCommandType::SET_BUTTON_1_MAX:
  case ControllerCommandType::SET_BUTTON_2_MIN:
  case ControllerCommandType::SET_BUTTON_2_MAX:
  case ControllerCommandType::SET_BUTTON_3_MIN:
  case ControllerCommandType::SET_BUTTON_3_MAX:
  case ControllerCommandType::SET_BUTTON_NO_PRESS: {
    if (!inRange(command.integerValue, 0, 4096)) {
      return false;
    }
    const std::uint16_t value = static_cast<std::uint16_t>(command.integerValue);
    switch (command.type) {
    case ControllerCommandType::SET_BUTTON_1_MIN:
      settings.btn1Min = value;
      break;
    case ControllerCommandType::SET_BUTTON_1_MAX:
      settings.btn1Max = value;
      break;
    case ControllerCommandType::SET_BUTTON_2_MIN:
      settings.btn2Min = value;
      break;
    case ControllerCommandType::SET_BUTTON_2_MAX:
      settings.btn2Max = value;
      break;
    case ControllerCommandType::SET_BUTTON_3_MIN:
      settings.btn3Min = value;
      break;
    case ControllerCommandType::SET_BUTTON_3_MAX:
      settings.btn3Max = value;
      break;
    case ControllerCommandType::SET_BUTTON_NO_PRESS:
      settings.btnNoPress = value;
      break;
    default:
      break;
    }
    if (!saveSettings()) {
      return false;
    }
#ifdef NORVI_AE01_R
    NorviButtonHandler::applySettings();
#endif
    return true;
  }

  case ControllerCommandType::SET_SENSOR_MAPPING:
    return applySensorMapping(command, false);

  case ControllerCommandType::CLEAR_SENSOR_MAPPING:
    return applySensorMapping(command, true);

  case ControllerCommandType::SET_POOL_PUMP_MANUAL:
  case ControllerCommandType::SET_SOLAR_PUMP_MANUAL:
    if (dependencies_.operationMode.getTypedMode() != OperationMode::MANUAL) {
      LOG_WARN("Controller command: manual pump request rejected outside manual mode\n");
      return false;
    }
    if (command.type == ControllerCommandType::SET_POOL_PUMP_MANUAL) {
      dependencies_.poolPump.setSwitch(command.enabled);
    } else {
      dependencies_.solarPump.setSwitch(command.enabled);
    }
    return true;

  case ControllerCommandType::TOGGLE_POOL_PUMP:
  case ControllerCommandType::TOGGLE_SOLAR_PUMP: {
    const OperationMode currentMode = dependencies_.operationMode.getTypedMode();
    if (command.toggleModePolicy == PumpToggleModePolicy::REQUIRE_MANUAL && currentMode != OperationMode::MANUAL) {
      LOG_WARN("Controller command: Web pump toggle rejected outside manual mode\n");
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

  case ControllerCommandType::FACTORY_RESET: {
    static const char *const kOperationalNamespaces[] = {"pool-pump", "solar-pump", "pool-controller"};
    Preferences preferences;
    for (const char *name : kOperationalNamespaces) {
      if (preferences.begin(name, false)) {
        preferences.clear();
        preferences.end();
      }
    }
    ConfigManager::reset();
    return saveSettings();
  }
  }

  return false;
}

}  // namespace PoolController
