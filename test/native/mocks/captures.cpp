// Global capture instances for test assertions
#include <map>
#include <string>
#include "Arduino.h"
#include "WebServer.h"
#include "AsyncMqttClient.h"

WebServerCapture wsCapture;
MqttClientCapture mqttCapture;

// Serial mock capture state (disabled by default)
std::string SerialClass::s_capture;
bool SerialClass::s_captureEnabled = false;

// Preferences static storage (shared across all instances like real NVS)
std::map<std::string, std::string> Preferences::s_data;
