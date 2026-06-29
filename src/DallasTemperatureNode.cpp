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
}

void DallasTemperatureNode::clearAddressFilter() {
  memset(filterAddr_, 0, sizeof(DeviceAddress));
  hasFilter_ = false;
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
    if (hasFilter_) {
      bool found = false;
      for (uint8_t i = 0; i < numberOfDevices; i++) {
        DeviceAddress addr;
        if (activeSensor->getAddress(addr, i)) {
          if (memcmp(addr, filterAddr_, sizeof(DeviceAddress)) == 0) {
            deviceIndex_ = i;
            memcpy(deviceAddress_, addr, sizeof(DeviceAddress));
            _sensorFound = true;
            found = true;
            char adr[18];
            address2String(addr, adr, sizeof(adr));
            Serial.printf("  ◦ %s: matched address filter → device %d [%s] ✓\n", _id, i, adr);
            break;
          }
        }
      }
      if (!found) {
        Serial.printf("  ✖ %s: address filter not found! "
                      "Falling back to device index %d\n",
          _id, deviceIndex_);
        if (activeSensor->getAddress(deviceAddress_, deviceIndex_)) {
          _sensorFound = true;
        }
      }
    } else {
      // ── No filter: cache address by deviceIndex_ ─────────────────────
      if (activeSensor->getAddress(deviceAddress_, deviceIndex_)) {
        _sensorFound = true;
        char adr[18];
        address2String(deviceAddress_, adr, sizeof(adr));
        Serial.printf("  ◦ %s: no filter → device %d [%s]", _id, deviceIndex_, adr);
        if (sharedSensor_) {
          Serial.print(" (shared bus)");
        }
        Serial.println();
      }
    }

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

void DallasTemperatureNode::loop() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;
  unsigned long effectiveInterval = std::isnan(_temperature) ? RECOVERY_INTERVAL : _measurementInterval;

  if (Utils::shouldMeasure(_lastMeasurement, effectiveInterval)) {
    _lastMeasurement = millis();

    if (sharedSensor_ && numberOfDevices > 0) {
      // ── Shared bus mode ────────────────────────────────────────────────
      // The master (deviceIndex 0) drives the conversion for all sensors.
      if (isBusMaster_) {
        Serial.printf("〽 Reading Dallas sensors (shared bus)\n");

        PoolController::SystemMonitor::feedWatchdog();
        activeSensor->requestTemperatures();
        PoolController::SystemMonitor::feedWatchdog();

        // Master reads its own sensor
        float newTemp = activeSensor->getTempC(deviceAddress_);
        if (newTemp == DEVICE_DISCONNECTED_C) {
          Serial.println("  ✖ Solar sensor disconnected - setting to NaN");
          _temperature = NAN;
          _sensorFound = false;
          PoolController::DegradationManager::reportSensorStatus(_id, false);
        } else {
          _temperature = newTemp;
          _sensorFound = true;
          PoolController::DegradationManager::reportSensorStatus(_id, true);
          Serial.printf("  ◦ Solar Temp = %.1f°C\n", _temperature);
        }
      } else {
        // Slave: read from the conversion the master already triggered
        float newTemp = activeSensor->getTempC(deviceAddress_);
        if (newTemp == DEVICE_DISCONNECTED_C) {
          Serial.println("  ✖ Pool sensor disconnected - setting to NaN");
          _temperature = NAN;
          _sensorFound = false;
          PoolController::DegradationManager::reportSensorStatus(_id, false);
        } else {
          _temperature = newTemp;
          _sensorFound = true;
          PoolController::DegradationManager::reportSensorStatus(_id, true);
          Serial.printf("  ◦ Pool Temp = %.1f°C\n", _temperature);
        }
      }
    } else if (numberOfDevices > 0) {
      // ── Dedicated bus mode (standard) ──────────────────────────────────
      Serial.printf("〽 Reading Dallas sensor: %s\n", _id);

      PoolController::SystemMonitor::feedWatchdog();
      activeSensor->requestTemperatures();
      PoolController::SystemMonitor::feedWatchdog();

      for (uint8_t i = 0; i < numberOfDevices; i++) {
        DeviceAddress tempDeviceAddress;
        if (activeSensor->getAddress(tempDeviceAddress, i)) {
          float newTemp = activeSensor->getTempC(tempDeviceAddress);
          if (newTemp == DEVICE_DISCONNECTED_C) {
            Serial.println("  ✖ Sensor disconnected - setting to NaN");
            _temperature = NAN;
            _sensorFound = false;
            PoolController::DegradationManager::reportSensorStatus(_id, false);
          } else {
            _temperature = newTemp;
            _sensorFound = true;
            PoolController::DegradationManager::reportSensorStatus(_id, true);
            Serial.printf("  ◦ Temp = %.1f°C\n", _temperature);
          }
        }
      }
    } else {
      // ── No sensor found — rescan ──────────────────────────────────────
      Serial.println("No Sensor found on bus! Rescanning...");
      PoolController::DegradationManager::reportSensorStatus(_id, false);

      if (sharedSensor_) {
        // In shared mode, rescan the shared bus
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
          Serial.printf("  ◦ %d device(s) found after rescan\n", numberOfDevices);
          _sensorFound = true;
        }
      }
    }
  }
}

void DallasTemperatureNode::address2String(const DeviceAddress deviceAddress, char *buffer, size_t size) const {
  snprintf(buffer, size, "%02X%02X%02X%02X%02X%02X%02X%02X", deviceAddress[0], deviceAddress[1], deviceAddress[2],
    deviceAddress[3], deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);
}
