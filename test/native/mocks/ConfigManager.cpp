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
bool ConfigManager::_recordInvalid = false;
bool ConfigManager::_saveFails = false;
bool ConfigManager::_sensorMappingReadFails = false;
bool ConfigManager::_sensorMappingSaveFails = false;

// ── Sensor Address Mapping ──
uint8_t ConfigManager::_sensorSolarAddr[8] = {0};
uint8_t ConfigManager::_sensorPoolAddr[8] = {0};
bool ConfigManager::_sensorSolarValid = false;
bool ConfigManager::_sensorPoolValid = false;

bool ConfigManager::saveSensorMapping(const uint8_t solarAddr[8], const uint8_t poolAddr[8]) {
  if (_sensorMappingSaveFails) {
    return false;
  }
  memcpy(_sensorSolarAddr, solarAddr, 8);
  memcpy(_sensorPoolAddr, poolAddr, 8);
  _sensorSolarValid = true;
  _sensorPoolValid = true;
  return true;
}

bool ConfigManager::loadSensorMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  memset(solarAddr, 0, 8);
  memset(poolAddr, 0, 8);
  if (_sensorSolarValid) memcpy(solarAddr, _sensorSolarAddr, 8);
  if (_sensorPoolValid) memcpy(poolAddr, _sensorPoolAddr, 8);
  return _sensorSolarValid || _sensorPoolValid;
}

void ConfigManager::seedSensorMappingForTest(
  const uint8_t solarAddr[8], bool solarPresent, const uint8_t poolAddr[8], bool poolPresent) {
  memcpy(_sensorSolarAddr, solarAddr, 8);
  memcpy(_sensorPoolAddr, poolAddr, 8);
  _sensorSolarValid = solarPresent;
  _sensorPoolValid = poolPresent;
}

} // namespace PoolController
