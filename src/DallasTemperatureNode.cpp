// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DallasTemperatureNode.cpp
 * @brief DS18B20 sensor node implementation with bus scanning, recovery, and
 *        degradation reporting.
 */

#include "DallasTemperatureNode.hpp"
#include "Config.hpp"
#include "SystemMonitor.hpp"
#include "DegradationManager.hpp"
#include "SensorSlots.hpp"
#include "Utils.hpp"

// ── Dedicated bus constructor ──────────────────────────────────────────────

DallasTemperatureNode::DallasTemperatureNode(const char *id, const char *name, const uint8_t pin, const int measurementInterval) {
  _id = id;
  _name = name;
  _pin = pin;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = 0;
  numberOfDevices = 0;
  _temperature = NAN;
  _sensorFound = false;

  oneWire.begin(_pin);
  sensor.setOneWire(&oneWire);
}

// ── Shared bus constructor ─────────────────────────────────────────────────

DallasTemperatureNode::DallasTemperatureNode(
  const char *id, const char *name, DallasTemperature *sharedSensor, uint8_t deviceIndex, const int measurementInterval) {
  _id = id;
  _name = name;
  _pin = 0;  // not used in shared mode
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement = 0;
  numberOfDevices = 0;
  _temperature = NAN;
  _sensorFound = false;

  sharedSensor_ = sharedSensor;
  deviceIndex_ = deviceIndex;
  isBusMaster_ = (deviceIndex == 0);

  // Clear the cached address
  memset(deviceAddress_, 0, sizeof(deviceAddress_));
}

// ═════════════════════════════════════════════════════════════════════════════
// ── Address filter helpers ─────────────────────────────────────────────────

void DallasTemperatureNode::setAddressFilter(const DeviceAddress addr) {
  memcpy(filterAddr_, addr, sizeof(DeviceAddress));
  hasFilter_ = true;

  if (numberOfDevices > 0) {
    resolveFilter();
  }
}

void DallasTemperatureNode::clearAddressFilter() {
  memset(filterAddr_, 0, sizeof(DeviceAddress));
  hasFilter_ = false;

  if (numberOfDevices > 0) {
    resolveFilter();
  }
}

bool DallasTemperatureNode::resolveFilter() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (hasFilter_ && numberOfDevices > 0) {
    // ── Address filter active: find the matching device on the bus ──
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress addr;
      if (activeSensor->getAddress(addr, i)) {
        if (memcmp(addr, filterAddr_, sizeof(DeviceAddress)) == 0) {
          // Note: intentionally NOT updating deviceIndex_ here — the constructor-
          // provided default (solar=0, pool=1) must survive as the fallback index
          // used by the no-filter path in clearAddressFilter().
          memcpy(deviceAddress_, addr, sizeof(DeviceAddress));
          _sensorFound = true;
          char adr[18];
          address2String(addr, adr, sizeof(adr));
          Serial.printf("  ◦ %s: filter resolved → device %d [%s] ✓\n", _id, i, adr);
          return true;
        }
      }
    }
    // Filter address not found on bus — fall back to deviceIndex_
    Serial.printf("  ✖ %s: filter address not found on bus! "
                  "Falling back to device index %d\n",
      _id, deviceIndex_);
    if (activeSensor->getAddress(deviceAddress_, deviceIndex_)) {
      _sensorFound = true;
      return true;
    }
    _sensorFound = false;
    return false;
  }

  if (numberOfDevices > 0) {
    // ── No filter: cache address by deviceIndex_ ───────────────────
    if (activeSensor->getAddress(deviceAddress_, deviceIndex_)) {
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
  }

  _sensorFound = false;
  return false;
}

void DallasTemperatureNode::getDeviceAddressString(char *buffer, size_t size) const {
  address2String(deviceAddress_, buffer, size);
}

bool DallasTemperatureNode::getDetectedDeviceAddress(uint8_t index, DeviceAddress addr) const {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : const_cast<DallasTemperature *>(&sensor);
  if (index >= numberOfDevices) {
    return false;
  }
  return activeSensor->getAddress(addr, index);
}

float DallasTemperatureNode::getDetectedDeviceTemperature(uint8_t index) const {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : const_cast<DallasTemperature *>(&sensor);
  if (index >= numberOfDevices) {
    return NAN;
  }
  DeviceAddress addr;
  if (!activeSensor->getAddress(addr, index)) {
    return NAN;
  }
  return activeSensor->getTempC(addr);
}

