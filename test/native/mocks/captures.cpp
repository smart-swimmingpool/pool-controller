// Global capture instances for test assertions
#include <map>
#include <string>
#include "Arduino.h"
#include "WebServer.h"
#include "espMqttClientAsync.h"
#include "HTTPClient.h"

WebServerCapture wsCapture;
MqttClientCapture mqttCapture;
HttpCapture httpCapture;

// Arduino mock clock offset (test hook).
uint32_t g_millisOffset = 0;

// ESP mock heap state (default mirrors the previous hardcoded value).
uint32_t ESPClass::s_mockFreeHeap = 180000;
int ESPClass::s_restartCount = 0;
MockStreamState mockStreamState;

const std::string &httpResponseBody() {
  return httpCapture.responseBody;
}

// Serial mock capture state (disabled by default)
std::string SerialClass::s_capture;
bool SerialClass::s_captureEnabled = false;

// Preferences static storage (shared across all instances like real NVS)
std::map<std::string, std::string> Preferences::s_data;
