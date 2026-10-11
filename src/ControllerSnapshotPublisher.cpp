// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "ControllerSnapshotPublisher.hpp"

#include <Arduino.h>
#include <array>
#include <cmath>
#include <cstring>

#include "ConfigManager.hpp"
#include "ControllerSnapshot.hpp"
#include "ControllerSnapshotStore.hpp"
#include "DallasTemperatureNode.hpp"
#include "DegradationManager.hpp"
#include "ESP32TemperatureNode.hpp"
#include "NetworkManager.hpp"
#include "OperationModeNode.hpp"
#include "RelayModuleNode.hpp"
#include "Rule.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"

namespace PoolController {

namespace {

bool isZeroAddress(const std::array<std::uint8_t, 8> &address) {
  for (const std::uint8_t value : address) {
    if (value != 0) {
      return false;
    }
  }
  return true;
}

SensorReadingSnapshot sensorReading(float value, bool found) {
  SensorReadingSnapshot reading{};
  reading.value = value;
  reading.valid = found && std::isfinite(value);
  return reading;
}

SensorReadingSnapshot controllerReading(float value) {
  SensorReadingSnapshot reading{};
  reading.value = value;
  reading.valid = std::isfinite(value);
  return reading;
}

SensorMappingSnapshot mappingSnapshot(const std::array<std::uint8_t, 8> &configuredAddress, const DallasTemperatureNode &node) {
  SensorMappingSnapshot mapping{};
  mapping.address = configuredAddress;
  mapping.configured = !isZeroAddress(configuredAddress);
  mapping.found = node.isSensorFound();

  if (mapping.configured && mapping.found) {
    const std::uint8_t *selectedAddress = node.getDeviceAddress();
    const bool addressMatches = std::memcmp(selectedAddress, configuredAddress.data(), configuredAddress.size()) == 0;
    mapping.found = selectedAddress != nullptr && addressMatches;
  }
  return mapping;
}

TimeDegradationState snapshotTimeDegradation(TimeDegradation degradation) {
  switch (degradation) {
  case TimeDegradation::GREEN:
    return TimeDegradationState::GREEN;
  case TimeDegradation::YELLOW:
    return TimeDegradationState::YELLOW;
  case TimeDegradation::RED:
  default:
    return TimeDegradationState::RED;
  }
}

}  // namespace

void ControllerSnapshotPublisher::publish() {
  SystemSnapshot snapshot{};
  const std::uint32_t nowMs = millis();

  const float poolTemperature = dependencies_.poolTemperature.getTemperature();
  const float solarTemperature = dependencies_.solarTemperature.getTemperature();
  const float controllerTemperature = dependencies_.controllerTemperature.getTemperature();

  snapshot.sensors.pool = sensorReading(poolTemperature, dependencies_.poolTemperature.isSensorFound());
  snapshot.sensors.solar = sensorReading(solarTemperature, dependencies_.solarTemperature.isSensorFound());
  snapshot.sensors.controller = controllerReading(controllerTemperature);

  std::array<std::uint8_t, 8> solarAddress{};
  std::array<std::uint8_t, 8> poolAddress{};
  ConfigManager::loadSensorMapping(solarAddress.data(), poolAddress.data());
  snapshot.sensors.solarMapping = mappingSnapshot(solarAddress, dependencies_.solarTemperature);
  snapshot.sensors.poolMapping = mappingSnapshot(poolAddress, dependencies_.poolTemperature);

  // Transitional generation: this identifies complete Core-1 read-model
  // publications. Once #170 provides a coherent Core-0 acquisition snapshot,
  // these two fields should use that sensor acquisition generation/timestamp.
  snapshot.sensors.measuredAtMs = nowMs;
  snapshot.sensors.generation = ++generation_;

  snapshot.network.wifiConnected = NetworkManager::isWiFiConnected();
  snapshot.network.mqttConnected = NetworkManager::isMqttConnected();
  snapshot.network.apMode = NetworkManager::isApMode();
  snapshot.network.wifiRssi = static_cast<std::int16_t>(NetworkManager::getWiFiRSSI());

  snapshot.health.safeMode = DegradationManager::isSafe();
  snapshot.health.timeValid = isTimeSyncValid();
  snapshot.health.timeDegradation = snapshotTimeDegradation(getTimeDegradation());
  snapshot.health.poolSensorValid = snapshot.sensors.pool.valid;
  snapshot.health.solarSensorValid = snapshot.sensors.solar.valid;

  const auto &settings = ConfigManager::getSettings();
  snapshot.circulation.threshold = settings.tempCircThreshold;
  snapshot.circulation.factorMinutesPerDegree = static_cast<std::uint16_t>(settings.tempCircFactor);
  snapshot.circulation.maxRuntimeMinutes = static_cast<std::uint16_t>(settings.tempCircMaxRuntime);

  const TimerSetting timer = dependencies_.operationMode.getTimerSetting();
  snapshot.timer.startHour = timer.timerStartHour;
  snapshot.timer.startMinute = timer.timerStartMinutes;
  snapshot.timer.endHour = timer.timerEndHour;
  snapshot.timer.endMinute = timer.timerEndMinutes;

  Rule *activeRule = dependencies_.operationMode.getRule();
  if (activeRule != nullptr) {
    snapshot.timer.effectiveRuntimeMinutes = activeRule->getEffectiveRuntimeMinutes();
    snapshot.timer.circulationExtensionMinutes = activeRule->getCirculationExtensionMinutes();
    snapshot.timer.activeEndMinutes = activeRule->getActiveEndMinutes();
  }

  snapshot.settings.loopInterval = static_cast<std::uint32_t>(settings.loopInterval);
  snapshot.settings.timezoneIndex = static_cast<std::int16_t>(settings.timezoneIndex);
  snapshot.settings.timeLossGreenHours = static_cast<std::uint16_t>(settings.timeLossGreenHours);
  snapshot.settings.timeLossRedHours = static_cast<std::uint16_t>(settings.timeLossRedHours);
  snapshot.settings.button1Min = settings.btn1Min;
  snapshot.settings.button1Max = settings.btn1Max;
  snapshot.settings.button2Min = settings.btn2Min;
  snapshot.settings.button2Max = settings.btn2Max;
  snapshot.settings.button3Min = settings.btn3Min;
  snapshot.settings.button3Max = settings.btn3Max;
  snapshot.settings.buttonNoPress = settings.btnNoPress;

  snapshot.mode = dependencies_.operationMode.getTypedMode();
  snapshot.poolPumpOn = dependencies_.poolPump.getSwitch();
  snapshot.solarPumpOn = dependencies_.solarPump.getSwitch();
  snapshot.poolMaxTemperature = dependencies_.operationMode.getPoolMaxTemperature();
  snapshot.solarMinTemperature = dependencies_.operationMode.getSolarMinTemperature();
  snapshot.temperatureHysteresis = dependencies_.operationMode.getTemperatureHysteresis();
  snapshot.uptimeMs = nowMs;
  snapshot.freeHeapBytes = SystemMonitor::getFreeHeap();

  dependencies_.store.publish(snapshot);
}

}  // namespace PoolController
