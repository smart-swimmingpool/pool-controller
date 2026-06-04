// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>

namespace PoolController {

class WebPortal {
public:
  WebPortal() = default;

  static bool begin();
  static void loop();

  // Session management
  static bool handleAuthentication();
  static void generateSessionToken();
  static bool isClientAuthenticated();

private:
  static void setupRoutes();

  // HTML Handlers
  static void handleRoot();
  static void handleLogin();
  static void handleNotFound();

  // Static Asset Handlers (LittleFS-served web assets with PROGMEM fallback)
  static void handleStyleCss();
  static void handleAppJs();

  // API Handlers
  static void apiGetStatus();
  static void apiScanWiFi();
  static void apiGetConfig();
  static void apiSaveConfig();
  static void apiSetMode();
  static void apiLogin();
  static void apiLogout();
  static void apiRestart();
  static void apiFactoryReset();
  static void apiUpdateCheck();
  static void apiUpdateStatus();
  static void apiUpdateInstall();

  static WebServer server_;
  static DNSServer dnsServer_;
  static bool dnsServerStarted_;

  static String activeSessionToken_;
  static uint32_t sessionStartTime_;

  static constexpr uint32_t kSessionTimeoutMs = 15 * 60 * 1000;  // 15 mins
  static constexpr uint16_t kDnsPort = 53;
};

}  // namespace PoolController
