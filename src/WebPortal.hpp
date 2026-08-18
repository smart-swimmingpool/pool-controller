// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file WebPortal.hpp
 * @brief HTTP web dashboard, REST API, and captive portal for the Pool Controller.
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "LogCapture.hpp"

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
  /** @brief Generate a new CSRF token. */
  static void generateCsrfToken();
  /** @brief Get the current CSRF token. */
  static String getCsrfToken();
  /** @brief Check if login is currently locked out due to too many attempts. */
  static bool isLoginLockedOut();
  /** @brief Check if the current HTTP request has a valid session token. @return true if authenticated. */
  static bool isClientAuthenticated();

  // ── Rate limiting helpers (public for testing) ──
  /** @brief Reset login attempt counter. */
  static void resetLoginAttempts() { loginAttemptCount_ = 0; }
  /** @brief Increment login attempt counter. */
  static void incrementLoginAttempts() {
    loginAttemptCount_++;
    lastLoginAttemptTime_ = millis();
  }
  /** @brief Get maximum login attempts before lockout. */
  static uint8_t getMaxLoginAttempts() { return kMaxLoginAttempts; }
  /** @brief Get login lockout duration in milliseconds. */
  static uint32_t getLoginLockoutMs() { return kLoginLockoutMs; }

  // ── Log view helper (public for testing) ──
  /** @brief Serialize LogCapture entries as the /api/logs JSON payload.
   *         @param epoch boot epoch of the client's cursor (see LogCapture::epoch()).
   *         @return bytes written (0 on error / empty buffer). */
  static size_t buildLogsJson(uint32_t since, uint32_t epoch, size_t count, LogLevel minLevel, char *buf, size_t bufSize);

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
  /** @brief Serve a LittleFS web asset, preferring a pre-compressed .gz variant. */
  static bool serveWebFile(const char *path, const char *contentType, const char *cacheControl);

  // ── REST API Handlers ──
  /** @brief GET /api/status — return JSON with all telemetry data. */
  static void apiGetStatus();
  /** @brief GET /api/logs — return JSON with captured log entries (unauthenticated, read-only). */
  static void apiGetLogs();
  /** @brief POST /api/logs/clear — empty the LogCapture ring buffer (authenticated). */
  static void apiClearLogs();
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
  /** @brief GET /api/sensors — list detected DS18B20 devices and current role mapping. */
  static void apiGetSensors();
  /** @brief POST /api/sensors/map — save sensor-to-role address mapping to NVS. */
  static void apiSaveSensorMapping();
  /** @brief POST /api/fs/upload — upload a file to LittleFS (OTA-safe web asset deployment). */
  static void apiFsUpload();
  /** @brief Streaming upload handler for /api/fs/upload multipart file data. */
  static void handleFsUploadStream();
  /** @brief POST /api/calibrate/start — start the NORVI button calibration wizard. */
  static void apiCalibrateStart();
  /** @brief GET /api/calibrate/status — return calibration status as JSON. */
  static void apiCalibrateStatus();
  /** @brief POST /api/calibrate/cancel — cancel a running calibration. */
  static void apiCalibrateCancel();

  static WebServer server_;
  static DNSServer dnsServer_;
  static bool dnsServerStarted_;

  static String activeSessionToken_;
  static String csrfToken_;
  static uint32_t sessionStartTime_;
  static uint32_t lastLoginAttemptTime_;
  static uint8_t loginAttemptCount_;

  static constexpr uint32_t kSessionTimeoutMs = 10 * 60 * 1000;  // 10 mins (reduced from 15)
  static constexpr uint16_t kDnsPort = 53;

  static constexpr uint8_t kMaxLoginAttempts = 5;
  static constexpr uint32_t kLoginLockoutMs = 60 * 1000;  // 1 minute lockout
};

}  // namespace PoolController
