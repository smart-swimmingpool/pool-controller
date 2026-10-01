#include "NetworkManager.hpp"

namespace PoolController {

AsyncMqttClient NetworkManager::_mqttClient;
bool NetworkManager::_wifiConnected = true;
bool NetworkManager::_mqttConnected = true;
bool NetworkManager::_apMode = false;
int NetworkManager::_wifiRssi = -65;
bool NetworkManager::_publishOk = true;
std::function<void()> NetworkManager::_publishHook;
std::string NetworkManager::_publishHookTopic;

} // namespace PoolController
