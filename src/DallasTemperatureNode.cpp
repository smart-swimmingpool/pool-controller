// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DallasTemperatureNode.cpp
 * @brief DS18B20 sensor node implementation with bus scanning, recovery, and
 *        cross-core discovery snapshots.
 */

#include "DallasTemperatureNode.hpp"
#include "DallasAddressLookup.hpp"
#include "Config.hpp"
#include "SystemMonitor.hpp"
#include "DegradationManager.hpp"
#include "SensorSlots.hpp"
#include "LogCapture.hpp"

DallasTemperatureNode::DallasTemperatureNode(
  PoolController::SensorId slot, const char *id, const char *name, const uint8_t pin, const int measurementInterval)
    : slot_(slot) {
  _id = id;
  _name = name;
  _pin = pin;
  _measurementInterval.store(
    (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL, std::memory_order_relaxed);
  _temperature = NAN;
  _sensorFound = false;

  oneWire.begin(_pin);
  sensor.setOneWire(&oneWire);
}

DallasTemperatureNode::DallasTemperatureNode(PoolController::SensorId slot, const char *id, const char *name,
  DallasTemperature *sharedSensor, uint8_t deviceIndex, const int measurementInterval)
    : slot_(slot) {
  _id = id;
  _name = name;
  _pin = 0;
  _measurementInterval.store(
    (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL, std::memory_order_relaxed);
  _temperature = NAN;
  _sensorFound = false;

  sharedSensor_ = sharedSensor;
  deviceIndex_ = deviceIndex;
  isBusMaster_ = (deviceIndex == 0);
  memset(deviceAddress_, 0, sizeof(deviceAddress_));
}

void DallasTemperatureNode::lockCache() const {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  portENTER_CRITICAL(&cacheMux_);
#else
  while (cacheLock_.test_and_set(std::memory_order_acquire)) {
  }
#endif
}

void DallasTemperatureNode::unlockCache() const {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  portEXIT_CRITICAL(&cacheMux_);
#else
  cacheLock_.clear(std::memory_order_release);
#endif
}

void DallasTemperatureNode::setAddressFilter(const DeviceAddress addr) {
  lockCache();
  memcpy(filterAddr_, addr, sizeof(DeviceAddress));
  unlockCache();
  hasFilter_.store(true, std::memory_order_release);
  mappingDirty_.store(true, std::memory_order_release);
}

void DallasTemperatureNode::clearAddressFilter() {
  lockCache();
  memset(filterAddr_, 0, sizeof(DeviceAddress));
  unlockCache();
  hasFilter_.store(false, std::memory_order_release);
  mappingDirty_.store(true, std::memory_order_release);
}

void DallasTemperatureNode::publishSelectedAddress() {
  lockCache();
  memcpy(cachedSelectedAddress_, deviceAddress_, sizeof(DeviceAddress));
  unlockCache();
}

bool DallasTemperatureNode::resolveFilter() {
  DallasTemperature *activeSensor = bus();
  DeviceAddress filter{};
  const bool hasFilter = hasFilter_.load(std::memory_order_acquire);

  if (hasFilter) {
    lockCache();
    memcpy(filter, filterAddr_, sizeof(DeviceAddress));
    unlockCache();
  }

  if (hasFilter && numberOfDevices > 0) {
    uint8_t index = 0;
    if (PoolController::findDallasDeviceByAddress(*activeSensor, numberOfDevices, filter, index)) {
      memcpy(deviceAddress_, filter, sizeof(DeviceAddress));
      publishSelectedAddress();
      _sensorFound = true;
      char adr[18];
      address2String(filter, adr, sizeof(adr));
      LOG_INFO("  ◦ %s: filter resolved → device %d [%s] ✓\n", _id, index, adr);
      return true;
    }

    LOG_ERROR("  ✖ %s: filter address not found on bus! Falling back to device index %d\n", _id, deviceIndex_);
    if (activeSensor->getAddress(deviceAddress_, deviceIndex_)) {
      publishSelectedAddress();
      _sensorFound = true;
      return true;
    }

    memset(deviceAddress_, 0, sizeof(deviceAddress_));
    publishSelectedAddress();
    _sensorFound = false;
    return false;
  }

  if (numberOfDevices > 0 && activeSensor->getAddress(deviceAddress_, deviceIndex_)) {
    publishSelectedAddress();
    _sensorFound = true;
    char adr[18];
    address2String(deviceAddress_, adr, sizeof(adr));
    if (sharedSensor_) {
      LOG_INFO("  ◦ %s: no filter → device %d [%s] (shared bus)\n", _id, deviceIndex_, adr);
    } else {
      LOG_INFO("  ◦ %s: no filter → device %d [%s]\n", _id, deviceIndex_, adr);
    }
    return true;
  }

  memset(deviceAddress_, 0, sizeof(deviceAddress_));
  publishSelectedAddress();
  _sensorFound = false;
  return false;
}

void DallasTemperatureNode::applyPendingAddressFilter() {
  if (!mappingDirty_.exchange(false, std::memory_order_acq_rel)) {
    return;
  }
  if (numberOfDevices > 0) {
    resolveFilter();
  }
}

void DallasTemperatureNode::refreshDetectedSnapshot(bool includeTemperatures) {
  DallasTemperature *activeSensor = bus();
  DeviceAddress localAddresses[MAX_DETECTED_DEVICES]{};
  float localTemperatures[MAX_DETECTED_DEVICES];
  for (float &value : localTemperatures) {
    value = NAN;
  }

  const uint8_t count = (numberOfDevices < MAX_DETECTED_DEVICES) ? numberOfDevices : MAX_DETECTED_DEVICES;
  uint8_t written = 0;
  for (uint8_t i = 0; i < count; i++) {
    DeviceAddress addr;
    if (!activeSensor->getAddress(addr, i)) {
      continue;
    }
    memcpy(localAddresses[written], addr, sizeof(DeviceAddress));
    if (includeTemperatures) {
      const float value = activeSensor->getTempC(addr);
      localTemperatures[written] = (value == DEVICE_DISCONNECTED_C) ? NAN : value;
    }
    written++;
  }

  lockCache();
  detectedCount_ = written;
  for (uint8_t i = 0; i < MAX_DETECTED_DEVICES; i++) {
    if (i < written) {
      memcpy(detectedAddresses_[i], localAddresses[i], sizeof(DeviceAddress));
      detectedTemperatures_[i] = localTemperatures[i];
    } else {
      memset(detectedAddresses_[i], 0, sizeof(DeviceAddress));
      detectedTemperatures_[i] = NAN;
    }
  }
  unlockCache();
}

uint8_t DallasTemperatureNode::getDeviceCount() const {
  lockCache();
  const uint8_t count = detectedCount_;
  unlockCache();
  return count;
}

void DallasTemperatureNode::getDeviceAddressString(char *buffer, size_t size) const {
  DeviceAddress addr;
  lockCache();
  memcpy(addr, cachedSelectedAddress_, sizeof(DeviceAddress));
  unlockCache();
  address2String(addr, buffer, size);
}

bool DallasTemperatureNode::getDetectedDeviceAddress(uint8_t index, DeviceAddress addr) const {
  lockCache();
  if (index >= detectedCount_) {
    unlockCache();
    return false;
  }
  memcpy(addr, detectedAddresses_[index], sizeof(DeviceAddress));
  unlockCache();
  return true;
}

float DallasTemperatureNode::getDetectedDeviceTemperature(uint8_t index) const {
  lockCache();
  if (index >= detectedCount_) {
    unlockCache();
    return NAN;
  }
  const float value = detectedTemperatures_[index];
  unlockCache();
  return value;
}

void DallasTemperatureNode::begin() {
  DallasTemperature *activeSensor = bus();

  if (!sharedSensor_) {
    activeSensor->begin();
  }

  // SensorTask owns the conversion delay. Without this, requestTemperatures()
  // blocks internally and the task would wait a second time afterwards.
  activeSensor->setWaitForConversion(false);

  numberOfDevices = activeSensor->getDeviceCount();
  LOG_INFO("• DallasTemperature: Parasite power is: %d\n", activeSensor->isParasitePowerMode());
  const uint8_t displayPin = sharedSensor_ ? PoolController::PIN_DS_SOLAR : _pin;

  if (numberOfDevices > 0) {
    LOG_INFO("  ◦ %d devices found on PIN %d\n", numberOfDevices, displayPin);

    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress addr;
      if (activeSensor->getAddress(addr, i)) {
        char adr[18];
        address2String(addr, adr, sizeof(adr));
        LOG_INFO("  ◦ PIN %d: Device %d address: %s\n", displayPin, i, adr);
      }
    }

    resolveFilter();
    mappingDirty_.store(false, std::memory_order_release);
    refreshDetectedSnapshot(false);

    if (sharedSensor_) {
      PoolController::DegradationManager::reportSensorStatus("solar-temp", numberOfDevices > 0);
      PoolController::DegradationManager::reportSensorStatus("pool-temp", numberOfDevices > 1);
    } else {
      PoolController::DegradationManager::reportSensorStatus(_id, true);
    }
  } else {
    LOG_ERROR("✖ No Dallas sensors found on pin %d\n", displayPin);
    _sensorFound = false;
    refreshDetectedSnapshot(false);
    PoolController::DegradationManager::reportSensorStatus(_id, false);
  }

  // Start in recovery cadence until the first valid measurement is published.
  PoolController::SensorSlots::write(slotId(), NAN, false);
}

void DallasTemperatureNode::beginMeasurement() {
  DallasTemperature *activeSensor = bus();
  applyPendingAddressFilter();

  if (sharedSensor_ && numberOfDevices > 0) {
    if (isBusMaster_) {
      PoolController::SystemMonitor::feedWatchdog();
      activeSensor->requestTemperatures();
      PoolController::SystemMonitor::feedWatchdog();
    }
  } else if (numberOfDevices > 0) {
    PoolController::SystemMonitor::feedWatchdog();
    activeSensor->requestTemperatures();
    PoolController::SystemMonitor::feedWatchdog();
  }
}

void DallasTemperatureNode::finishMeasurement() {
  if (numberOfDevices == 0) {
    LOG_INFO("No Sensor found on bus! Rescanning (%s)...\n", _id);
    PoolController::DegradationManager::reportSensorStatus(_id, false);
    PoolController::SensorSlots::write(slotId(), NAN, false);
    rescanBus();
    refreshDetectedSnapshot(false);
    return;
  }

  // Both bus modes read the one device resolved for this node, so the filter
  // and failure counter always track a single sensor.
  DallasTemperature *activeSensor = bus();
  const bool needsRescan = handleReading(activeSensor->getTempC(deviceAddress_));
  PoolController::SensorSlots::write(slotId(), _temperature, _sensorFound);
  if (needsRescan) {
    LOG_WARN("  ⚠ %s: %u failed readings — rescanning bus\n", _id, static_cast<unsigned>(RESCAN_AFTER_FAILURES));
    rescanBus();
  }
  refreshDetectedSnapshot(!needsRescan);
}

bool DallasTemperatureNode::handleReading(float raw) {
  using Action = PoolController::TemperatureReadingFilter::Action;

  switch (filter_.apply(raw)) {
  case Action::ACCEPT:
    _temperature = raw;
    _sensorFound = true;
    consecutiveFailures_ = 0;
    PoolController::DegradationManager::reportSensorStatus(_id, true);
    LOG_DEBUG("  ◦ %s = %.1f°C\n", _id, _temperature);
    return false;

  case Action::HOLD:
    // DS18B20 power-on value after a supply glitch — keep the last temperature
    LOG_WARN("  ⚠ %s: ignoring 85.0°C power-on value, keeping %.1f°C\n", _id, _temperature);
    return false;

  case Action::UNCONFIRMED:
    // No valid value yet, but not a failed read either: waiting for the
    // filter to confirm 85 °C must not trigger a rescan that resets it.
    LOG_WARN("  ⚠ %s: unconfirmed 85.0°C power-on value - no temperature yet\n", _id);
    _temperature = NAN;
    _sensorFound = false;
    PoolController::DegradationManager::reportSensorStatus(_id, false);
    return false;

  case Action::REJECT:
    if (raw == DEVICE_DISCONNECTED_C) {
      LOG_ERROR("  ✖ %s: sensor disconnected - setting to NaN\n", _id);
    } else {
      LOG_ERROR("  ✖ %s: implausible reading %.1f°C - setting to NaN\n", _id, raw);
    }
    _temperature = NAN;
    _sensorFound = false;
    PoolController::DegradationManager::reportSensorStatus(_id, false);
    if (consecutiveFailures_ < UINT8_MAX) {
      consecutiveFailures_++;
    }
    return consecutiveFailures_ >= RESCAN_AFTER_FAILURES;
  }
  return false;
}

bool DallasTemperatureNode::targetAddress(DeviceAddress target) const {
  static const DeviceAddress kNoAddress{};
  if (hasFilter_.load(std::memory_order_acquire)) {
    lockCache();
    memcpy(target, filterAddr_, sizeof(DeviceAddress));
    unlockCache();
    return true;
  }
  if (memcmp(deviceAddress_, kNoAddress, sizeof(DeviceAddress)) != 0) {
    memcpy(target, deviceAddress_, sizeof(DeviceAddress));
    return true;
  }
  return false;
}

void DallasTemperatureNode::rescanBus() {
  // Runs on the task that owns the OneWire bus (SensorTask after startup).
  DallasTemperature *activeSensor = bus();
  consecutiveFailures_ = 0;
  filter_.reset();

  activeSensor->begin();
  activeSensor->setWaitForConversion(false);
  numberOfDevices = activeSensor->getDeviceCount();

  DeviceAddress target;
  if (!sharedSensor_ || !targetAddress(target)) {
    // A dedicated bus belongs to this node alone, so the usual resolution
    // (including index fallback) also picks up a replaced sensor. The same
    // applies to a shared-bus node that never resolved a device.
    if (numberOfDevices > 0) {
      resolveFilter();
    }
    LOG_INFO("  ◦ %s: %d device(s) found after rescan\n", _id, numberOfDevices);
    return;
  }

  // On the shared bus only this node's own address may come back. An index
  // fallback could hand the pool probe to the solar node when the solar probe
  // has dropped off the bus.
  memcpy(deviceAddress_, target, sizeof(DeviceAddress));
  publishSelectedAddress();
  uint8_t index = 0;
  if (PoolController::findDallasDeviceByAddress(*activeSensor, numberOfDevices, target, index)) {
    LOG_INFO("  ◦ %s: sensor found again after rescan (device %d)\n", _id, index);
    return;
  }

  _sensorFound = false;
  LOG_WARN("  ⚠ %s: sensor still missing after rescan (%d device(s) on bus)\n", _id, numberOfDevices);
}

void DallasTemperatureNode::address2String(const DeviceAddress deviceAddress, char *buffer, size_t size) const {
  snprintf(buffer, size, "%02X%02X%02X%02X%02X%02X%02X%02X", deviceAddress[0], deviceAddress[1], deviceAddress[2],
    deviceAddress[3], deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);
}
