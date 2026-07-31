/**
 * @file test_webportal_logs.cpp
 * @brief Tests for WebPortal::buildLogsJson — the /api/logs JSON builder hook.
 *
 * buildLogsJson is a public static test hook (pattern: "Rate limiting
 * helpers (public for testing)") that serializes LogCapture entries into
 * the JSON payload served by apiGetLogs. These tests verify the payload
 * shape and the since/count/level filters without needing the WebServer.
 */

#include <stdio.h>
#include <string.h>
#include <string>
#include "Arduino.h"
#include "ArduinoJson.h"

// Mock WebPortal dependencies
#include "WebServer.h"
#include "DNSServer.h"
#include "WebPortal.hpp"
#include "LogCapture.hpp"

using namespace PoolController;  // NOLINT(build/namespaces)

// Global test helpers
extern WebServerCapture wsCapture;

extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_TRUE(cond)                                     \
  do {                                                        \
    if (!(cond)) {                                            \
      test_fail(__FILE__, __LINE__, "Expected true: " #cond); \
      return 1;                                               \
    }                                                         \
    test_pass(__FILE__, __LINE__);                            \
  } while (0)

#define ASSERT_EQ(a, b)                                                                                          \
  do {                                                                                                           \
    auto _a = (a);                                                                                               \
    auto _b = (b);                                                                                               \
    if (_a != _b) {                                                                                              \
      char _msg[256];                                                                                            \
      snprintf(_msg, sizeof(_msg), "Expected %s == %s: got %lld vs %lld", #a, #b, (long long)_a, (long long)_b); \
      test_fail(__FILE__, __LINE__, _msg);                                                                       \
      return 1;                                                                                                  \
    }                                                                                                            \
    test_pass(__FILE__, __LINE__);                                                                               \
  } while (0)

namespace {

// Serializes via the hook and parses the result. Returns true on success.
bool buildAndParse(uint32_t since, size_t count, LogLevel minLevel,
                   JsonDocument &doc, size_t &payloadLen) {
  char buf[8192];
  payloadLen = WebPortal::buildLogsJson(since, count, minLevel, buf, sizeof(buf));
  if (payloadLen == 0) {
    return false;
  }
  DeserializationError err = deserializeJson(doc, buf);
  return err == DeserializationError::Ok;
}

}  // namespace

int run_webportal_logs_tests() {
  int passed = 0, failed = 0;
  int rc;

  // Keep test output clean — no Serial mirror.
  LogCapture::setLogToSerial(false);

  // ── Test: full dump — ok, next, entries with all fields ──
  {
    test_begin("WebPortal::buildLogsJson", "full dump has ok, next, entries");
    LogCapture::begin();
    LogCapture::log(LogLevel::Info, "boot msg %d", 1);
    LogCapture::log(LogLevel::Info, "boot msg %d", 2);

    JsonDocument doc;
    size_t len = 0;
    rc = buildAndParse(0, 200, LogLevel::Debug, doc, len) ? 0 : 1;
    if (rc == 0) {
      rc = (doc["ok"] == true && doc["next"] == 3) ? 0 : 1;
    }
    if (rc == 0) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      rc = (arr.size() == 2) ? 0 : 1;
    }
    if (rc == 0) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      rc = (arr[0]["seq"] == 1 && arr[0]["level"] == "info" &&
            strcmp(arr[0]["msg"], "boot msg 1") == 0 && arr[0]["t"].is<uint32_t>()) ? 0 : 1;
    }
    if (rc == 0) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      rc = (arr[1]["seq"] == 2 && strcmp(arr[1]["msg"], "boot msg 2") == 0) ? 0 : 1;
    }
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "buildLogsJson full dump malformed");
      failed++;
    }
    test_suite_end("WebPortal::buildLogsJson::full", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: since filter returns only newer entries ──
  {
    test_begin("WebPortal::buildLogsJson", "since filter returns newer entries");
    LogCapture::begin();
    for (int i = 1; i <= 5; ++i) {
      LogCapture::log(LogLevel::Info, "msg %d", i);
    }

    JsonDocument doc;
    size_t len = 0;
    rc = buildAndParse(3, 200, LogLevel::Debug, doc, len) ? 0 : 1;
    if (rc == 0) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      rc = (arr.size() == 2 && arr[0]["seq"] == 4 && arr[1]["seq"] == 5) ? 0 : 1;
    }
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "since=3 should yield seq 4 and 5");
      failed++;
    }
    test_suite_end("WebPortal::buildLogsJson::since", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: count cap limits entry count ──
  {
    test_begin("WebPortal::buildLogsJson", "count cap limits entries");
    LogCapture::begin();
    for (int i = 1; i <= 5; ++i) {
      LogCapture::log(LogLevel::Info, "msg %d", i);
    }

    JsonDocument doc;
    size_t len = 0;
    rc = buildAndParse(0, 2, LogLevel::Debug, doc, len) ? 0 : 1;
    if (rc == 0) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      rc = (arr.size() == 2 && arr[0]["seq"] == 1 && arr[1]["seq"] == 2) ? 0 : 1;
    }
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "count=2 should yield seq 1 and 2");
      failed++;
    }
    test_suite_end("WebPortal::buildLogsJson::count", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: level filter excludes lower severities ──
  {
    test_begin("WebPortal::buildLogsJson", "level filter excludes lower severities");
    LogCapture::begin();
    LogCapture::log(LogLevel::Debug, "debug line");
    LogCapture::log(LogLevel::Info, "info line");
    LogCapture::log(LogLevel::Warning, "warn line");
    LogCapture::log(LogLevel::Error, "error line");

    JsonDocument doc;
    size_t len = 0;
    rc = buildAndParse(0, 200, LogLevel::Warning, doc, len) ? 0 : 1;
    if (rc == 0) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      rc = (arr.size() == 2 && arr[0]["level"] == "warning" && arr[1]["level"] == "error") ? 0 : 1;
    }
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "level=warning should yield warning and error only");
      failed++;
    }
    test_suite_end("WebPortal::buildLogsJson::level", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: empty result yields entries: [] ──
  {
    test_begin("WebPortal::buildLogsJson", "empty result yields empty entries");
    LogCapture::begin();
    LogCapture::log(LogLevel::Info, "a");
    LogCapture::log(LogLevel::Info, "b");
    LogCapture::clear();

    JsonDocument doc;
    size_t len = 0;
    rc = buildAndParse(0, 200, LogLevel::Debug, doc, len) ? 0 : 1;
    if (rc == 0) {
      JsonArray arr = doc["entries"].as<JsonArray>();
      rc = (arr.size() == 0 && doc["ok"] == true && doc["next"] == 3) ? 0 : 1;
    }
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "after clear entries should be empty, next stays lastSeq+1");
      failed++;
    }
    test_suite_end("WebPortal::buildLogsJson::empty", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  return passed + failed;
}
