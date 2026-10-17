#include "NetworkManager.hpp"

namespace PoolController {

espMqttClientAsync NetworkManager::_mqttClient;
bool NetworkManager::_wifiConnected = true;
bool NetworkManager::_mqttConnected = true;
bool NetworkManager::_apMode = false;
int NetworkManager::_wifiRssi = -65;
IPAddress NetworkManager::_localIp{192, 168, 1, 100};
bool NetworkManager::_publishOk = true;
std::function<void()> NetworkManager::_publishHook;
std::string NetworkManager::_publishHookTopic;
NetworkManager::MqttMessageCallback NetworkManager::_mqttCallback;

}  // namespace PoolController
