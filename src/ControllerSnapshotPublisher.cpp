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

bool isValidDallasTemperature(float value) {
  return std::isfinite(value) && value != DEVICE_DISCONNECTED_C;
}

SensorReadingSnapshot sensorReading(float value, bool found) {
  SensorReadingSnapshot reading{};
  reading.value = value;
  reading.valid = found && isValidDallasTemperature(value);
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
    // The SensorTask rewrites the selected ROM on Core 0; compare a locked copy.
    DeviceAddress selectedAddress;
    node.copySelectedAddress(selectedAddress);
    mapping.found = std::memcmp(selectedAddress, configuredAddress.data(), configuredAddress.size()) == 0;
  }
  return mapping;
}

bool containsDetectedAddress(const SensorSnapshot &sensors, const std::uint8_t address[8]) {
  for (std::uint8_t i = 0; i < sensors.detectedCount; ++i) {
    if (std::memcmp(sensors.detected[i].address.data(), address, sensors.detected[i].address.size()) == 0) {
      return true;
    }
  }
  return false;
}

void appendDetectedSensors(SensorSnapshot &sensors, const DallasTemperatureNode &node) {
  const std::uint8_t deviceCount = node.getDeviceCount();
  for (std::uint8_t index = 0; index < deviceCount && sensors.detectedCount < kMaxDetectedSensors; ++index) {
    std::array<std::uint8_t, 8> address{};
    if (!node.getDetectedDeviceAddress(index, address.data()) || containsDetectedAddress(sensors, address.data())) {
      continue;
    }

    DetectedSensorSnapshot &detected = sensors.detected[sensors.detectedCount++];
    detected.address = address;
    const float temperature = node.getDetectedDeviceTemperature(index);
    detected.temperature.value = temperature;
    detected.temperature.valid = isValidDallasTemperature(temperature);
  }
}

Ipv4AddressSnapshot ipv4Snapshot(const IPAddress &address) {
  Ipv4AddressSnapshot snapshot{};
  for (std::size_t i = 0; i < snapshot.octets.size(); ++i) {
    snapshot.octets[i] = address[static_cast<int>(i)];
    snapshot.valid = snapshot.valid || snapshot.octets[i] != 0;
  }
  return snapshot;
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
  const auto &settings = ConfigManager::getSettings();

  const float poolTemperature = nodes_.poolTemperature.getTemperature();
  const float solarTemperature = nodes_.solarTemperature.getTemperature();
  const float controllerTemperature = nodes_.controllerTemperature.getTemperature();

  snapshot.sensors.pool = sensorReading(poolTemperature, nodes_.poolTemperature.isSensorFound());
  snapshot.sensors.solar = sensorReading(solarTemperature, nodes_.solarTemperature.isSensorFound());
  snapshot.sensors.controller = controllerReading(controllerTemperature);

  std::array<std::uint8_t, 8> solarAddress{};
  std::array<std::uint8_t, 8> poolAddress{};
  ConfigManager::loadSensorMappingCached(solarAddress.data(), poolAddress.data());
  snapshot.sensors.solarMapping = mappingSnapshot(solarAddress, nodes_.solarTemperature);
  snapshot.sensors.poolMapping = mappingSnapshot(poolAddress, nodes_.poolTemperature);
  const std::uint32_t detectedRefreshSeconds = settings.loopInterval > 0 ? static_cast<std::uint32_t>(settings.loopInterval) : 1U;
  const std::uint32_t detectedRefreshIntervalMs = detectedRefreshSeconds * 1000U;
  if (!detectedInventoryInitialized_ || nowMs - lastDetectedInventoryRefreshMs_ >= detectedRefreshIntervalMs) {
    SensorSnapshot refreshedInventory{};
    appendDetectedSensors(refreshedInventory, nodes_.solarTemperature);
    appendDetectedSensors(refreshedInventory, nodes_.poolTemperature);
    detectedInventory_ = refreshedInventory.detected;
    detectedInventoryCount_ = refreshedInventory.detectedCount;
    lastDetectedInventoryRefreshMs_ = nowMs;
    detectedInventoryInitialized_ = true;
  }
  snapshot.sensors.detected = detectedInventory_;
  snapshot.sensors.detectedCount = detectedInventoryCount_;

  // Transitional generation: this identifies complete Core-1 read-model
  // publications. Once #170 provides a coherent Core-0 acquisition snapshot,
  // these two fields should use that sensor acquisition generation/timestamp.
  snapshot.sensors.measuredAtMs = nowMs;
  snapshot.sensors.generation = ++generation_;

  snapshot.network.wifiConnected = NetworkManager::isWiFiConnected();
  snapshot.network.mqttConnected = NetworkManager::isMqttConnected();
  snapshot.network.apMode = NetworkManager::isApMode();
  snapshot.network.wifiRssi = static_cast<std::int16_t>(NetworkManager::getWiFiRSSI());
  snapshot.network.localIp = ipv4Snapshot(NetworkManager::getLocalIPAddress());

  snapshot.health.safeMode = DegradationManager::isSafe();
  snapshot.health.timeValid = isTimeSyncValid();
  snapshot.health.timeDegradation = snapshotTimeDegradation(getTimeDegradation());
  snapshot.health.poolSensorValid = snapshot.sensors.pool.valid;
  snapshot.health.solarSensorValid = snapshot.sensors.solar.valid;

  snapshot.circulation.threshold = settings.tempCircThreshold;
  snapshot.circulation.factorMinutesPerDegree = static_cast<std::uint16_t>(settings.tempCircFactor);
  snapshot.circulation.maxRuntimeMinutes = static_cast<std::uint16_t>(settings.tempCircMaxRuntime);

  const TimerSetting timer = nodes_.operationMode.getTimerSetting();
  snapshot.timer.startHour = timer.timerStartHour;
  snapshot.timer.startMinute = timer.timerStartMinutes;
  snapshot.timer.endHour = timer.timerEndHour;
  snapshot.timer.endMinute = timer.timerEndMinutes;

  Rule *activeRule = nodes_.operationMode.getRule(false);
  if (activeRule != nullptr) {
    snapshot.timer.effectiveRuntimeMinutes = activeRule->getEffectiveRuntimeMinutes();
    snapshot.timer.circulationExtensionMinutes = activeRule->getCirculationExtensionMinutes();
    snapshot.timer.activeEndMinutes = activeRule->getActiveEndMinutes();
  }

  const auto &ntp = ConfigManager::getNtp();
  const char *ntpServer = ntp.server.c_str();
  snapshot.settings.ntpServer.assign(ntpServer, std::strlen(ntpServer));
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

  snapshot.mode = nodes_.operationMode.getTypedMode();
  snapshot.poolPumpOn = nodes_.poolPump.getSwitch();
  snapshot.solarPumpOn = nodes_.solarPump.getSwitch();
  snapshot.poolMaxTemperature = nodes_.operationMode.getPoolMaxTemperature();
  snapshot.solarMinTemperature = nodes_.operationMode.getSolarMinTemperature();
  snapshot.temperatureHysteresis = nodes_.operationMode.getTemperatureHysteresis();
  snapshot.uptimeMs = nowMs;
  snapshot.freeHeapBytes = SystemMonitor::getFreeHeap();
  snapshot.maxAllocHeapBytes = static_cast<std::uint32_t>(ESP.getMaxAllocHeap());

  store_.publish(snapshot);
}

}  // namespace PoolController
