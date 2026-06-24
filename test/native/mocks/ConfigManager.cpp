#include "ConfigManager.hpp"

namespace PoolController {

Settings ConfigManager::_settings = {};
WiFiConfig ConfigManager::_wifi;
MqttConfig ConfigManager::_mqtt;
NtpConfig ConfigManager::_ntp;
String ConfigManager::_adminPasswordHash =  // SHA-256("admin")
    "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";
bool ConfigManager::_configured = false;

} // namespace PoolController
