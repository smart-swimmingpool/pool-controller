// Global capture instances for test assertions
#include <map>
#include <string>
#include "WebServer.h"
#include "AsyncMqttClient.h"

WebServerCapture wsCapture;
MqttClientCapture mqttCapture;

// Preferences static storage (shared across all instances like real NVS)
std::map<std::string, std::string> Preferences::s_data;
