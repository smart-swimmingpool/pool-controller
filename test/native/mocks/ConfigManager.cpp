#include "ConfigManager.hpp"

namespace PoolController {

Settings ConfigManager::_settings;
WiFiConfig ConfigManager::_wifi;
MqttConfig ConfigManager::_mqtt;
NtpConfig ConfigManager::_ntp;
String ConfigManager::_adminPasswordHash = "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";  // SHA-256("admin")
bool ConfigManager::_configured = false;

} // namespace PoolController
