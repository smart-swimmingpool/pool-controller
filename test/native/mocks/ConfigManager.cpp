#include "ConfigManager.hpp"
#include <cstring>

namespace PoolController {

Settings ConfigManager::_settings = {};
WiFiConfig ConfigManager::_wifi;
MqttConfig ConfigManager::_mqtt;
NtpConfig ConfigManager::_ntp;
String ConfigManager::_adminPasswordHash =  // SHA-256("admin")
    "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";
bool ConfigManager::_configured = false;
bool ConfigManager::_saveFails = false;

// ── Sensor Address Mapping ──
uint8_t ConfigManager::_sensorSolarAddr[8] = {0};
uint8_t ConfigManager::_sensorPoolAddr[8] = {0};
bool ConfigManager::_sensorMappingValid = false;

void ConfigManager::saveSensorMapping(const uint8_t solarAddr[8], const uint8_t poolAddr[8]) {
  memcpy(_sensorSolarAddr, solarAddr, 8);
  memcpy(_sensorPoolAddr, poolAddr, 8);
  _sensorMappingValid = true;
}

bool ConfigManager::loadSensorMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  if (!_sensorMappingValid) return false;
  memcpy(solarAddr, _sensorSolarAddr, 8);
  memcpy(poolAddr, _sensorPoolAddr, 8);
  return true;
}

} // namespace PoolController
