// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file OtaUpdater.hpp
 * @brief OTA firmware update checker and installer — GitHub Releases integration.
 */

#pragma once

#include <Arduino.h>

namespace PoolController {

/**
 * @brief Checks GitHub Releases for new firmware, downloads and applies it via OTA.
 *
 * Periodically checks the GitHub Releases API for a newer version than the
 * currently running firmware (FW_VERSION from platformio.ini). Supports
 * manual update check and automatic install from the Web UI.
 * Uses the ESP32 Arduino Update library for OTA flashing.
 */
class OtaUpdater {
public:
  OtaUpdater() = default;

  /// Call once during setup.
  static void begin();

  /// Call periodically from main loop.
  static void loop();

  // ── Status ──

  /// True if a newer release was found on GitHub.
  static bool isUpdateAvailable();

  /// True while downloading and flashing.
  static bool isUpdateInProgress();

  /// Current running firmware version (FW_VERSION).
  static String getCurrentVersion();

  /// Latest version tag from GitHub (without "v" prefix).
  static String getLatestVersion();
  static String getLatestVersionTag();

  /// URL to the GitHub release page.
  static String getReleaseUrl();

  /// Download progress 0–100.
  static int getProgress();

  /// Human-readable status message.
  static String getStatusMessage();

  // ── Actions ──

  /// Check GitHub for a newer release. Returns true if update available.
  static bool checkForUpdate();

  /// Start the OTA download + flash. Returns true if started.
  static bool startUpdate();

private:
  // ── GitHub API ──
  static bool fetchLatestRelease();

  // ── Semver helpers ──
  struct Version {
    int major = 0, minor = 0, patch = 0;
  };
  static bool parseVersion(const String &str, Version &out);
  static bool isNewerVersion(const String &current, const String &latest);

  // ── OTA ──
  static bool downloadAndApply(const String &url);

  // ── State ──
  static String currentVersion_;
  static String latestVersion_;
  static String releaseUrl_;
  static String downloadUrl_;
  static bool updateAvailable_;
  static bool updateInProgress_;
  static int progress_;
  static String statusMessage_;
  static unsigned long lastCheckTime_;

  static constexpr unsigned long kCheckIntervalMs = 6UL * 3600UL * 1000UL;  // 6 hours
  static constexpr int kOtaBufferSize = 4096;
};

}  // namespace PoolController
