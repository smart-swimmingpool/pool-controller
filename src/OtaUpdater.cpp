// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "OtaUpdater.hpp"
#include "Version.h"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>

namespace PoolController {

// ISRG Root X1 — Let's Encrypt root CA used by GitHub
// https://letsencrypt.org/certificates/
static const char kGitHubRootCA[] PROGMEM =
  "-----BEGIN CERTIFICATE-----\n"
  "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\n"
  "UxM=\n"
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

// ── Public API ──

void OtaUpdater::begin() {
  Serial.printf("✓ OTA Updater initialized (current: %s)\n", currentVersion_.c_str());
  statusMessage_ = "Idle";
}

void OtaUpdater::loop() {
  // Periodic check when WiFi is connected and no update is in progress
  if (NetworkManager::isWiFiConnected() && !updateInProgress_) {
    unsigned long now = millis();
    // Handle wrap-around
    if (now - lastCheckTime_ >= kCheckIntervalMs || lastCheckTime_ == 0) {
      lastCheckTime_ = now;
      checkForUpdate();
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

  // ── OTA Safety: backup config before flashing ──
  Serial.println("OTA: Backing up configuration before update...");
  ConfigManager::backupConfig();

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

  WiFiClientSecure client;
  client.setInsecure();  // Accept any cert (sufficient for IoT device)
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
  client.setCACert(kGitHubRootCA);
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

}  // namespace PoolController
