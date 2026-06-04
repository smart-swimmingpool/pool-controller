// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "DallasTemperatureNode.hpp"
#include "SystemMonitor.hpp"
#include "DegradationManager.hpp"
#include "Utils.hpp"

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

void DallasTemperatureNode::begin() {
  sensor.begin();
  numberOfDevices = sensor.getDeviceCount();
  Serial.printf("• DallasTemperature: Parasite power is: %d\n", sensor.isParasitePowerMode());

  if (numberOfDevices > 0) {
    Serial.printf("  ◦ %d devices found on PIN %d\n", numberOfDevices, _pin);
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress tempDeviceAddress;
      if (sensor.getAddress(tempDeviceAddress, i)) {
        char adr[18];
        address2String(tempDeviceAddress, adr, sizeof(adr));
        Serial.printf("  ◦ PIN %d: Device %d address: %s\n", _pin, i, adr);
      }
    }
    _sensorFound = true;
    PoolController::DegradationManager::reportSensorStatus(_id, true);
  } else {
    Serial.printf("✖ No Dallas sensors found on pin %d\n", _pin);
    _sensorFound = false;
    PoolController::DegradationManager::reportSensorStatus(_id, false);
  }
}

void DallasTemperatureNode::loop() {
  unsigned long effectiveInterval = std::isnan(_temperature) ? RECOVERY_INTERVAL : _measurementInterval;

  if (Utils::shouldMeasure(_lastMeasurement, effectiveInterval)) {
    _lastMeasurement = millis();

    if (numberOfDevices > 0) {
      Serial.printf("〽 Reading Dallas sensor: %s\n", _id);
      
      // Feed watchdog before blocking 1-Wire operations
      PoolController::SystemMonitor::feedWatchdog();

      sensor.requestTemperatures();

      PoolController::SystemMonitor::feedWatchdog();

      for (uint8_t i = 0; i < numberOfDevices; i++) {
        DeviceAddress tempDeviceAddress;
        if (sensor.getAddress(tempDeviceAddress, i)) {
          float newTemp = sensor.getTempC(tempDeviceAddress);
          if (newTemp == DEVICE_DISCONNECTED_C) {
            Serial.println("  ✖ Sensor disconnected - setting to NaN");
            _temperature = NAN;
            _sensorFound = false;
            PoolController::DegradationManager::reportSensorStatus(_id, false);
          } else {
            _temperature = newTemp;
            _sensorFound = true;
            PoolController::DegradationManager::reportSensorStatus(_id, true);
            Serial.printf("  ◦ Temp = %f\n", _temperature);
          }
        }
      }
    } else {
      Serial.println("No Sensor found on bus! Rescanning...");
      PoolController::DegradationManager::reportSensorStatus(_id, false);
      
      sensor.begin();
      numberOfDevices = sensor.getDeviceCount();
      if (numberOfDevices > 0) {
        Serial.printf("  ◦ %d device(s) found after rescan\n", numberOfDevices);
        _sensorFound = true;
      }
    }
  }
}

void DallasTemperatureNode::address2String(const DeviceAddress deviceAddress, char* buffer, size_t size) {
  snprintf(buffer, size, "%02X%02X%02X%02X%02X%02X%02X%02X",
    deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3],
    deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);
}
