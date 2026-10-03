// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DallasTemperatureNode.cpp
 * @brief DS18B20 sensor node implementation with bus scanning, recovery, and
 *        cross-core discovery snapshots.
 */

#include "DallasTemperatureNode.hpp"
#include "Config.hpp"
#include "SystemMonitor.hpp"
#include "DegradationManager.hpp"
#include "SensorSlots.hpp"
#include "Utils.hpp"

DallasTemperatureNode::DallasTemperatureNode(const char *id, const char *name, const uint8_t pin, const int measurementInterval) {
  _id = id;
  _name = name;
  _pin = pin;
  _measurementInterval.store(
    (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL, std::memory_order_relaxed);
  _lastMeasurement = 0;
  _temperature = NAN;
  _sensorFound = false;

  oneWire.begin(_pin);
  sensor.setOneWire(&oneWire);
}

DallasTemperatureNode::DallasTemperatureNode(
  const char *id, const char *name, DallasTemperature *sharedSensor, uint8_t deviceIndex, const int measurementInterval) {
  _id = id;
  _name = name;
  _pin = 0;
  _measurementInterval.store(
    (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL, std::memory_order_relaxed);
  _lastMeasurement = 0;
  _temperature = NAN;
  _sensorFound = false;

  sharedSensor_ = sharedSensor;
  deviceIndex_ = deviceIndex;
  isBusMaster_ = (deviceIndex == 0);
  memset(deviceAddress_, 0, sizeof(deviceAddress_));
}

void DallasTemperatureNode::lockCache() const {
  while (cacheLock_.test_and_set(std::memory_order_acquire)) {
    yield();
  }
}

void DallasTemperatureNode::unlockCache() const {
  cacheLock_.clear(std::memory_order_release);
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
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;
  DeviceAddress filter{};
  const bool hasFilter = hasFilter_.load(std::memory_order_acquire);

  if (hasFilter) {
    lockCache();
    memcpy(filter, filterAddr_, sizeof(DeviceAddress));
    unlockCache();
  }

  if (hasFilter && numberOfDevices > 0) {
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress addr;
      if (activeSensor->getAddress(addr, i) && memcmp(addr, filter, sizeof(DeviceAddress)) == 0) {
        memcpy(deviceAddress_, addr, sizeof(DeviceAddress));
        publishSelectedAddress();
        _sensorFound = true;
        char adr[18];
        address2String(addr, adr, sizeof(adr));
        Serial.printf("  ◦ %s: filter resolved → device %d [%s] ✓\n", _id, i, adr);
        return true;
      }
    }

    Serial.printf("  ✖ %s: filter address not found on bus! Falling back to device index %d\n", _id, deviceIndex_);
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
    Serial.printf("  ◦ %s: no filter → device %d [%s]", _id, deviceIndex_, adr);
    if (sharedSensor_) {
      Serial.print(" (shared bus)");
    }
    Serial.println();
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
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;
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
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (!sharedSensor_) {
    activeSensor->begin();
  }

  // SensorTask owns the conversion delay. Without this, requestTemperatures()
  // blocks internally and the task would wait a second time afterwards.
  activeSensor->setWaitForConversion(false);

  numberOfDevices = activeSensor->getDeviceCount();
  Serial.printf("• DallasTemperature: Parasite power is: %d\n", activeSensor->isParasitePowerMode());

  if (numberOfDevices > 0) {
    const uint8_t displayPin = sharedSensor_ ? PoolController::PIN_DS_SOLAR : _pin;
    Serial.printf("  ◦ %d devices found on PIN %d\n", numberOfDevices, displayPin);

    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress addr;
      if (activeSensor->getAddress(addr, i)) {
        char adr[18];
        address2String(addr, adr, sizeof(adr));
        Serial.printf("  ◦ PIN %d: Device %d address: %s\n", displayPin, i, adr);
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
    const uint8_t displayPin = sharedSensor_ ? PoolController::PIN_DS_SOLAR : _pin;
    Serial.printf("✖ No Dallas sensors found on pin %d\n", displayPin);
    _sensorFound = false;
    refreshDetectedSnapshot(false);
    PoolController::DegradationManager::reportSensorStatus(_id, false);
  }

  // Start in recovery cadence until the first valid measurement is published.
  PoolController::SensorSlots::write(slotId(), NAN, false);
}

void DallasTemperatureNode::beginMeasurement() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;
  applyPendingAddressFilter();

  if (sharedSensor_ && numberOfDevices > 0) {
    if (isBusMaster_) {
      PoolController::SystemMonitor::feedWatchdogFromTask();
      activeSensor->requestTemperatures();
      PoolController::SystemMonitor::feedWatchdogFromTask();
    }
  } else if (numberOfDevices > 0) {
    PoolController::SystemMonitor::feedWatchdogFromTask();
    activeSensor->requestTemperatures();
    PoolController::SystemMonitor::feedWatchdogFromTask();
  }
}

void DallasTemperatureNode::finishMeasurement() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (sharedSensor_ && numberOfDevices > 0) {
    const float newTemp = activeSensor->getTempC(deviceAddress_);
    if (newTemp == DEVICE_DISCONNECTED_C) {
      _temperature = NAN;
      _sensorFound = false;
      PoolController::DegradationManager::reportSensorStatus(_id, false);
      Serial.printf("  ✖ %s sensor disconnected - setting to NaN\n", _id);
    } else {
      _temperature = newTemp;
      _sensorFound = true;
      PoolController::DegradationManager::reportSensorStatus(_id, true);
      Serial.printf("  ◦ %s Temp = %.1f°C\n", _id, _temperature);
    }
    PoolController::SensorSlots::write(slotId(), _temperature, _sensorFound);
    refreshDetectedSnapshot(true);
    return;
  }

  if (numberOfDevices > 0) {
    bool foundAny = false;
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress tempDeviceAddress;
      if (activeSensor->getAddress(tempDeviceAddress, i)) {
        const float newTemp = activeSensor->getTempC(tempDeviceAddress);
        if (newTemp != DEVICE_DISCONNECTED_C) {
          _temperature = newTemp;
          foundAny = true;
        }
      }
    }
    _sensorFound = foundAny;
    PoolController::DegradationManager::reportSensorStatus(_id, foundAny);
    if (foundAny) {
      Serial.printf("  ◦ %s Temp = %.1f°C\n", _id, _temperature);
    } else {
      _temperature = NAN;
      Serial.printf("  ✖ %s sensor disconnected - setting to NaN\n", _id);
    }
    PoolController::SensorSlots::write(slotId(), _temperature, _sensorFound);
    refreshDetectedSnapshot(true);
    return;
  }

  Serial.printf("No Sensor found on bus! Rescanning (%s)...\n", _id);
  PoolController::DegradationManager::reportSensorStatus(_id, false);
  PoolController::SensorSlots::write(slotId(), NAN, false);

  activeSensor->begin();
  activeSensor->setWaitForConversion(false);
  numberOfDevices = activeSensor->getDeviceCount();
  if (numberOfDevices > 0) {
    resolveFilter();
    if (_sensorFound) {
      PoolController::DegradationManager::reportSensorStatus(_id, true);
    }
    Serial.printf("  ◦ %d device(s) found after rescan\n", numberOfDevices);
  } else {
    _sensorFound = false;
    memset(deviceAddress_, 0, sizeof(deviceAddress_));
    publishSelectedAddress();
  }
  refreshDetectedSnapshot(false);
}

void DallasTemperatureNode::loop() {
  const unsigned long effectiveInterval = std::isnan(_temperature) ? RECOVERY_INTERVAL : getMeasurementInterval();
  if (Utils::shouldMeasure(_lastMeasurement, effectiveInterval)) {
    _lastMeasurement = millis();
    Serial.printf("〽 Reading Dallas sensor: %s\n", _id);
    beginMeasurement();
    // Synchronous fallback for tests/non-task callers. Production uses
    // SensorTask, which yields instead of blocking the control loop.
    delay(SYNC_CONVERSION_DELAY_MS);
    finishMeasurement();
  }
}

PoolController::SensorId DallasTemperatureNode::slotId() const {
  return (_id[0] == 's') ? PoolController::SensorId::SOLAR : PoolController::SensorId::POOL;
}

void DallasTemperatureNode::address2String(const DeviceAddress deviceAddress, char *buffer, size_t size) const {
  snprintf(buffer, size, "%02X%02X%02X%02X%02X%02X%02X%02X", deviceAddress[0], deviceAddress[1], deviceAddress[2],
    deviceAddress[3], deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);
}
