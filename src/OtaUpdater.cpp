// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file OtaUpdater.cpp
 * @brief OTA firmware update implementation — GitHub API check, download, and flashing.
 */

#include "OtaUpdater.hpp"
#include "Version.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"
#include "LogCapture.hpp"

namespace PoolController {

// USERTrust ECC Certification Authority — self-signed root CA that GitHub's
// current TLS chain (api.github.com and release/object downloads) actually
// terminates at: leaf -> "Sectigo Public Server Authentication CA DV E36"
// -> "Sectigo Public Server Authentication Root E46" -> this root.
// Verified against the live chain on 2026-07-11 via:
//   openssl s_client -connect api.github.com:443 -showcerts
// The previously pinned ISRG Root X1 (Let's Encrypt) does NOT appear anywhere
// in GitHub's chain and caused every OTA check/download to fail TLS
// verification silently (fetchLatestRelease()/downloadAndApply() both
// returned false with no HTTP-level diagnostic beyond "connection failed").
// Valid until 2038-01-18; if GitHub rotates its CA again, this constant
// needs updating (see openspec/specs/github-ca-chain.spec.md for the
// longer-term fix: a real CA bundle instead of a single pinned root).
// SHA256 fingerprint: 4F:F4:60:D5:4B:9C:86:DA:BF:BC:FC:57:12:E0:40:0D:
//                     2B:ED:3F:BC:4D:4F:BD:AA:86:E0:6A:DC:D2:A9:AD:7A
static const char kGitHubRootCA[] PROGMEM = "-----BEGIN CERTIFICATE-----\n"
                                            "MIICjzCCAhWgAwIBAgIQXIuZxVqUxdJxVt7NiYDMJjAKBggqhkjOPQQDAzCBiDEL\n"
                                            "MAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNl\n"
                                            "eSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMT\n"
                                            "JVVTRVJUcnVzdCBFQ0MgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAwMjAx\n"
                                            "MDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgT\n"
                                            "Ck5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVUaGUg\n"
                                            "VVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBFQ0MgQ2VydGlm\n"
                                            "aWNhdGlvbiBBdXRob3JpdHkwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQarFRaqflo\n"
                                            "I+d61SRvU8Za2EurxtW20eZzca7dnNYMYf3boIkDuAUU7FfO7l0/4iGzzvfUinng\n"
                                            "o4N+LZfQYcTxmdwlkWOrfzCjtHDix6EznPO/LlxTsV+zfTJ/ijTjeXmjQjBAMB0G\n"
                                            "A1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1xmNjmjAOBgNVHQ8BAf8EBAMCAQYwDwYD\n"
                                            "VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNoADBlAjA2Z6EWCNzklwBBHU6+4WMB\n"
                                            "zzuqQhFkoJ2UOQIReVx7Hfpkue4WQrO/isIJxOzksU0CMQDpKmFHjFJKS04YcPbW\n"
                                            "RNZu9YO6bVi9JNlWSOrvxKJGgYhqOkbRqZtNyWHa0V1Xahg=\n"
                                            "-----END CERTIFICATE-----\n";

// ── Statics ──

String OtaUpdater::currentVersion_ = FW_VERSION;
String OtaUpdater::latestVersion_;
String OtaUpdater::releaseUrl_;
String OtaUpdater::downloadUrl_;
bool OtaUpdater::updateAvailable_ = false;
bool OtaUpdater::updateInProgress_ = false;
int OtaUpdater::progress_ = 0;
String OtaUpdater::statusMessage_;
unsigned long OtaUpdater::lastCheckTime_ = 0;
unsigned long OtaUpdater::lastClockSyncFailTime_ = 0;
uint8_t OtaUpdater::clockSyncFailCount_ = 0;

// ── Public API ──

void OtaUpdater::begin() {
  LOG_INFO("✓ OTA Updater initialized (current: %s)\n", currentVersion_.c_str());
  statusMessage_ = "Idle";
}

void OtaUpdater::loop() {
  // Periodic check when WiFi is connected and no update is in progress
  if (NetworkManager::isWiFiConnected() && !updateInProgress_) {
    unsigned long now = millis();

    // Check if we're in clock sync backoff period
    if (clockSyncFailCount_ > 0) {
      // Handle unsigned wrap-around for backoff timer
      // unsigned subtraction handles millis() wrap-around correctly (C++ standard, modulo arithmetic)
      if (now - lastClockSyncFailTime_ >= kClockSyncBackoffMs) {
        // Backoff period expired, reset counter
        clockSyncFailCount_ = 0;
      } else {
        // Still in backoff period, skip this check
        return;
      }
    }

    // Handle wrap-around for main check timer
    if (now - lastCheckTime_ >= kCheckIntervalMs || lastCheckTime_ == 0) {
      lastCheckTime_ = now;
      bool checkSuccess = checkForUpdate();
      // Only reset retry timer for TLS/time failures, not for successful checks
      // checkForUpdate() returns true if update available, false if up-to-date or failed
      // We want to retry immediately only on TLS/time failures, not on successful checks
      if (!checkSuccess) {
        // Check if this was a TLS/time failure by checking if we have a valid time
        // If time is still not set, it's likely a TLS failure and we should retry
        time_t currentTime = time(nullptr);
        if (currentTime < 100000) {  // Time not set - likely TLS failure
          // Increment failure counter for backoff
          clockSyncFailCount_++;
          lastClockSyncFailTime_ = now;

          // If we haven't exceeded max retries, reset timer for immediate retry
          // Otherwise, wait for backoff period
          if (clockSyncFailCount_ < kMaxClockSyncRetries) {
            lastCheckTime_ = 0;  // Retry immediately
          }
          // Otherwise, backoff timer will prevent retry until period expires
        }
        // Otherwise, it was a successful check (up-to-date) - keep the timer
      } else {
        // Successful check - reset backoff counter
        clockSyncFailCount_ = 0;
      }
    }
  }
}

bool OtaUpdater::isUpdateAvailable() {
  return updateAvailable_;
}

bool OtaUpdater::isUpdateInProgress() {
  return updateInProgress_;
}

String OtaUpdater::getCurrentVersion() {
  return currentVersion_;
}

String OtaUpdater::getLatestVersion() {
  if (latestVersion_.length() > 0 && latestVersion_[0] == 'v') {
    return latestVersion_.substring(1);
  }
  return latestVersion_;
}

String OtaUpdater::getLatestVersionTag() {
  return latestVersion_;
}

String OtaUpdater::getReleaseUrl() {
  return releaseUrl_;
}

int OtaUpdater::getProgress() {
  return progress_;
}

String OtaUpdater::getStatusMessage() {
  return statusMessage_;
}

// ── Check for Update ──

bool OtaUpdater::checkForUpdate() {
  if (updateInProgress_)
    return false;

  statusMessage_ = "Checking for updates...";
  LOG_INFO("OTA: Checking for firmware update...\n");

  if (!fetchLatestRelease()) {
    statusMessage_ = "Check failed";
    LOG_ERROR("OTA: Update check failed\n");
    return false;
  }

  // Compare versions
  if (isNewerVersion(currentVersion_, latestVersion_)) {
    updateAvailable_ = true;
    statusMessage_ = "Update available: v" + getLatestVersion();
    LOG_INFO("OTA: New version available: %s (current: %s)\n", latestVersion_.c_str(), currentVersion_.c_str());
    return true;
  }

  updateAvailable_ = false;
  statusMessage_ = "Up to date (v" + currentVersion_ + ")";
  LOG_INFO("OTA: Firmware is up to date\n");
  return false;
}

// ── Start Update ──

bool OtaUpdater::startUpdate() {
  if (updateInProgress_) {
    LOG_WARN("OTA: Update already in progress\n");
    return false;
  }
  if (!updateAvailable_ || downloadUrl_.length() == 0) {
    statusMessage_ = "No update available";
    LOG_WARN("OTA: No update package available\n");
    return false;
  }
  if (!NetworkManager::isWiFiConnected()) {
    statusMessage_ = "No WiFi connection";
    return false;
  }

  // Check available flash space before starting OTA
  // We need to estimate the firmware size from the URL or use a default
  // For GitHub releases, we can try to get the size from the API response
  // For now, we'll use a conservative estimate based on typical firmware size
  // In a real implementation, we would have the actual size from the API

  // Try to extract expected size from download URL if available
  // For GitHub, we don't have the size in the URL, so we use a default estimate
  // Typical Pool Controller firmware is ~400-600KB
  const size_t estimatedFirmwareSize = 600 * 1024;  // 600KB estimate

  if (!hasSufficientSpace(estimatedFirmwareSize)) {
    LOG_ERROR("OTA: Cannot start update: insufficient flash space\n");
    return false;
  }

  // Config persists in NVS — survives OTA natively, no backup needed
  updateInProgress_ = true;
  // Don't clear updateAvailable_ yet — preserved so UI can retry on failure (P2 review fix)
  progress_ = 0;
  statusMessage_ = "Downloading... 0%";

  LOG_INFO("OTA: Starting download from %s\n", downloadUrl_.c_str());

  bool ok = downloadAndApply(downloadUrl_);
  if (!ok) {
    updateInProgress_ = false;
    statusMessage_ = "Update failed!";
    LOG_ERROR("OTA: Update failed!\n");
    // updateAvailable_ stays true so the user can retry
  } else {
    // On success, clear the flag before reboot
    updateAvailable_ = false;
  }
  // If success, ESP will reboot — we never reach here
  return ok;
}

// ── Private: GitHub API ──

bool OtaUpdater::fetchLatestRelease() {
#ifndef GITHUB_REPO
  statusMessage_ = "GITHUB_REPO not defined";
  return false;
#endif

  // Sync time before TLS verification to ensure certificate validity checks pass
  // GitHub's TLS certificates require valid system time (P2 review fix)
  // Only sync if time is not already set (time(0) > 100000 means year 2000+)
  time_t now = time(nullptr);
  if (now < 100000) {  // Time not set yet
    LOG_INFO("OTA: Syncing time before GitHub TLS verification...\n");
    // Ensure NTP client is set up
    timeClientSetup(ConfigManager::getNtp().server.c_str());
    // Sync system clock from NTP (sets settimeofday for mbedTLS)
    syncSystemClock();
    // Wait for sync to complete
    delay(2000);
    now = time(nullptr);
    if (now < 100000) {
      LOG_WARN("OTA: Time sync failed, TLS verification may fail\n");
      // Continue anyway - the TLS verification might still work
      // if the device has a valid time from a previous sync
    } else {
      LOG_INFO("OTA: Time synced: %ld\n", now);
    }
  }

  WiFiClientSecure client;
  // Pin the actual root CA GitHub's chain terminates at (see kGitHubRootCA
  // comment above). NOTE: `setCACertBundle(x509_crt_bundle)` was previously
  // attempted here behind an `#if defined(x509_crt_bundle)` guard, but
  // that symbol is never actually defined in this project (it requires
  // generating and embedding a CA bundle binary — see
  // openspec/specs/github-ca-chain.spec.md, tasks T1/T2, still open) — the
  // guard was always false and silently fell back to this same single-cert
  // path, so it was removed as dead code (YAGNI).
  client.setCACert(kGitHubRootCA);
  client.setTimeout(10000);

  // Build API URL
  String url = String("https://api.github.com/repos/") + GITHUB_REPO + "/releases/latest";
  LOG_DEBUG("OTA: Fetching %s\n", url.c_str());

  HTTPClient http;
  http.begin(client, url);
  http.setUserAgent("PoolController/1.0");
  http.addHeader("Accept", "application/vnd.github+json");

  int httpCode = http.GET();
  if (httpCode != 200) {
    LOG_ERROR("OTA: GitHub API returned HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  // Parse response (typically < 2KB)
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err) {
    LOG_ERROR("OTA: JSON parse error: %s\n", err.c_str());
    return false;
  }

  // Extract tag name (e.g. "v3.2.0")
  const char *tag = doc["tag_name"];
  if (!tag || strlen(tag) == 0) {
    LOG_ERROR("OTA: No tag_name in response\n");
    return false;
  }
  latestVersion_ = String(tag);

  // Release URL
  const char *htmlUrl = doc["html_url"];
  releaseUrl_ = htmlUrl ? String(htmlUrl) : "";

  // Determine expected firmware asset name for this board type
  String expectedAsset;
#ifdef NORVI_AE01_R
  expectedAsset = "firmware-norvi_ae01_r.bin";
#else
  expectedAsset = "firmware-esp32dev.bin";
#endif

  // Find the firmware binary asset — prefer board-specific, fall back to generic
  JsonArray assets = doc["assets"].as<JsonArray>();
  downloadUrl_ = "";
  for (JsonObject asset : assets) {
    const char *name = asset["name"];
    if (!name)
      continue;
    if (strstr(name, ".bin") == nullptr)
      continue;

    // Board-specific match takes priority
    if (expectedAsset == String(name)) {
      const char *url = asset["browser_download_url"];
      if (url) {
        downloadUrl_ = String(url);
        break;
      }
    }
  }

  // Fallback: first generic .bin asset
  if (downloadUrl_.length() == 0) {
    for (JsonObject asset : assets) {
      const char *name = asset["name"];
      if (name && strstr(name, ".bin") != nullptr) {
        const char *url = asset["browser_download_url"];
        if (url) {
          downloadUrl_ = String(url);
          LOG_WARN("OTA: Board-specific asset not found, using generic firmware.bin\n");
          break;
        }
      }
    }
  }

  if (downloadUrl_.length() == 0) {
    LOG_ERROR("OTA: No firmware binary found in release assets\n");
    return false;
  }

  LOG_INFO("OTA: Found %s → %s\n", latestVersion_.c_str(), downloadUrl_.c_str());
  return true;
}

// ── Semver helpers ──

bool OtaUpdater::parseVersion(const String &str, Version &out) {
  // Strip leading "v" or "V"
  String s = str;
  s.trim();
  if (s.length() > 0 && (s[0] == 'v' || s[0] == 'V')) {
    s = s.substring(1);
  }
  int n = sscanf(s.c_str(), "%d.%d.%d", &out.major, &out.minor, &out.patch);
  return n >= 3;
}

bool OtaUpdater::isNewerVersion(const String &current, const String &latest) {
  Version cur, lat;
  if (!parseVersion(current, cur) || !parseVersion(latest, lat)) {
    // If we can't parse, err on the side of no update
    return false;
  }
  if (lat.major > cur.major)
    return true;
  if (lat.major < cur.major)
    return false;
  if (lat.minor > cur.minor)
    return true;
  if (lat.minor < cur.minor)
    return false;
  if (lat.patch > cur.patch)
    return true;
  return false;
}

// ── OTA Download + Flash ──

bool OtaUpdater::downloadAndApply(const String &url) {
  WiFiClientSecure client;
  // Pin the actual root CA GitHub's chain terminates at (see kGitHubRootCA
  // comment above). NOTE: `setCACertBundle(x509_crt_bundle)` was previously
  // attempted here behind an `#if defined(x509_crt_bundle)` guard, but
  // that symbol is never actually defined in this project (it requires
  // generating and embedding a CA bundle binary — see
  // openspec/specs/github-ca-chain.spec.md, tasks T1/T2, still open) — the
  // guard was always false and silently fell back to this same single-cert
  // path, so it was removed as dead code (YAGNI).
  client.setCACert(kGitHubRootCA);
  client.setTimeout(10000);

  HTTPClient http;
  http.begin(client, url);
  http.setUserAgent("PoolController/1.0");
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  if (httpCode != 200) {
    LOG_ERROR("OTA: Download returned HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  int totalSize = http.getSize();
  if (totalSize <= 0) {
    LOG_ERROR("OTA: Invalid content size\n");
    http.end();
    return false;
  }

  // Verify firmware size is reasonable (prevents integer overflow and bad downloads)
  // Typical firmware sizes: 200KB - 2MB
  const size_t kMaxFirmwareSize = 2 * 1024 * 1024;  // 2MB maximum
  const size_t kMinFirmwareSize = 50 * 1024;        // 50KB minimum

  if (static_cast<size_t>(totalSize) > kMaxFirmwareSize) {
    LOG_ERROR("OTA: Firmware too large: %d bytes (max %u)\n", totalSize, kMaxFirmwareSize);
    statusMessage_ = "Error: Firmware too large";
    http.end();
    return false;
  }

  if (static_cast<size_t>(totalSize) < kMinFirmwareSize) {
    LOG_ERROR("OTA: Firmware too small: %d bytes (min %u)\n", totalSize, kMinFirmwareSize);
    statusMessage_ = "Error: Firmware too small";
    http.end();
    return false;
  }

  // Verify we have sufficient space for this specific firmware size
  if (!hasSufficientSpace(static_cast<size_t>(totalSize))) {
    LOG_ERROR("OTA: Insufficient space for this firmware\n");
    http.end();
    return false;
  }

  LOG_DEBUG("OTA: Download size: %d bytes\n", totalSize);

  if (!Update.begin(totalSize)) {
    LOG_ERROR("OTA: Update.begin() failed: %s\n", Update.errorString());
    http.end();
    return false;
  }

  // Stream download in chunks
  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[kOtaBufferSize];
  int totalRead = 0;

  while (http.connected() && totalRead < totalSize) {
    // Feed watchdog to prevent 30s timeout reset during long downloads
    PoolController::SystemMonitor::feedWatchdog();

    size_t available = stream->available();
    if (available == 0) {
      delay(1);
      continue;
    }
    size_t toRead = min(available, sizeof(buffer));
    size_t read = stream->readBytes(buffer, toRead);
    if (read == 0) {
      delay(10);
      continue;
    }

    size_t written = Update.write(buffer, read);
    if (written != read) {
      LOG_ERROR("OTA: Write error at byte %d: %s\n", totalRead, Update.errorString());
      Update.end(false);
      http.end();
      return false;
    }

    totalRead += read;
    progress_ = (totalRead * 100) / totalSize;
    statusMessage_ = "Downloading... " + String(progress_) + "%";
  }

  http.end();

  if (totalRead != totalSize) {
    LOG_ERROR("OTA: Incomplete download (%d / %d)\n", totalRead, totalSize);
    Update.end(false);
    return false;
  }

  if (!Update.end(true)) {
    LOG_ERROR("OTA: Update.end() failed: %s\n", Update.errorString());
    return false;
  }

  LOG_INFO("OTA: Update successful! Rebooting...\n");
  statusMessage_ = "Update successful! Rebooting...";
  Serial.flush();
  NetworkManager::restart();
  return true;  // Never actually reached
}

// ── Space and Size Verification ──

size_t OtaUpdater::getAvailableFlashSpace() {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  // On ESP32, get free sketch space
  return ESP.getFreeSketchSpace();
#elif defined(ESP8266)
  // On ESP8266, use the Update library's available space
  return ESP.getFreeSketchSpace();
#else
  // For native tests or other platforms, return a large value
  return 4UL * 1024 * 1024;  // 4MB (typical ESP32 flash size)
#endif
}

bool OtaUpdater::hasSufficientSpace(size_t firmwareSize) {
  size_t availableSpace = getAvailableFlashSpace();

  // Calculate required space: firmware size + safety margin
  size_t requiredSpace = firmwareSize + static_cast<size_t>(firmwareSize * kSpaceSafetyMargin);

  // Also ensure we have at least minimum free space
  requiredSpace = std::max(requiredSpace, kMinFreeSpace);

  if (availableSpace < requiredSpace) {
    LOG_ERROR("OTA: Insufficient flash space. Need %u bytes, have %u bytes\n", requiredSpace, availableSpace);
    statusMessage_ = "Error: Insufficient flash space";
    return false;
  }

  LOG_INFO("OTA: Sufficient space available (%u bytes free, %u bytes required)\n", availableSpace, requiredSpace);
  return true;
}

}  // namespace PoolController