// ═════════════════════════════════════════════════════════════════════════════

void DallasTemperatureNode::begin() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (!sharedSensor_) {
    // Dedicated mode: init our own sensor
    activeSensor->begin();
  }
  // Shared mode: sensor->begin() was already called externally — just scan

  numberOfDevices = activeSensor->getDeviceCount();
  Serial.printf("• DallasTemperature: Parasite power is: %d\n", activeSensor->isParasitePowerMode());

  if (numberOfDevices > 0) {
    uint8_t displayPin = sharedSensor_ ? PoolController::PIN_DS_SOLAR : _pin;
    Serial.printf("  ◦ %d devices found on PIN %d\n", numberOfDevices, displayPin);

    // ── Print all detected devices ──────────────────────────────────────
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress addr;
      if (activeSensor->getAddress(addr, i)) {
        char adr[18];
        address2String(addr, adr, sizeof(adr));
        Serial.printf("  ◦ PIN %d: Device %d address: %s\n", displayPin, i, adr);
      }
    }

    // ── Resolve address filter (persistent mapping) ─────────────────────
    resolveFilter();

    // ── Report global status ───────────────────────────────────────────
    if (sharedSensor_) {
      PoolController::DegradationManager::reportSensorStatus("solar-temp", numberOfDevices > 0);
      PoolController::DegradationManager::reportSensorStatus("pool-temp", numberOfDevices > 1);
    } else {
      PoolController::DegradationManager::reportSensorStatus(_id, true);
    }
  } else {
    uint8_t displayPin = sharedSensor_ ? PoolController::PIN_DS_SOLAR : _pin;
    Serial.printf("✖ No Dallas sensors found on pin %d\n", displayPin);
    _sensorFound = false;
    PoolController::DegradationManager::reportSensorStatus(_id, false);
  }
}

void DallasTemperatureNode::beginMeasurement() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (sharedSensor_ && numberOfDevices > 0) {
    // Shared bus: only the master drives the conversion for all sensors.
    if (isBusMaster_) {
      PoolController::SystemMonitor::feedWatchdogFromTask();
      activeSensor->requestTemperatures();
      PoolController::SystemMonitor::feedWatchdogFromTask();
    }
  } else if (numberOfDevices > 0) {
    // Dedicated bus: start our own conversion.
    PoolController::SystemMonitor::feedWatchdogFromTask();
    activeSensor->requestTemperatures();
    PoolController::SystemMonitor::feedWatchdogFromTask();
  }
}

void DallasTemperatureNode::finishMeasurement() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (sharedSensor_ && numberOfDevices > 0) {
    // Shared bus: master and slave each read their own device.
    float newTemp = activeSensor->getTempC(deviceAddress_);
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
  } else if (numberOfDevices > 0) {
    // Dedicated bus: read all devices, take the last valid reading.
    bool foundAny = false;
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress tempDeviceAddress;
      if (activeSensor->getAddress(tempDeviceAddress, i)) {
        float newTemp = activeSensor->getTempC(tempDeviceAddress);
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
  } else {
    // No sensor found — rescan the bus.
    Serial.printf("No Sensor found on bus! Rescanning (%s)...\n", _id);
    PoolController::DegradationManager::reportSensorStatus(_id, false);
    PoolController::SensorSlots::write(slotId(), NAN, false);

    if (sharedSensor_) {
      activeSensor->begin();
      numberOfDevices = activeSensor->getDeviceCount();
      if (numberOfDevices > deviceIndex_) {
        activeSensor->getAddress(deviceAddress_, deviceIndex_);
        _sensorFound = true;
        PoolController::DegradationManager::reportSensorStatus(_id, true);
        Serial.printf("  ◦ %d device(s) found after rescan\n", numberOfDevices);
      }
    } else {
      activeSensor->begin();
      numberOfDevices = activeSensor->getDeviceCount();
      if (numberOfDevices > 0) {
        _sensorFound = true;
        Serial.printf("  ◦ %d device(s) found after rescan\n", numberOfDevices);
      }
    }
  }
}

void DallasTemperatureNode::loop() {
  unsigned long effectiveInterval = std::isnan(_temperature) ? RECOVERY_INTERVAL : _measurementInterval;
  if (Utils::shouldMeasure(_lastMeasurement, effectiveInterval)) {
    _lastMeasurement = millis();
    Serial.printf("〽 Reading Dallas sensor: %s\n", _id);
    beginMeasurement();
    // Sync fallback (tests / non-task callers): conversion is blocking here.
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
