// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file WebPortal.cpp
 * @brief Web server implementation — dashboard HTML, REST API handlers,
 *        static asset serving, session management, and OTA upload.
 */

#include "WebPortal.hpp"
#include "Version.h"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "SystemMonitor.hpp"
#include "DegradationManager.hpp"
#include "OperationModeNode.hpp"
#include "PoolController.hpp"
#include "OtaUpdater.hpp"
#include "TimeClientHelper.hpp"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <Update.h>
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"

namespace PoolController {

WebServer WebPortal::server_(80);
DNSServer WebPortal::dnsServer_;
bool WebPortal::dnsServerStarted_ = false;
String WebPortal::activeSessionToken_ = "";
uint32_t WebPortal::sessionStartTime_ = 0;
constexpr uint32_t WebPortal::kSessionTimeoutMs;
constexpr uint16_t WebPortal::kDnsPort;

// Nodes declared in PoolController.cpp
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;
extern RelayModuleNode poolPumpNode;
extern RelayModuleNode solarPumpNode;
extern OperationModeNode operationModeNode;

// PROGMEM fallbacks removed — web assets are served from LittleFS only.
// Run `pio run --target uploadfs` to deploy data/web/ to the device.

bool WebPortal::begin() {
  if (!LittleFS.begin(false)) {
    Serial.println("✖ LittleFS mount failed — static web assets may be unavailable");
  }

  setupRoutes();

  // If AP mode, setup Captive DNS Server
  if (NetworkManager::isApMode()) {
    dnsServer_.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer_.start(kDnsPort, "*", WiFi.softAPIP());
    Serial.println("✓ Captive Portal DNS running.");
    dnsServerStarted_ = true;
  }

  server_.begin();
  Serial.println("✓ Web Server running on port 80.");
  return true;
}

void WebPortal::loop() {
  if (NetworkManager::isApMode()) {
    if (!dnsServerStarted_) {
      dnsServer_.setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer_.start(kDnsPort, "*", WiFi.softAPIP());
      Serial.println("✓ Captive Portal DNS running.");
      dnsServerStarted_ = true;
    }
    dnsServer_.processNextRequest();
  }
  server_.handleClient();

  // Session timeout checking
  if (activeSessionToken_.length() > 0 && (millis() - sessionStartTime_ > kSessionTimeoutMs)) {
    Serial.println("Session timed out.");
    activeSessionToken_ = "";
  }
}

void WebPortal::generateSessionToken() {
  activeSessionToken_ = String(random(100000, 999999));
  sessionStartTime_ = millis();
}

bool WebPortal::isClientAuthenticated() {
  if (NetworkManager::isApMode()) {
    // In AP fallback mode with saved credentials, require admin password.
    // Only allow unauthenticated AP access for truly unconfigured devices.
    if (!ConfigManager::isConfigured()) {
      return true;  // Initial setup — no password set yet
    }
    // Fall through to session-cookie check below
  }

  // Only check cookie if a session token was actually generated
  // (empty token would match 'session=' in any cookie)
  if (activeSessionToken_.length() > 0 && server_.hasHeader("Cookie")) {
    String cookie = server_.header("Cookie");
    if (cookie.indexOf("session=" + activeSessionToken_) != -1) {
      sessionStartTime_ = millis();  // Refresh timeout
      return true;
    }
  }
  return false;
}

bool WebPortal::handleAuthentication() {
  if (!isClientAuthenticated()) {
    handleLogin();
    return false;
  }
  return true;
}

void WebPortal::setupRoutes() {
  // HTML Handlers
  server_.on("/", handleRoot);
  server_.on("/login", handleLogin);

  // Static web assets (no authentication required)
  server_.on("/style.css", HTTP_GET, handleStyleCss);
  server_.on("/app.js", HTTP_GET, handleAppJs);
  server_.on("/manifest.json", HTTP_GET, handleManifestJson);
  server_.on("/sw.js", HTTP_GET, handleSwJs);
  server_.on("/icon.svg", HTTP_GET, handleIconSvg);

  // API Handlers (some password protected)
  server_.on("/api/status", HTTP_GET, apiGetStatus);
  server_.on("/api/scan", HTTP_GET, []() {
    if (!handleAuthentication())
      return;
    apiScanWiFi();
  });
  server_.on("/api/config", HTTP_GET, []() {
    if (!handleAuthentication())
      return;
    apiGetConfig();
  });
  server_.on("/api/config", HTTP_POST, []() {
    if (!handleAuthentication())
      return;
    apiSaveConfig();
  });
  server_.on("/api/mode", HTTP_POST, []() {
    if (!handleAuthentication())
      return;
    apiSetMode();
  });
  server_.on("/api/pump", HTTP_POST, []() {
    if (!handleAuthentication())
      return;
    apiTogglePump();
  });

  server_.on("/api/login", HTTP_POST, apiLogin);
  server_.on("/api/logout", HTTP_GET, apiLogout);
  server_.on("/api/restart", HTTP_GET, []() {
    if (!handleAuthentication())
      return;
    apiRestart();
  });
  server_.on("/api/factory_reset", HTTP_GET, []() {
    if (!handleAuthentication())
      return;
    apiFactoryReset();
  });

  // OTA Update management
  server_.on("/api/update/check", HTTP_GET, []() {
    if (!handleAuthentication())
      return;
    apiUpdateCheck();
  });
  server_.on("/api/update/status", HTTP_GET, []() {
    if (!handleAuthentication())
      return;
    apiUpdateStatus();
  });
  server_.on("/api/update/install", HTTP_POST, []() {
    if (!handleAuthentication())
      return;
    apiUpdateInstall();
  });

  // OTA Firmware handling (manual upload via web form)
  server_.on(
    "/api/update", HTTP_POST,
    []() {
      if (!handleAuthentication())
        return;
      server_.sendHeader("Connection", "close");
      server_.send(Update.hasError() ? 500 : 200, "text/plain", Update.hasError() ? "FAIL" : "OK");
      delay(1000);
      ESP.restart();
    },
    []() {
      if (!isClientAuthenticated())
        return;
      HTTPUpload &upload = server_.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Signed OTA Update Starting: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("Signed OTA Update Success: %u bytes\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });

  server_.onNotFound(handleNotFound);

  // Collect Cookie headers for authentication checking
  const char *headerkeys[] = {"Cookie"};
  server_.collectHeaders(headerkeys, 1);
}

void WebPortal::handleRoot() {
  if (!isClientAuthenticated()) {
    handleLogin();
    return;
  }

  File f = LittleFS.open("/web/index.html", "r");
  if (f) {
    server_.streamFile(f, "text/html");
    f.close();
    return;
  }

  // No PROGMEM fallback — tell user to upload web assets
  String html = R"HTML(
<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Pool Controller</title>
<style>body{background:#06121e;color:#8aadc4;font-family:system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;padding:2rem;text-align:center;line-height:1.6}
h1{color:#00e5ff;margin-bottom:0.5rem}a{color:#48cae4}</style>
</head><body>
<h1>Pool Controller</h1>
<p>Firmware is running but web assets are not uploaded.</p>
<p style="font-size:0.875rem">Run <code style="background:rgba(255,255,255,0.08);padding:0.2rem 0.5rem;border-radius:4px">pio run --target uploadfs</code> to deploy <code>data/web/</code> to the device.</p>
<p style="margin-top:2rem;font-size:0.8rem">Or visit <a href="/update">/update</a> for OTA firmware update.</p>
</body></html>
)HTML";
  server_.send(200, "text/html", html);
}

void WebPortal::handleStyleCss() {
  File f = LittleFS.open("/web/style.css", "r");
  if (f) {
    server_.streamFile(f, "text/css");
    f.close();
    return;
  }
  server_.send(404, "text/plain", "Not Found");
}

void WebPortal::handleAppJs() {
  File f = LittleFS.open("/web/app.js", "r");
  if (f) {
    server_.streamFile(f, "application/javascript");
    f.close();
    return;
  }
  server_.send(404, "text/plain", "Not Found");
}

void WebPortal::handleManifestJson() {
  File f = LittleFS.open("/web/manifest.json", "r");
  if (f) {
    server_.streamFile(f, "application/manifest+json");
    f.close();
    return;
  }
  server_.send(404, "text/plain", "Not Found");
}

void WebPortal::handleSwJs() {
  File f = LittleFS.open("/web/sw.js", "r");
  if (f) {
    server_.streamFile(f, "application/javascript");
    f.close();
    return;
  }
  server_.send(404, "text/plain", "Not Found");
}

void WebPortal::handleIconSvg() {
  File f = LittleFS.open("/web/icon.svg", "r");
  if (f) {
    server_.streamFile(f, "image/svg+xml");
    f.close();
    return;
  }
  server_.send(404, "text/plain", "Not Found");
}

// Minimal inline login — no PROGMEM CSS framework, just a functional form
void WebPortal::handleLogin() {
  String html = R"HTML(
<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Pool Controller — Login</title>
<style>body{background:#06121e;color:#e2f0f7;font-family:system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;padding:2rem}
.card{background:rgba(8,28,48,0.75);backdrop-filter:blur(16px);border:1px solid rgba(0,200,220,0.12);border-radius:16px;padding:2rem;max-width:400px;width:100%}
h1{color:#00e5ff;text-align:center;margin-bottom:0.25rem;font-size:1.5rem}
p{color:#8aadc4;text-align:center;margin-bottom:2rem;font-size:0.9rem}
input{width:100%;padding:0.75rem 1rem;background:rgba(0,0,0,0.3);border:1px solid rgba(0,200,220,0.1);border-radius:8px;color:#e2f0f7;font-size:1rem}
input:focus{outline:none;border-color:#00e5ff;box-shadow:0 0 0 3px rgba(0,229,255,0.2)}
button{width:100%;padding:0.75rem;margin-top:1rem;background:linear-gradient(135deg,#00b4d8,#00e5ff);border:none;border-radius:8px;color:#000;font-weight:600;font-size:1rem;cursor:pointer}
.error{color:#ef4444;margin-top:0.5rem;display:none;font-size:0.875rem}
</style></head><body>
<div class="card">
<h1>Pool Controller</h1>
<p>Web Administration Sign In</p>
<form id="loginForm">
<input type="password" id="password" required placeholder="Administrator Password">
<div class="error" id="error">Invalid Password!</div>
<button type="submit">Unlock Console</button>
</form></div>
<script>
document.getElementById('loginForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const pwd = document.getElementById('password').value;
  const err = document.getElementById('error');
  const res = await fetch('/api/login', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'password=' + encodeURIComponent(pwd)
  });
  if (res.status === 200) { window.location.reload(); }
  else { err.style.display = 'block'; }
});
</script></body></html>
)HTML";
  server_.send(200, "text/html", html);
}
void WebPortal::handleNotFound() {
  // Captive Portal DNS redirection
  if (NetworkManager::isApMode()) {
    server_.sendHeader("Location", "http://192.168.4.1/", true);
    server_.send(302, "text/plain", "");
    return;
  }

  server_.send(404, "text/plain", "Page Not Found");
}

void WebPortal::apiGetStatus() {
  JsonDocument doc;

  doc["pool_temp"] = poolTemperatureNode.getTemperature();
  doc["solar_temp"] = solarTemperatureNode.getTemperature();
  doc["ctrl_temp"] = ctrlTemperatureNode.getTemperature();
  doc["pool_pump"] = poolPumpNode.getSwitch();
  doc["solar_pump"] = solarPumpNode.getSwitch();
  doc["op_mode"] = operationModeNode.getMode();
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["max_alloc"] = ESP.getMaxAllocHeap();
  doc["ap_mode"] = NetworkManager::isApMode();
  doc["rssi"] = NetworkManager::getWiFiRSSI();
  doc["wifi_connected"] = NetworkManager::isWiFiConnected();
  doc["mqtt_connected"] = NetworkManager::isMqttConnected();
  doc["local_ip"] = NetworkManager::getLocalIP();
  doc["fw_version"] = FW_VERSION;

  // Current date/time in configured timezone
  TimeChangeRule *tcr;
  time_t localTime = getTimeFor(ConfigManager::getSettings().timezoneIndex, &tcr);
  char timeBuf[64];
  snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d", year(localTime), month(localTime), day(localTime),
    hour(localTime), minute(localTime), second(localTime));
  doc["local_time"] = timeBuf;
  doc["local_time_epoch"] = static_cast<long>(localTime);
  doc["timezone_name"] = getTimeInfoFor(ConfigManager::getSettings().timezoneIndex);
  doc["time_degradation"] = static_cast<int>(getTimeDegradation());

  // Effective runtime (temperature-based circulation) — actual minutes, not end-of-day
  {
    Rule *active = operationModeNode.getRule();
    doc["effective_runtime"] = (active != nullptr) ? active->getEffectiveRuntimeMinutes() : 0;
  }

  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}

void WebPortal::apiScanWiFi() {
  int n = WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < n; ++i) {
    JsonObject obj = arr.add<JsonObject>();
    obj["ssid"] = WiFi.SSID(i);
    obj["rssi"] = WiFi.RSSI(i);
    obj["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }

  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}

void WebPortal::apiGetConfig() {
  JsonDocument doc;

  JsonObject wifiObj = doc["wifi"].to<JsonObject>();
  wifiObj["ssid"] = ConfigManager::getWiFi().ssid;

  JsonObject mqttObj = doc["mqtt"].to<JsonObject>();
  mqttObj["host"] = ConfigManager::getMqtt().host;
  mqttObj["port"] = ConfigManager::getMqtt().port;
  mqttObj["username"] = ConfigManager::getMqtt().username;

  JsonObject ntpObj = doc["ntp"].to<JsonObject>();
  ntpObj["server"] = ConfigManager::getNtp().server;

  JsonObject settingsObj = doc["settings"].to<JsonObject>();
  settingsObj["op_mode"] = ConfigManager::getSettings().opMode;
  settingsObj["loop_interval"] = ConfigManager::getSettings().loopInterval;
  settingsObj["temp_max_pool"] = ConfigManager::getSettings().tempMaxPool;
  settingsObj["temp_min_solar"] = ConfigManager::getSettings().tempMinSolar;
  settingsObj["temp_hysteresis"] = ConfigManager::getSettings().tempHysteresis;
  settingsObj["temp_circ_threshold"] = ConfigManager::getSettings().tempCircThreshold;
  settingsObj["temp_circ_factor"] = ConfigManager::getSettings().tempCircFactor;
  settingsObj["temp_circ_max_runtime"] = ConfigManager::getSettings().tempCircMaxRuntime;
  settingsObj["timezone"] = ConfigManager::getSettings().timezoneIndex;
  settingsObj["time_loss_green_hours"] = ConfigManager::getSettings().timeLossGreenHours;
  settingsObj["time_loss_red_hours"] = ConfigManager::getSettings().timeLossRedHours;
  settingsObj["timer_start_hour"] = operationModeNode.getTimerSetting().timerStartHour;
  settingsObj["timer_start_min"] = operationModeNode.getTimerSetting().timerStartMinutes;
  settingsObj["timer_end_hour"] = operationModeNode.getTimerSetting().timerEndHour;
  settingsObj["timer_end_min"] = operationModeNode.getTimerSetting().timerEndMinutes;

  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}

void WebPortal::apiSaveConfig() {
  if (!server_.hasArg("type")) {
    server_.send(400, "text/plain", "Missing Configuration Type");
    return;
  }

  String type = server_.arg("type");

  if (type == "wifi") {
    ConfigManager::getWiFi().ssid = server_.arg("ssid");
    ConfigManager::getWiFi().password = server_.arg("password");
    ConfigManager::setConfigured(true);  // P1: Mark device as configured
    ConfigManager::save();
    server_.send(200, "text/plain", "OK");

    // Restart soon after saving new WiFi connection setup
    delay(2000);
    ESP.restart();
    return;
  } else if (type == "mqtt") {
    ConfigManager::getMqtt().host = server_.arg("host");
    ConfigManager::getMqtt().port = server_.arg("port").toInt();
    ConfigManager::getMqtt().username = server_.arg("username");
    // Preserve existing password if field is left blank (P2 review fix)
    if (server_.arg("password").length() > 0) {
      ConfigManager::getMqtt().password = server_.arg("password");
    }
    ConfigManager::save();

    // Disconnect MQTT to reconnect immediately with new config
    NetworkManager::disconnectMqtt();
    server_.send(200, "text/plain", "OK");
    return;
  } else if (type == "settings") {
    ConfigManager::getSettings().opMode = server_.arg("mode");
    ConfigManager::getSettings().loopInterval = server_.arg("interval").toInt();
    ConfigManager::getSettings().tempMaxPool = server_.arg("max_pool").toFloat();
    ConfigManager::getSettings().tempMinSolar = server_.arg("min_solar").toFloat();
    ConfigManager::getSettings().tempHysteresis = server_.arg("hysteresis").toFloat();
    if (server_.hasArg("circ_threshold"))
      ConfigManager::getSettings().tempCircThreshold = server_.arg("circ_threshold").toFloat();
    if (server_.hasArg("circ_factor"))
      ConfigManager::getSettings().tempCircFactor = server_.arg("circ_factor").toInt();
    if (server_.hasArg("circ_max_runtime"))
      ConfigManager::getSettings().tempCircMaxRuntime = server_.arg("circ_max_runtime").toInt();
    ConfigManager::getSettings().timezoneIndex = server_.arg("timezone").toInt();
    ConfigManager::getSettings().timeLossGreenHours = server_.arg("green").toInt();
    ConfigManager::getSettings().timeLossRedHours = server_.arg("red").toInt();

    ConfigManager::save();

    // Apply timezone change to running clock immediately (P2)
    setTimezoneIndex(ConfigManager::getSettings().timezoneIndex);

    // Apply time-loss degradation thresholds immediately (P2 review fix)
    setTimeDegradationGreenHours(static_cast<uint8_t>(ConfigManager::getSettings().timeLossGreenHours));
    setTimeDegradationRedHours(static_cast<uint8_t>(ConfigManager::getSettings().timeLossRedHours));

    // Propagate loop interval to all running nodes (P2 review fix)
    solarTemperatureNode.setMeasurementInterval(ConfigManager::getSettings().loopInterval);
    poolTemperatureNode.setMeasurementInterval(ConfigManager::getSettings().loopInterval);
    ctrlTemperatureNode.setMeasurementInterval(ConfigManager::getSettings().loopInterval);
    poolPumpNode.setMeasurementInterval(ConfigManager::getSettings().loopInterval);
    solarPumpNode.setMeasurementInterval(ConfigManager::getSettings().loopInterval);
    operationModeNode.setMeasurementInterval(ConfigManager::getSettings().loopInterval);

    // Propagate changes directly into runtime parameters
    operationModeNode.setMode(ConfigManager::getSettings().opMode.c_str());
    operationModeNode.setPoolMaxTemperature(ConfigManager::getSettings().tempMaxPool);
    operationModeNode.setSolarMinTemperature(ConfigManager::getSettings().tempMinSolar);
    operationModeNode.setTemperatureHysteresis(ConfigManager::getSettings().tempHysteresis);

    // NTP server live update
    if (server_.hasArg("ntp_server")) {
      ConfigManager::getNtp().server = server_.arg("ntp_server");
      ConfigManager::save();
      timeClientSetup(ConfigManager::getNtp().server.c_str());
    }

    // Apply timer settings
    {
      TimerSetting ts = operationModeNode.getTimerSetting();
      if (server_.hasArg("timer_start_h"))
        ts.timerStartHour = server_.arg("timer_start_h").toInt();
      if (server_.hasArg("timer_start_m"))
        ts.timerStartMinutes = server_.arg("timer_start_m").toInt();
      if (server_.hasArg("timer_end_h"))
        ts.timerEndHour = server_.arg("timer_end_h").toInt();
      if (server_.hasArg("timer_end_m"))
        ts.timerEndMinutes = server_.arg("timer_end_m").toInt();
      operationModeNode.setTimerSetting(ts);
    }

    server_.send(200, "text/plain", "OK");
    return;
  } else if (type == "password") {
    ConfigManager::setAdminPassword(server_.arg("password"));
    ConfigManager::save();
    server_.send(200, "text/plain", "OK");
    return;
  }

  server_.send(400, "text/plain", "Invalid Config Request");
}

void WebPortal::apiSetMode() {
  if (!server_.hasArg("mode")) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing mode\"}");
    return;
  }

  String mode = server_.arg("mode");
  if (operationModeNode.setMode(mode)) {
    ConfigManager::getSettings().opMode = mode;
    ConfigManager::save();
    server_.send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"" + mode + "\"}");
  } else {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid mode\"}");
  }
}

void WebPortal::apiTogglePump() {
  if (!server_.hasArg("pump")) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing pump parameter\"}");
    return;
  }

  // Only allow pump toggling in manual mode
  if (operationModeNode.getMode() != "manu") {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Pump control only available in manual mode\"}");
    return;
  }

  String pump = server_.arg("pump");
  bool newState;

  if (pump == "pool") {
    newState = !poolPumpNode.getSwitch();
    poolPumpNode.setSwitch(newState);
  } else if (pump == "solar") {
    newState = !solarPumpNode.getSwitch();
    solarPumpNode.setSwitch(newState);
  } else {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid pump. Use 'pool' or 'solar'\"}");
    return;
  }

  String json = "{\"status\":\"ok\",\"state\":" + String(newState ? "true" : "false") + "}";
  server_.send(200, "application/json", json);
}

void WebPortal::apiLogin() {
  if (!server_.hasArg("password")) {
    server_.send(400, "text/plain", "Password Required");
    return;
  }

  String pass = server_.arg("password");
  if (ConfigManager::verifyAdminPassword(pass)) {
    generateSessionToken();
    server_.sendHeader("Set-Cookie", "session=" + activeSessionToken_ + "; Path=/; HttpOnly; Max-Age=900");
    server_.send(200, "text/plain", "OK");
  } else {
    server_.send(401, "text/plain", "Unauthorized");
  }
}

void WebPortal::apiLogout() {
  activeSessionToken_ = "";
  server_.sendHeader("Set-Cookie", "session=deleted; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
  server_.send(200, "text/plain", "OK");
}

void WebPortal::apiRestart() {
  server_.send(200, "text/plain", "OK");
  delay(1000);
  ESP.restart();
}

void WebPortal::apiFactoryReset() {
  server_.send(200, "text/plain", "OK");

  // Clear NVS Preferences (relay states, operation mode, timer settings)
  // that survive a config.json wipe (P2 review fix)
  Preferences prefs;
  prefs.begin("pool-pump", false);
  prefs.clear();
  prefs.end();
  prefs.begin("solar-pump", false);
  prefs.clear();
  prefs.end();
  prefs.begin("pool-controller", false);
  prefs.clear();
  prefs.end();

  ConfigManager::reset();
  ConfigManager::save();
  delay(1000);
  ESP.restart();
}

void WebPortal::apiUpdateCheck() {
  if (!NetworkManager::isWiFiConnected()) {
    server_.send(503, "application/json", "{\"status\":\"error\",\"message\":\"No WiFi\"}");
    return;
  }

  if (OtaUpdater::isUpdateInProgress()) {
    server_.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Update in progress\"}");
    return;
  }

  bool available = OtaUpdater::checkForUpdate();

  JsonDocument doc;
  doc["status"] = available ? "update_available" : "up_to_date";
  doc["current_version"] = OtaUpdater::getCurrentVersion();
  doc["latest_version"] = OtaUpdater::getLatestVersion();
  if (available) {
    doc["release_url"] = OtaUpdater::getReleaseUrl();
  }

  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}

void WebPortal::apiUpdateStatus() {
  JsonDocument doc;
  doc["current_version"] = OtaUpdater::getCurrentVersion();
  doc["latest_version"] = OtaUpdater::getLatestVersion();
  doc["update_available"] = OtaUpdater::isUpdateAvailable();
  doc["update_in_progress"] = OtaUpdater::isUpdateInProgress();
  doc["progress"] = OtaUpdater::getProgress();
  doc["status_message"] = OtaUpdater::getStatusMessage();
  doc["release_url"] = OtaUpdater::getReleaseUrl();

  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}

void WebPortal::apiUpdateInstall() {
  if (!NetworkManager::isWiFiConnected()) {
    server_.send(503, "application/json", "{\"status\":\"error\",\"message\":\"No WiFi\"}");
    return;
  }

  if (!OtaUpdater::isUpdateAvailable()) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No update available\"}");
    return;
  }

  if (OtaUpdater::isUpdateInProgress()) {
    server_.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Already installing\"}");
    return;
  }

  // Send immediate response before update starts
  server_.send(200, "application/json", "{\"status\":\"started\"}");

  OtaUpdater::startUpdate();
  // If we return here, the update failed (success reboots)
}

}  // namespace PoolController
