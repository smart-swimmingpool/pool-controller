// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

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
#include "TimeClientHelper.hpp"

namespace PoolController {

// ISRG Root X1 — Let's Encrypt root CA used by GitHub
// https://letsencrypt.org/certificates/
// ISRG Root X1 — Let's Encrypt root CA used by GitHub
// https://letsencrypt.org/certificates/
// SHA256: 96:bcec:0626:4976:f374:6077:9acf:28c5:a7cf:e8a3:c0aa:e11a:8ffe:ce05:c0bd:df08:c6
static const char kGitHubRootCA[] PROGMEM = "-----BEGIN CERTIFICATE-----\n"
                                            "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
                                            "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
                                            "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
                                            "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
                                            "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
                                            "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
                                            "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
                                            "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
                                            "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
                                            "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
                                            "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
                                            "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
                                            "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
                                            "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
                                            "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
                                            "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
                                            "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
                                            "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
                                            "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
                                            "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
                                            "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
                                            "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
                                            "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
                                            "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
                                            "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
                                            "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
                                            "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
                                            "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
                                            "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
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
  Serial.printf("✓ OTA Updater initialized (current: %s)\n", currentVersion_.c_str());
  statusMessage_ = "Idle";
}

void OtaUpdater::loop() {
  // Periodic check when WiFi is connected and no update is in progress
  if (NetworkManager::isWiFiConnected() && !updateInProgress_) {
    unsigned long now = millis();

    // Check if we're in clock sync backoff period
    if (clockSyncFailCount_ > 0) {
      // Handle unsigned wrap-around for backoff timer
      if (now - lastClockSyncFailTime_ >= kClockSyncBackoffMs || now < lastClockSyncFailTime_) {
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
  Serial.println("OTA: Checking for firmware update...");

  if (!fetchLatestRelease()) {
    statusMessage_ = "Check failed";
    Serial.println("OTA: Update check failed");
    return false;
  }

  // Compare versions
  if (isNewerVersion(currentVersion_, latestVersion_)) {
    updateAvailable_ = true;
    statusMessage_ = "Update available: v" + getLatestVersion();
    Serial.printf("OTA: New version available: %s (current: %s)\n", latestVersion_.c_str(), currentVersion_.c_str());
    return true;
  }

  updateAvailable_ = false;
  statusMessage_ = "Up to date (v" + currentVersion_ + ")";
  Serial.println("OTA: Firmware is up to date");
  return false;
}

// ── Start Update ──

bool OtaUpdater::startUpdate() {
  if (updateInProgress_) {
    Serial.println("OTA: Update already in progress");
    return false;
  }
  if (!updateAvailable_ || downloadUrl_.length() == 0) {
    statusMessage_ = "No update available";
    Serial.println("OTA: No update package available");
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
    Serial.println("OTA: Cannot start update: insufficient flash space");
    return false;
  }

  // Config persists in NVS — survives OTA natively, no backup needed
  updateInProgress_ = true;
  // Don't clear updateAvailable_ yet — preserved so UI can retry on failure (P2 review fix)
  progress_ = 0;
  statusMessage_ = "Downloading... 0%";

  Serial.printf("OTA: Starting download from %s\n", downloadUrl_.c_str());

  bool ok = downloadAndApply(downloadUrl_);
  if (!ok) {
    updateInProgress_ = false;
    statusMessage_ = "Update failed!";
    Serial.println("OTA: Update failed!");
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
    Serial.println("OTA: Syncing time before GitHub TLS verification...");
    // Ensure NTP client is set up
    timeClientSetup(ConfigManager::getNtp().server.c_str());
    // Sync system clock from NTP (sets settimeofday for mbedTLS)
    syncSystemClock();
    // Wait for sync to complete
    delay(2000);
    now = time(nullptr);
    if (now < 100000) {
      Serial.println("OTA: Time sync failed, TLS verification may fail");
      // Continue anyway - the TLS verification might still work
      // if the device has a valid time from a previous sync
    } else {
      Serial.printf("OTA: Time synced: %ld\n", now);
    }
  }

  WiFiClientSecure client;
  // Use CA certificate validation for GitHub TLS
  // Try to use ESP32's built-in CA bundle if available (includes ~130 root CAs)
  // This covers GitHub's CDN which may use various CA chains (Let's Encrypt, Sectigo, etc.)
  // Per openspec/specs/github-ca-chain.spec.md requirement R2
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
// Check if x509_crt_bundle and setCACertBundle are available (ESP32 Arduino core >= 2.0.0)
#if defined(x509_crt_bundle) && defined(ESP32_WiFiClientSecure_setCACertBundle)
  client.setCACertBundle(x509_crt_bundle);
#else
  // Fallback to single root CA for older ESP32 cores
  client.setCACert(kGitHubRootCA);
#endif
#else
  // Fallback for non-ESP32 platforms - use single root CA
  client.setCACert(kGitHubRootCA);
#endif
  client.setTimeout(10000);

  // Build API URL
  String url = String("https://api.github.com/repos/") + GITHUB_REPO + "/releases/latest";
  Serial.printf("OTA: Fetching %s\n", url.c_str());

  HTTPClient http;
  http.begin(client, url);
  http.setUserAgent("PoolController/1.0");
  http.addHeader("Accept", "application/vnd.github+json");

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("OTA: GitHub API returned HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  // Parse response (typically < 2KB)
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err) {
    Serial.printf("OTA: JSON parse error: %s\n", err.c_str());
    return false;
  }

  // Extract tag name (e.g. "v3.2.0")
  const char *tag = doc["tag_name"];
  if (!tag || strlen(tag) == 0) {
    Serial.println("OTA: No tag_name in response");
    return false;
  }
  latestVersion_ = String(tag);

  // Release URL
  const char *htmlUrl = doc["html_url"];
  releaseUrl_ = htmlUrl ? String(htmlUrl) : "";

  // Find the firmware binary asset
  JsonArray assets = doc["assets"].as<JsonArray>();
  downloadUrl_ = "";
  for (JsonObject asset : assets) {
    const char *name = asset["name"];
    if (name && strstr(name, ".bin") != nullptr) {
      const char *url = asset["browser_download_url"];
      if (url) {
        downloadUrl_ = String(url);
        break;
      }
    }
  }

  if (downloadUrl_.length() == 0) {
    Serial.println("OTA: No firmware binary found in release assets");
    return false;
  }

  Serial.printf("OTA: Found %s → %s\n", latestVersion_.c_str(), downloadUrl_.c_str());
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
  // Use CA certificate validation for GitHub TLS
  // Try to use ESP32's built-in CA bundle if available (includes ~130 root CAs)
  // This covers GitHub's CDN which may use various CA chains (Let's Encrypt, Sectigo, etc.)
  // Per openspec/specs/github-ca-chain.spec.md requirement R2
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
// Check if x509_crt_bundle and setCACertBundle are available (ESP32 Arduino core >= 2.0.0)
#if defined(x509_crt_bundle) && defined(ESP32_WiFiClientSecure_setCACertBundle)
  client.setCACertBundle(x509_crt_bundle);
#else
  // Fallback to single root CA for older ESP32 cores
  client.setCACert(kGitHubRootCA);
#endif
#else
  // Fallback for non-ESP32 platforms - use single root CA
  client.setCACert(kGitHubRootCA);
#endif
  client.setTimeout(10000);

  HTTPClient http;
  http.begin(client, url);
  http.setUserAgent("PoolController/1.0");
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("OTA: Download returned HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  int totalSize = http.getSize();
  if (totalSize <= 0) {
    Serial.println("OTA: Invalid content size");
    http.end();
    return false;
  }

  // Verify firmware size is reasonable (prevents integer overflow and bad downloads)
  // Typical firmware sizes: 200KB - 2MB
  const size_t kMaxFirmwareSize = 2 * 1024 * 1024;  // 2MB maximum
  const size_t kMinFirmwareSize = 50 * 1024;        // 50KB minimum

  if (static_cast<size_t>(totalSize) > kMaxFirmwareSize) {
    Serial.printf("OTA: Firmware too large: %d bytes (max %u)\n", totalSize, kMaxFirmwareSize);
    statusMessage_ = "Error: Firmware too large";
    http.end();
    return false;
  }

  if (static_cast<size_t>(totalSize) < kMinFirmwareSize) {
    Serial.printf("OTA: Firmware too small: %d bytes (min %u)\n", totalSize, kMinFirmwareSize);
    statusMessage_ = "Error: Firmware too small";
    http.end();
    return false;
  }

  // Verify we have sufficient space for this specific firmware size
  if (!hasSufficientSpace(static_cast<size_t>(totalSize))) {
    Serial.println("OTA: Insufficient space for this firmware");
    http.end();
    return false;
  }

  Serial.printf("OTA: Download size: %d bytes\n", totalSize);

  if (!Update.begin(totalSize)) {
    Serial.printf("OTA: Update.begin() failed: %s\n", Update.errorString());
    http.end();
    return false;
  }

  // Stream download in chunks
  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[kOtaBufferSize];
  int totalRead = 0;

  while (http.connected() && totalRead < totalSize) {
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
      Serial.printf("OTA: Write error at byte %d: %s\n", totalRead, Update.errorString());
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
    Serial.printf("OTA: Incomplete download (%d / %d)\n", totalRead, totalSize);
    Update.end(false);
    return false;
  }

  if (!Update.end(true)) {
    Serial.printf("OTA: Update.end() failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("OTA: Update successful! Rebooting...");
  statusMessage_ = "Update successful! Rebooting...";
  Serial.flush();
  delay(1000);
  ESP.restart();
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
    Serial.printf("OTA: Insufficient flash space. Need %u bytes, have %u bytes\n", requiredSpace, availableSpace);
    statusMessage_ = "Error: Insufficient flash space";
    return false;
  }

  Serial.printf("OTA: Sufficient space available (%u bytes free, %u bytes required)\n", availableSpace, requiredSpace);
  return true;
}

}  // namespace PoolController
