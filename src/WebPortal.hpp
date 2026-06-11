// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file WebPortal.hpp
 * @brief HTTP web dashboard, REST API, and captive portal for the Pool Controller.
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>

namespace PoolController {

/**
 * @brief HTTP server providing the web dashboard, REST API, and captive portal.
 *
 * Serves the Web UI (HTML/CSS/JS from LittleFS with PROGMEM fallback),
 * handles login/session management, exposes a REST API for status and
 * configuration, starts a DNS captive portal in AP mode, and provides
 * OTA firmware update via the web interface.
 */
class WebPortal {
public:
  WebPortal() = default;

  /** @brief Start the HTTP server and optional DNS captive portal. @return true if server started successfully. */
  static bool begin();
  /** @brief Handle incoming HTTP client requests. Call periodically from main loop(). */
  static void loop();

  // ── Session management ──
  /** @brief Verify the client session token. @return true if a valid session exists. */
  static bool handleAuthentication();
  /** @brief Generate a new session token and store the start time. */
  static void generateSessionToken();
  /** @brief Check if the current HTTP request has a valid session token. @return true if authenticated. */
  static bool isClientAuthenticated();

private:
  /** @brief Register all HTTP routes, handlers, and static asset paths. */
  static void setupRoutes();

  // ── HTML Handlers ──
  /** @brief Serve the main dashboard HTML page. */
  static void handleRoot();
  /** @brief Serve the login page. */
  static void handleLogin();
  /** @brief Serve a 404 page for unmatched routes. */
  static void handleNotFound();

  // ── Static Asset Handlers (LittleFS-served web assets with PROGMEM fallback) ──
  /** @brief Serve style.css (LittleFS). */
  static void handleStyleCss();
  /** @brief Serve app.js (LittleFS). */
  static void handleAppJs();
  /** @brief Serve manifest.json for PWA (LittleFS). */
  static void handleManifestJson();
  /** @brief Serve sw.js service worker for PWA (LittleFS). */
  static void handleSwJs();
  /** @brief Serve icon.svg for PWA (LittleFS). */
  static void handleIconSvg();

  // ── REST API Handlers ──
  /** @brief GET /api/status — return JSON with all telemetry data. */
  static void apiGetStatus();
  /** @brief GET /api/wifi/scan — return JSON list of visible WiFi networks. */
  static void apiScanWiFi();
  /** @brief GET /api/config — return current configuration as JSON. */
  static void apiGetConfig();
  /** @brief POST /api/config — save configuration from JSON body. */
  static void apiSaveConfig();
  /** @brief POST /api/mode — set the operation mode. */
  static void apiSetMode();
  /** @brief POST /api/pump/toggle — toggle a specific relay. */
  static void apiTogglePump();
  /** @brief POST /api/login — authenticate and create a session. */
  static void apiLogin();
  /** @brief POST /api/logout — invalidate the current session. */
  static void apiLogout();
  /** @brief POST /api/restart — reboot the device. */
  static void apiRestart();
  /** @brief POST /api/factory-reset — wipe config and reboot. */
  static void apiFactoryReset();
  /** @brief GET /api/update-check — check GitHub for newer firmware. */
  static void apiUpdateCheck();
  /** @brief GET /api/update-status — return OTA update progress. */
  static void apiUpdateStatus();
  /** @brief POST /api/update-install — start OTA firmware download + flash. */
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
