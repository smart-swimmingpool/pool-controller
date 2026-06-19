// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file WebPortal.cpp
 * @brief Web server implementation — dashboard HTML, REST API handlers,
 *        static asset serving, session management, and OTA upload.
 */

#include "WebPortal.hpp"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <Update.h>
#include <memory>

// ESP32 specific headers
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <esp_random.h>
#else
#include <cstdlib> // for random() in native tests
#include <ctime>   // for time() in native tests
#endif

#include "ConfigManager.hpp"
#include "DegradationManager.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "NetworkManager.hpp"
#include "OtaUpdater.hpp"
#include "OperationModeNode.hpp"
#include "PoolController.hpp"
#include "RelayModuleNode.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"
#include "Version.h"

namespace PoolController {

WebServer WebPortal::server_(80);
DNSServer WebPortal::dnsServer_;
bool WebPortal::dnsServerStarted_ = false;
String WebPortal::activeSessionToken_ = "";
String WebPortal::csrfToken_ = "";
uint32_t WebPortal::sessionStartTime_ = 0;
uint32_t WebPortal::lastLoginAttemptTime_ = 0;
uint8_t WebPortal::loginAttemptCount_ = 0;
constexpr uint32_t WebPortal::kSessionTimeoutMs;
constexpr uint16_t WebPortal::kDnsPort;

// CSRF Protection
String WebPortal::csrfToken_ = "";
uint32_t WebPortal::csrfTokenTime_ = 0;

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

  // Initialize CSRF token
  generateCsrfToken();

  return true;
}

// ── CSRF Protection Methods ──────────────────────────────────

String WebPortal::generateCsrfToken() {
  // Generate a random token using millis() and random for entropy
  uint32_t randomValue = random(1000000, 9999999);
  uint32_t tokenValue = millis() + randomValue;
  // Convert to hex string manually to avoid String constructor ambiguity
  char tokenBuffer[17];  // 8 hex chars + null terminator
  snprintf(tokenBuffer, sizeof(tokenBuffer), "%08X", tokenValue);
  csrfToken_ = String(tokenBuffer);
  csrfTokenTime_ = millis();
  return csrfToken_;
}

bool WebPortal::validateCsrfToken(const String &token) {
  // Check if token matches and is not expired
  if (token == csrfToken_) {
    // Check if token is still valid (not expired)
    if (millis() - csrfTokenTime_ < kCsrfTokenTimeoutMs) {
      return true;
    }
    // Token expired, generate new one
    generateCsrfToken();
  }
  return false;
}

String WebPortal::getCurrentCsrfToken() {
  // Regenerate token if expired
  if (millis() - csrfTokenTime_ >= kCsrfTokenTimeoutMs) {
    generateCsrfToken();
  }
  return csrfToken_;
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

// Generate secure random token - uses ESP32 hardware RNG when available
static String generateSecureToken(size_t length) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  String token;

#if !defined(ESP32) && !defined(ARDUINO_ARCH_ESP32)
  // Seed random for native tests
  static bool seeded = false;
  if (!seeded) {
    srandom(time(nullptr));
    seeded = true;
  }
#endif

  for (size_t i = 0; i < length; i++) {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    // Use ESP32 hardware RNG for cryptographic security
    uint32_t randomValue = esp_random();
    token += charset[randomValue % (sizeof(charset) - 1)];
#else
    // Fallback for native tests - use standard random
    // Use String constructor with single char and then append
    char c = charset[random() % (sizeof(charset) - 1)];
    token = token + String(c);
#endif
  }

  return token;
}

void WebPortal::generateSessionToken() {
  // Generate cryptographically secure session token using ESP32 hardware RNG
  activeSessionToken_ = generateSecureToken(32);  // 32 characters = 192 bits of entropy
  csrfToken_ = generateSecureToken(32);  // Generate new CSRF token for the session
  sessionStartTime_ = millis();
  loginAttemptCount_ = 0;  // Reset login attempts on new session
}

void WebPortal::generateCsrfToken() {
  csrfToken_ = generateSecureToken(32);
}

