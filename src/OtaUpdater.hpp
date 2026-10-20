// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file OtaUpdater.hpp
 * @brief OTA firmware update checker and installer — GitHub Releases integration.
 */

#pragma once

#include <Arduino.h>

#include <atomic>
#include <memory>

#include "OtaDownloadSession.hpp"

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

  /// True while downloading and flashing (until reboot or failure).
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

  /// Start the OTA update now: connect and request the firmware, then return.
  /// The TLS connect blocks once, bounded by the client timeout; the body is
  /// then streamed by loop(), one bounded step per iteration.
  /// Returns true if the download started. Call only from the loop task.
  /// Web UI and MQTT use requestUpdate(); this is the step loop() runs.
  static bool startUpdate();

  /// Request an OTA update (Web UI, MQTT). loop() starts it on its next
  /// iteration. Ignored while an update is already requested or running.
  static void requestUpdate();

  /// True while a requested update has not yet been started by loop().
  static bool isUpdateRequested();

  // ── Space and Size Verification ──

  /// Check if there's sufficient flash space for OTA update.
  /// @param firmwareSize Size of firmware in bytes.
  /// @return true if sufficient space available.
  static bool hasSufficientSpace(size_t firmwareSize);

  /// Get available flash space for OTA updates.
  /// @return Available space in bytes.
  static size_t getAvailableFlashSpace();

  // ── Semver helpers (public for native unit tests) ──
  struct Version {
    int major = 0, minor = 0, patch = 0;
  };
  static bool parseVersion(const String &str, Version &out);
  static bool isNewerVersion(const String &current, const String &latest);

private:
  // ── GitHub API ──
  static bool fetchLatestRelease();

  // ── OTA ──
  struct DownloadContext;
  /// Connect, validate the firmware size and start the flash write.
  /// @return nullptr on success, otherwise the user-facing error message.
  static const char *beginDownload(const String &url);
  static void stepDownload();
  static void completeDownload();
  static void abortDownload(OtaDownloadSession::Result result, size_t written, size_t total);
  static void failUpdate(const char *message);

  /// Update lifecycle. REQUESTED → CONNECTING → DOWNLOADING → reboot,
  /// or back to IDLE on failure.
  enum class State : uint8_t {
    IDLE,         ///< Nothing to do; periodic release checks may run
    REQUESTED,    ///< requestUpdate() was called; loop() starts it next
    CONNECTING,   ///< startUpdate() is connecting and validating the firmware
    DOWNLOADING,  ///< download_ streams the firmware, one step per loop()
  };

  // ── State ──
  static String currentVersion_;
  static String latestVersion_;
  static String releaseUrl_;
  static String downloadUrl_;
  static bool updateAvailable_;
  static std::atomic<State> state_;
  static std::unique_ptr<DownloadContext> download_;
  static int progress_;
  static String statusMessage_;
  static unsigned long lastCheckTime_;
  static unsigned long lastClockSyncFailTime_;
  static uint8_t clockSyncFailCount_;

  static constexpr unsigned long kCheckIntervalMs = 6UL * 3600UL * 1000UL;   // 6 hours
  static constexpr unsigned long kClockSyncBackoffMs = 5UL * 60UL * 1000UL;  // 5 minutes backoff
  static constexpr uint8_t kMaxClockSyncRetries = 3;
  static constexpr int kOtaBufferSize = 4096;
  static constexpr uint32_t kDownloadStallTimeoutMs = 30UL * 1000UL;         // abort after 30 s without data
  static constexpr uint32_t kDownloadTotalTimeoutMs = 10UL * 60UL * 1000UL;  // abort after 10 min overall
  static constexpr float kSpaceSafetyMargin = 0.15f;                         // 15% safety margin for OTA
  static constexpr size_t kMinFreeSpace = 1024 * 1024;                       // 1MB minimum free space
  static constexpr const char *kInsufficientSpaceMessage = "Error: Insufficient flash space";
};

}  // namespace PoolController
