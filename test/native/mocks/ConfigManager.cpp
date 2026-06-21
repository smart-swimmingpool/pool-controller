#include "ConfigManager.hpp"

namespace PoolController {

Settings ConfigManager::_settings;
WiFiConfig ConfigManager::_wifi;
MqttConfig ConfigManager::_mqtt;
NtpConfig ConfigManager::_ntp;

} // namespace PoolController
