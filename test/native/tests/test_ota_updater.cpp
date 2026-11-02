// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

#include <cstring>
#include <string>

#include "HTTPClient.h"
#include "NetworkManager.hpp"
#include "../../../src/OtaUpdater.hpp"
#include "test_assert.h"

using namespace PoolController;

// Minimal GitHub release payload with the board-specific asset.
static void setReleaseResponse(const char *tag, const char *assetUrl, const char *assetName) {
  httpCapture.clear();
  mockStreamState.reset();
  httpCapture.httpCode = 200;
  httpCapture.responseBody = std::string("{\"tag_name\":\"") + tag +
    "\",\"html_url\":\"https://github.com/r/releases\","
    "\"assets\":[{\"name\":\"" +
    assetName + "\",\"browser_download_url\":\"" + assetUrl + "\"}]}";
  httpCapture.contentSize = static_cast<int>(httpCapture.responseBody.size());
}

static void setUpdateAvailableState() {
  // Seed a newer release so startUpdate() has a download URL.
  setReleaseResponse("v999.0.0", "https://example.com/firmware.bin", "firmware-norvi_ae01_r.bin");
  OtaUpdater::begin();
  httpCapture.httpCode = 200;
  OtaUpdater::checkForUpdate();
}

int run_ota_updater_tests() {
  int passed = 0;
  int failed = 0;
  int rc = 0;

  test_begin("OtaUpdater", "begin() reports the current version and idle status");
  OtaUpdater::begin();
  ASSERT_EQ(OtaUpdater::getCurrentVersion(), String(FW_VERSION));
  ASSERT_EQ(OtaUpdater::getStatusMessage(), String("Idle"));
  test_suite_end("OtaUpdater::begin", 1, 0);

  test_begin("OtaUpdater", "parseVersion accepts v-prefixed and plain semver");
  {
    OtaUpdater::Version v{};
    ASSERT_TRUE(OtaUpdater::parseVersion(String("v3.2.1"), v));
    ASSERT_EQ(v.major, 3);
    ASSERT_EQ(v.minor, 2);
    ASSERT_EQ(v.patch, 1);
    ASSERT_TRUE(OtaUpdater::parseVersion(String("1.0.0"), v));
    ASSERT_EQ(v.major, 1);
  }
  test_suite_end("OtaUpdater::parse-version", 1, 0);

  test_begin("OtaUpdater", "parseVersion rejects incomplete or malformed input");
  {
    OtaUpdater::Version v{};
    ASSERT_TRUE(!OtaUpdater::parseVersion(String("3.2"), v));
    ASSERT_TRUE(!OtaUpdater::parseVersion(String("abc"), v));
    ASSERT_TRUE(!OtaUpdater::parseVersion(String(""), v));
  }
  test_suite_end("OtaUpdater::parse-invalid", 1, 0);

  test_begin("OtaUpdater", "isNewerVersion compares major, minor and patch correctly");
  {
    ASSERT_TRUE(OtaUpdater::isNewerVersion(String("v1.0.0"), String("v1.0.1")));
    ASSERT_TRUE(OtaUpdater::isNewerVersion(String("v1.0.0"), String("v1.1.0")));
    ASSERT_TRUE(OtaUpdater::isNewerVersion(String("v1.9.9"), String("v2.0.0")));
    ASSERT_TRUE(!OtaUpdater::isNewerVersion(String("v2.0.0"), String("v1.9.9")));
    ASSERT_TRUE(!OtaUpdater::isNewerVersion(String("v1.2.3"), String("v1.2.3")));
  }
  test_suite_end("OtaUpdater::is-newer", 1, 0);

  test_begin("OtaUpdater", "isNewerVersion rejects unparseable input");
  {
    ASSERT_TRUE(!OtaUpdater::isNewerVersion(String("v1.0.0"), String("oops")));
    ASSERT_TRUE(!OtaUpdater::isNewerVersion(String("oops"), String("v1.0.0")));
  }
  test_suite_end("OtaUpdater::is-newer-invalid", 1, 0);

  test_begin("OtaUpdater", "checkForUpdate detects a newer GitHub release");
  {
    setReleaseResponse("v999.0.0", "https://example.com/firmware.bin", "firmware-norvi_ae01_r.bin");
    OtaUpdater::begin();
    ASSERT_TRUE(OtaUpdater::checkForUpdate());
    ASSERT_TRUE(OtaUpdater::isUpdateAvailable());
    ASSERT_EQ(OtaUpdater::getLatestVersion(), String("999.0.0"));
    ASSERT_EQ(OtaUpdater::getLatestVersionTag(), String("v999.0.0"));
    ASSERT_EQ(OtaUpdater::getReleaseUrl(), String("https://github.com/r/releases"));
  }
  test_suite_end("OtaUpdater::check-newer", 1, 0);

  test_begin("OtaUpdater", "checkForUpdate reports up-to-date on same version");
  {
    setReleaseResponse(FW_VERSION, "https://example.com/firmware.bin", "firmware-norvi_ae01_r.bin");
    OtaUpdater::begin();
    ASSERT_TRUE(!OtaUpdater::checkForUpdate());
    ASSERT_TRUE(!OtaUpdater::isUpdateAvailable());
  }
  test_suite_end("OtaUpdater::check-current", 1, 0);

  test_begin("OtaUpdater", "checkForUpdate fails on HTTP error");
  {
    httpCapture.clear();
    mockStreamState.reset();
    httpCapture.httpCode = 500;
    OtaUpdater::begin();
    ASSERT_TRUE(!OtaUpdater::checkForUpdate());
    ASSERT_TRUE(!OtaUpdater::isUpdateAvailable());
  }
  test_suite_end("OtaUpdater::check-http-error", 1, 0);

  test_begin("OtaUpdater", "checkForUpdate fails on missing tag_name");
  {
    httpCapture.clear();
    mockStreamState.reset();
    httpCapture.httpCode = 200;
    httpCapture.responseBody = "{\"html_url\":\"https://github.com/r/releases\",\"assets\":[]}";
    httpCapture.contentSize = static_cast<int>(httpCapture.responseBody.size());
    OtaUpdater::begin();
    ASSERT_TRUE(!OtaUpdater::checkForUpdate());
  }
  test_suite_end("OtaUpdater::check-no-tag", 1, 0);

  test_begin("OtaUpdater", "checkForUpdate fails when no .bin asset exists");
  {
    setReleaseResponse("v999.0.0", "https://example.com/readme.txt", "readme.txt");
    OtaUpdater::begin();
    ASSERT_TRUE(!OtaUpdater::checkForUpdate());
  }
  test_suite_end("OtaUpdater::check-no-asset", 1, 0);

  test_begin("OtaUpdater", "checkForUpdate falls back to a generic .bin asset");
  {
    setReleaseResponse("v999.0.0", "https://example.com/firmware.bin", "firmware.bin");
    OtaUpdater::begin();
    ASSERT_TRUE(OtaUpdater::checkForUpdate());
  }
  test_suite_end("OtaUpdater::check-generic-asset", 1, 0);

  test_begin("OtaUpdater", "startUpdate is refused without a prepared release");
  {
    // Bring the OTA state back to "up to date" so no download URL is known.
    setReleaseResponse(FW_VERSION, "https://example.com/firmware.bin", "firmware-norvi_ae01_r.bin");
    OtaUpdater::begin();
    OtaUpdater::checkForUpdate();
    ASSERT_TRUE(!OtaUpdater::isUpdateAvailable());
    ASSERT_TRUE(!OtaUpdater::startUpdate());
    ASSERT_EQ(OtaUpdater::getStatusMessage(), String("No update available"));
  }
  test_suite_end("OtaUpdater::start-no-update", 1, 0);

  test_begin("OtaUpdater", "startUpdate is refused without WiFi");
  {
    setUpdateAvailableState();
    NetworkManager::setWiFiConnected(false);
    ASSERT_TRUE(!OtaUpdater::startUpdate());
    ASSERT_EQ(OtaUpdater::getStatusMessage(), String("No WiFi connection"));
    NetworkManager::setWiFiConnected(true);
  }
  test_suite_end("OtaUpdater::start-no-wifi", 1, 0);

  test_begin("OtaUpdater", "hasSufficientSpace validates firmware size against flash");
  {
    ASSERT_TRUE(OtaUpdater::hasSufficientSpace(600 * 1024));
    ASSERT_TRUE(!OtaUpdater::hasSufficientSpace(64UL * 1024 * 1024));
  }
  test_suite_end("OtaUpdater::space", 1, 0);

  test_begin("OtaUpdater", "startUpdate rejects failed downloads with a specific status message");
  {
    setUpdateAvailableState();
    NetworkManager::setWiFiConnected(true);
    // HTTP 404 during download.
    httpCapture.httpCode = 404;
    ASSERT_TRUE(!OtaUpdater::startUpdate());
    ASSERT_EQ(OtaUpdater::getStatusMessage(), String("Error: Download failed"));
    // Invalid (non-positive) content size.
    httpCapture.httpCode = 200;
    httpCapture.contentSize = 0;
    ASSERT_TRUE(!OtaUpdater::startUpdate());
    ASSERT_EQ(OtaUpdater::getStatusMessage(), String("Error: Invalid firmware size"));
    // Far too small to be a firmware image.
    httpCapture.contentSize = 512;
    ASSERT_TRUE(!OtaUpdater::startUpdate());
    ASSERT_EQ(OtaUpdater::getStatusMessage(), String("Error: Firmware too small"));
    // Far too large to be a firmware image.
    httpCapture.contentSize = 3 * 1024 * 1024;
    ASSERT_TRUE(!OtaUpdater::startUpdate());
    ASSERT_EQ(OtaUpdater::getStatusMessage(), String("Error: Firmware too large"));
    ASSERT_TRUE(!OtaUpdater::isUpdateInProgress());
  }
  test_suite_end("OtaUpdater::download-guards", 1, 0);

  test_begin("OtaUpdater", "startUpdate streams a plausible firmware into the flash writer");
  {
    setUpdateAvailableState();
    NetworkManager::setWiFiConnected(true);
    // Plausible firmware: 128 KB body, real content size.
    httpCapture.httpCode = 200;
    httpCapture.contentSize = 128 * 1024;
    httpCapture.responseBody.assign(4096, 'F');
    mockStreamState.reset();
    ASSERT_TRUE(OtaUpdater::startUpdate());
    // The body is streamed by loop(), one bounded step per iteration.
    for (int i = 0; i < 1000 && OtaUpdater::getProgress() < 100; ++i) {
      OtaUpdater::loop();
    }
    ASSERT_EQ(OtaUpdater::getProgress(), 100);
  }
  test_suite_end("OtaUpdater::download-happy-path", 1, 0);

  test_begin("OtaUpdater", "requestUpdate is started by loop() and ignored while an update runs");
  {
    setUpdateAvailableState();
    NetworkManager::setWiFiConnected(true);
    httpCapture.httpCode = 500;
    OtaUpdater::requestUpdate();
    ASSERT_TRUE(OtaUpdater::isUpdateRequested());
    ASSERT_TRUE(!OtaUpdater::isUpdateInProgress());
    OtaUpdater::loop();
    ASSERT_TRUE(!OtaUpdater::isUpdateRequested());
    ASSERT_TRUE(!OtaUpdater::isUpdateInProgress());
    ASSERT_EQ(OtaUpdater::getStatusMessage(), String("Error: Download failed"));

    // A second request while a download streams must not queue a restart.
    httpCapture.httpCode = 200;
    httpCapture.contentSize = 128 * 1024;
    httpCapture.responseBody.assign(4096, 'F');
    mockStreamState.reset();
    ASSERT_TRUE(OtaUpdater::startUpdate());
    OtaUpdater::requestUpdate();
    ASSERT_TRUE(OtaUpdater::isUpdateInProgress());
    ASSERT_TRUE(!OtaUpdater::isUpdateRequested());
    for (int i = 0; i < 1000 && OtaUpdater::getProgress() < 100; ++i) {
      OtaUpdater::loop();
    }
    ASSERT_EQ(OtaUpdater::getProgress(), 100);
    OtaUpdater::loop();  // nothing left to stream before the reboot
  }
  test_suite_end("OtaUpdater::request-update", 1, 0);

  test_begin("OtaUpdater", "progress and status expose the OTA lifecycle");
  {
    OtaUpdater::begin();
    const String current = OtaUpdater::getCurrentVersion();
    ASSERT_TRUE(!current.isEmpty());
    // After a successful flash the device would reboot; in the mock the
    // in-progress flag stays set until that restart.
    const int progress = OtaUpdater::getProgress();
    ASSERT_TRUE(progress == 0 || progress == 100);
  }
  test_suite_end("OtaUpdater::lifecycle", 1, 0);

  (void)rc;
  (void)passed;
  (void)failed;
  return 0;
}