String WebPortal::getCsrfToken() {
  return csrfToken_;
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

  // Sensor mapping endpoints
  server_.on("/api/sensors", HTTP_GET, []() {
    if (!handleAuthentication()) return;
    apiGetSensors();
  });
  server_.on("/api/sensors/map", HTTP_POST, []() {
    if (!handleAuthentication()) return;
    apiSaveSensorMapping();
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

  // Thresholds — also in apiGetConfig, but duplicated here so the dashboard
  // can show them without authentication (apiGetStatus is unauthenticated).
  doc["temp_max_pool"] = ConfigManager::getSettings().tempMaxPool;
  doc["temp_min_solar"] = ConfigManager::getSettings().tempMinSolar;

  // Effective runtime (temperature-based circulation) — actual minutes, not end-of-day
  {
    Rule *active = operationModeNode.getRule();
    doc["effective_runtime"] = (active != nullptr) ? active->getEffectiveRuntimeMinutes() : 0;
  }

  // Serialize directly to a pre-allocated buffer to minimize String usage
  // Use a static buffer to avoid heap fragmentation
  static char jsonBuffer[1024];
  size_t jsonLength = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
  if (jsonLength > 0) {
    server_.send(200, "application/json", jsonBuffer);
  } else {
    server_.send(500, "text/plain", "JSON serialization error");
  }
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

  // Serialize directly to buffer to minimize String usage
  // Increased buffer size to handle environments with many visible APs or long SSIDs
  // Per P2 review: avoid truncating WiFi scan JSON responses
  static char jsonBuffer[4096];
  size_t jsonLength = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
  if (jsonLength > 0) {
    // Check if serialization was truncated
    if (jsonLength >= sizeof(jsonBuffer) - 1) {
      server_.send(500, "text/plain", "JSON buffer overflow");
      return;
    }
    server_.send(200, "application/json", jsonBuffer);
  } else {
    server_.send(500, "text/plain", "JSON serialization error");
  }
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

  // Serialize directly to buffer to minimize String usage
  // Increased buffer size to handle long MQTT host/user and NTP server values
  // Per P2 review: check config JSON serialization for truncation
  static char jsonBuffer[2048];
  size_t jsonLength = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
  if (jsonLength > 0) {
    // Check if serialization was truncated
    if (jsonLength >= sizeof(jsonBuffer) - 1) {
      server_.send(500, "text/plain", "JSON buffer overflow");
      return;
    }
    server_.send(200, "application/json", jsonBuffer);
  } else {
    server_.send(500, "text/plain", "JSON serialization error");
  }
}

void WebPortal::apiSaveConfig() {
  if (!server_.hasArg("type")) {
    server_.send(400, "text/plain", "Missing Configuration Type");
    return;
  }

  String type = server_.arg("type");

  if (type == "wifi") {
    String ssid = server_.arg("ssid");
    String password = server_.arg("password");

    // Input validation for SSID
    if (ssid.length() == 0 || ssid.length() > 32) {
      server_.send(400, "text/plain", "Invalid SSID length (1-32 characters)");
      return;
    }

    // Input validation for password
    if (password.length() > 64) {
      server_.send(400, "text/plain", "Password too long (max 64 characters)");
      return;
    }

    // Basic character validation - SSID should be printable ASCII
    for (size_t i = 0; i < ssid.length(); i++) {
      char c = ssid.charAt(i);
      if (c < 32 || c > 126) {
        server_.send(400, "text/plain", "Invalid SSID characters");
        return;
      }
    }

    ConfigManager::getWiFi().ssid = ssid;
    ConfigManager::getWiFi().password = password;
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
    String newPassword = server_.arg("password");

    // Input validation for password
    if (newPassword.length() < 8) {
      server_.send(400, "text/plain", "Password must be at least 8 characters");
      return;
    }

    if (newPassword.length() > 64) {
      server_.send(400, "text/plain", "Password too long (max 64 characters)");
      return;
    }

    ConfigManager::setAdminPassword(newPassword);
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

bool WebPortal::isLoginLockedOut() {
  if (loginAttemptCount_ >= kMaxLoginAttempts) {
    uint32_t now = millis();
    // Check if lockout period has passed (handle unsigned wrap-around)
    if (now - lastLoginAttemptTime_ < kLoginLockoutMs &&
        now >= lastLoginAttemptTime_) {
      return true;  // Still locked out
    }
    // Lockout period has passed, reset counter
    loginAttemptCount_ = 0;
  }
  return false;
}

void WebPortal::apiLogin() {
  // Rate limiting check
  if (isLoginLockedOut()) {
    uint32_t remaining = kLoginLockoutMs - (millis() - lastLoginAttemptTime_);
    server_.sendHeader("Retry-After", String(remaining / 1000));
    server_.send(429, "text/plain", "Too many attempts. Try again later.");
    return;
  }

  if (!server_.hasArg("password")) {
    server_.send(400, "text/plain", "Password Required");
    return;
  }

  String pass = server_.arg("password");

  // Input validation - limit password length for NEW passwords only
  // Existing passwords (stored as SHA-256 hash) may be longer than 64 chars
  // and should still be allowed to authenticate (P2 review fix)
  // Only enforce max length when setting new passwords via apiSaveConfig

  if (ConfigManager::verifyAdminPassword(pass)) {
    generateSessionToken();
    // Cookie with HttpOnly, SameSite, and shorter expiry
    // Note: Secure attribute removed because device serves UI on HTTP (port 80)
    // Adding Secure would prevent browsers from sending cookie back over http://
    String cookieHeader = "session=" + activeSessionToken_ +
                          "; Path=/; HttpOnly; SameSite=Strict; Max-Age=600";
    server_.sendHeader("Set-Cookie", cookieHeader);
    loginAttemptCount_ = 0; // Reset on successful login
    server_.send(200, "text/plain", "OK");
  } else {
    // Increment failed attempt counter
    loginAttemptCount_++;
    lastLoginAttemptTime_ = millis();
    server_.send(401, "text/plain", "Unauthorized");
  }
}

void WebPortal::apiLogout() {
  activeSessionToken_ = "";
  // Clear cookie with SameSite attribute for consistency
  server_.sendHeader("Set-Cookie", "session=deleted; Path=/; HttpOnly; SameSite=Lax; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
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

// ── Sensor mapping helpers ─────────────────────────────────────────────

/**
 * @brief Parse a 16-character hex string into an 8-byte DeviceAddress.
 * @param hex  Hex string like "28AABBCCDDEEFF11" (case-insensitive).
 * @param addr Output buffer for the 8-byte address.
 * @return true on success, false if the string is malformed.
 */
static bool hexStringToAddress(const String &hex, uint8_t addr[8]) {
  if (hex.length() != 16) return false;
  for (uint8_t i = 0; i < 8; i++) {
    char byteStr[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
    char *end = nullptr;
    unsigned long val = strtoul(byteStr, &end, 16);
    if (end != byteStr + 2) return false;
    addr[i] = static_cast<uint8_t>(val);
  }
  return true;
}

/**
 * @brief Save a sensor-to-role address mapping to NVS.
 *
 * Stores the 8-byte ROM addresses for solar and pool sensors in the
 * `ds18b20` Preferences namespace, making them persist across reboots.
 *
 * @param solarAddr  Solar sensor address (or nullptr / all-zero to clear).
 * @param poolAddr   Pool sensor address (or nullptr / all-zero to clear).
 */
static void saveSensorMappingNvs(const uint8_t solarAddr[8], const uint8_t poolAddr[8]) {
  Preferences prefs;
  prefs.begin("ds18b20", false);  // read-write
  prefs.putBytes("solar_adr", solarAddr, 8);
  prefs.putBytes("pool_adr", poolAddr, 8);
  prefs.end();

  char buf[17];
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X",
    solarAddr[0], solarAddr[1], solarAddr[2], solarAddr[3],
    solarAddr[4], solarAddr[5], solarAddr[6], solarAddr[7]);
  Serial.printf("• Sensor mapping saved via WebUI — Solar [%s]", buf);
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X",
    poolAddr[0], poolAddr[1], poolAddr[2], poolAddr[3],
    poolAddr[4], poolAddr[5], poolAddr[6], poolAddr[7]);
  Serial.printf(", Pool [%s]\n", buf);
}

// ═══════════════════════════════════════════════════════════════════════
// Sensor REST API handlers
// ═══════════════════════════════════════════════════════════════════════

void WebPortal::apiGetSensors() {
  JsonDocument doc;

  // ── Detected devices ────────────────────────────────────────────────
  // Collect unique addresses from both temperature node buses.
  // In shared-bus mode (NORVI) this produces one list; in dual-bus mode
  // devices from both segments are merged.
  JsonArray detected = doc["detected"].to<JsonArray>();
  uint8_t maxDevices = max(solarTemperatureNode.getDeviceCount(), poolTemperatureNode.getDeviceCount());

  // Dedup with a fixed-size array (practical max ~20 DS18B20 per bus)
  static constexpr uint8_t kMaxDevices = 20;
  String seen[kMaxDevices];
  uint8_t seenCount = 0;

  for (uint8_t i = 0; i < maxDevices && seenCount < kMaxDevices; i++) {
    DeviceAddress addr;
    float temp = NAN;

    // Try solar node first, fall back to pool node
    if (i < solarTemperatureNode.getDeviceCount() && solarTemperatureNode.getDetectedDeviceAddress(i, addr)) {
      temp = solarTemperatureNode.getDetectedDeviceTemperature(i);
    } else if (i < poolTemperatureNode.getDeviceCount() && poolTemperatureNode.getDetectedDeviceAddress(i, addr)) {
      temp = poolTemperatureNode.getDetectedDeviceTemperature(i);
    } else {
      continue;
    }

    // Format address as hex string
    char addrStr[17];
    snprintf(addrStr, sizeof(addrStr), "%02X%02X%02X%02X%02X%02X%02X%02X",
      addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);

    // Deduplicate (same address may appear on shared bus)
    bool alreadySeen = false;
    for (uint8_t si = 0; si < seenCount; si++) {
      if (seen[si] == addrStr) {
        alreadySeen = true;
        break;
      }
    }
    if (alreadySeen) continue;
    seen[seenCount++] = addrStr;

    JsonObject dev = detected.add<JsonObject>();
    dev["address"] = addrStr;
    if (!isnan(temp)) dev["temperature"] = temp;
  }

  // ── Current role mapping ────────────────────────────────────────────
  JsonObject mapping = doc["mapping"].to<JsonObject>();

  if (solarTemperatureNode.hasAddressFilter()) {
    char buf[17];
    solarTemperatureNode.getDeviceAddressString(buf, sizeof(buf));
    mapping["solar"] = buf;
  } else {
    mapping["solar"] = nullptr;
  }

  if (poolTemperatureNode.hasAddressFilter()) {
    char buf[17];
    poolTemperatureNode.getDeviceAddressString(buf, sizeof(buf));
    mapping["pool"] = buf;
  } else {
    mapping["pool"] = nullptr;
  }

  // Also include whether the address is currently found on the bus
  mapping["solar_found"] = solarTemperatureNode.isSensorFound();
  mapping["pool_found"] = poolTemperatureNode.isSensorFound();

  // Sensor node names for display
  mapping["solar_name"] = "Solar Temperature";
  mapping["pool_name"] = "Pool Temperature";

  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}

void WebPortal::apiSaveSensorMapping() {
  uint8_t solarAddr[8] = {0};
  uint8_t poolAddr[8] = {0};
  bool hasSolar = false, hasPool = false;

  if (server_.hasArg("solar_addr") && server_.arg("solar_addr").length() > 0) {
    if (!hexStringToAddress(server_.arg("solar_addr"), solarAddr)) {
      server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid solar address format\"}");
      return;
    }
    hasSolar = true;
  }

  if (server_.hasArg("pool_addr") && server_.arg("pool_addr").length() > 0) {
    if (!hexStringToAddress(server_.arg("pool_addr"), poolAddr)) {
      server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid pool address format\"}");
      return;
    }
    hasPool = true;
  }

  if (!hasSolar && !hasPool) {
    server_.send(400, "application/json", "{\"status\":\"error\",\"message\":\"At least one sensor address required\"}");
    return;
  }

  // Save to NVS (persistent across reboots)
  saveSensorMappingNvs(solarAddr, poolAddr);

  // Apply to running instances immediately
  if (hasSolar) {
    solarTemperatureNode.setAddressFilter(solarAddr);
  }
  if (hasPool) {
    poolTemperatureNode.setAddressFilter(poolAddr);
  }

  server_.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Sensor mapping saved. Reboot to apply.\"}");
}

}  // namespace PoolController
