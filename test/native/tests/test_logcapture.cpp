/**
 * @file test_logcapture.cpp
 * @brief Unit tests for LogCapture ring buffer — wrapping, seq, filters, clear, truncation.
 */

#include <stdio.h>
#include <string.h>

// Mock includes (picked up via -I mocks/)
#include "Arduino.h"
#include "LogCapture.hpp"

// Test framework
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

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

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

using PoolController::LogCapture;
using PoolController::LogEntry;
using PoolController::LogLevel;

int run_logcapture_tests() {
  int passed = 0, failed = 0;
  int rc;

  // Keep test output clean — mirror goes to stdout via the mock Serial.
  LogCapture::setLogToSerial(false);

  // ── Test: ring wraparound keeps newest entries ──
  {
    test_begin("LogCapture", "wraparound keeps newest entries");
    LogCapture::begin();
    const size_t N = LogCapture::LOG_BUFFER_ENTRIES + 5;
    for (size_t i = 0; i < N; ++i) {
      LogCapture::log(LogLevel::Info, "entry %zu", i);
    }
    LogEntry entries[LogCapture::LOG_BUFFER_ENTRIES];
    size_t got = LogCapture::getEntries(0, LogCapture::epoch(), 4096, LogLevel::Debug, entries, LogCapture::LOG_BUFFER_ENTRIES);
    rc = (got == LogCapture::LOG_BUFFER_ENTRIES) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected %u entries, got %u", (unsigned)LogCapture::LOG_BUFFER_ENTRIES, (unsigned)got);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::wraparound", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);

    test_begin("LogCapture", "wraparound oldest seq > 0");
    rc = (entries[0].seq > 0) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Oldest kept entry should have seq > 0");
      failed++;
    }
    test_suite_end("LogCapture::wraparound_oldest", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: seq monotonicity ──
  {
    test_begin("LogCapture", "lastSeq equals number of logs");
    LogCapture::begin();
    const size_t N = 5;
    for (size_t i = 0; i < N; ++i) {
      LogCapture::log(LogLevel::Info, "log %zu", i);
    }
    rc = (LogCapture::lastSeq() == N) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected lastSeq %u, got %u", (unsigned)N, (unsigned)LogCapture::lastSeq());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::lastseq", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);

    test_begin("LogCapture", "entries have strictly increasing seq");
    LogEntry entries[N];
    size_t got = LogCapture::getEntries(0, LogCapture::epoch(), N, LogLevel::Debug, entries, N);
    bool increasing = (got == N);
    for (size_t i = 1; increasing && i < got; ++i) {
      increasing = (entries[i].seq > entries[i - 1].seq);
    }
    rc = increasing ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "Entries must have strictly increasing seq");
      failed++;
    }
    test_suite_end("LogCapture::seq_increasing", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: since-filter returns only seq > sinceSeq ──
  {
    test_begin("LogCapture", "since filter returns only newer entries");
    LogCapture::begin();
    for (size_t i = 0; i < 5; ++i) {
      LogCapture::log(LogLevel::Info, "log %zu", i);
    }
    LogEntry entries[8];
    size_t got = LogCapture::getEntries(3, LogCapture::epoch(), 8, LogLevel::Debug, entries, 8);
    rc = (got == 2 && entries[0].seq == 4 && entries[1].seq == 5) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 2 entries seq 4..5, got %u", (unsigned)got);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::since", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: stale cursor from before a reboot is clamped to 0 ──
  {
    test_begin("LogCapture", "stale pre-reboot cursor returns whole ring");
    LogCapture::begin();
    for (size_t i = 0; i < 5; ++i) {
      LogCapture::log(LogLevel::Info, "boot %zu", i);
    }
    // begin() restarts the sequence at 0, so a cursor persisted across the
    // reboot (higher than every new seq) must not suppress the whole ring.
    LogEntry entries[8];
    size_t got = LogCapture::getEntries(1000, LogCapture::epoch(), 8, LogLevel::Debug, entries, 8);
    rc = (got == 5 && entries[0].seq == 1 && entries[4].seq == 5) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 5 entries seq 1..5, got %u", (unsigned)got);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::stale_cursor", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: level filter ──
  {
    test_begin("LogCapture", "level filter >= Warning");
    LogCapture::begin();
    LogCapture::log(LogLevel::Debug, "d");
    LogCapture::log(LogLevel::Info, "i");
    LogCapture::log(LogLevel::Warning, "w");
    LogCapture::log(LogLevel::Error, "e");
    LogEntry entries[8];
    size_t got = LogCapture::getEntries(0, LogCapture::epoch(), 8, LogLevel::Warning, entries, 8);
    rc = (got == 2 && entries[0].level == LogLevel::Warning && entries[1].level == LogLevel::Error) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 2 warn/error entries, got %u", (unsigned)got);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::level", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: clear hides entries but keeps seq ──
  {
    test_begin("LogCapture", "clear empties ring");
    LogCapture::begin();
    LogCapture::log(LogLevel::Info, "a");
    LogCapture::log(LogLevel::Info, "b");
    LogCapture::log(LogLevel::Info, "c");
    LogCapture::clear();
    LogEntry entries[8];
    size_t got = LogCapture::getEntries(0, LogCapture::epoch(), 8, LogLevel::Debug, entries, 8);
    rc = (got == 0) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 0 entries after clear, got %u", (unsigned)got);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::clear_empty", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);

    test_begin("LogCapture", "clear keeps lastSeq");
    rc = (LogCapture::lastSeq() == 3) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected lastSeq 3 after clear, got %u", (unsigned)LogCapture::lastSeq());
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::clear_seq", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);

    test_begin("LogCapture", "post-clear entries continue above watermark");
    LogCapture::log(LogLevel::Info, "d");
    size_t got2 = LogCapture::getEntries(0, LogCapture::epoch(), 8, LogLevel::Debug, entries, 8);
    rc = (got2 == 1 && entries[0].seq == 4) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 1 entry seq 4 after clear, got %u", (unsigned)got2);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::clear_watermark", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: truncation of oversized messages ──
  {
    test_begin("LogCapture", "long message is truncated safely");
    LogCapture::begin();
    char longMsg[LOG_MSG_SIZE * 2];
    memset(longMsg, 'A', sizeof(longMsg) - 1);
    longMsg[sizeof(longMsg) - 1] = '\0';
    LogCapture::log(LogLevel::Info, "%s", longMsg);
    LogEntry entries[2];
    size_t got = LogCapture::getEntries(0, LogCapture::epoch(), 2, LogLevel::Debug, entries, 2);
    size_t len = strnlen(entries[0].message, sizeof(entries[0].message));
    rc = (got == 1 && len == LOG_MSG_SIZE - 1 && entries[0].message[len] == '\0') ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected truncated len %d, got %u", LOG_MSG_SIZE - 1, (unsigned)len);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::truncation", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: Serial mirror keeps the full message even when the ring truncates ──
  {
    test_begin("LogCapture", "serial mirror keeps full message when ring truncates");
    LogCapture::begin();
    LogCapture::setLogToSerial(true);
    SerialClass::enableCapture();

    // Message longer than a ring slot: the ring copy must be truncated to
    // LOG_MSG_SIZE-1, but the Serial mirror must receive the full text.
    char longMsg[LOG_MSG_SIZE * 3];
    memset(longMsg, 'C', sizeof(longMsg) - 1);
    longMsg[sizeof(longMsg) - 1] = '\0';
    LogCapture::log(LogLevel::Info, "%s", longMsg);

    const std::string serialOut = SerialClass::capture();
    SerialClass::disableCapture();
    LogCapture::setLogToSerial(false);

    LogEntry entries[2];
    size_t got = LogCapture::getEntries(0, LogCapture::epoch(), 2, LogLevel::Debug, entries, 2);
    size_t ringLen = strnlen(entries[0].message, sizeof(entries[0].message));

    rc = (got == 1 && ringLen == LOG_MSG_SIZE - 1 && serialOut.size() == sizeof(longMsg) - 1 &&
           memcmp(serialOut.data(), longMsg, sizeof(longMsg) - 1) == 0)
      ? 0
      : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[128];
      snprintf(msg, sizeof(msg), "ring len %u (want %d), serial len %zu (want %zu)", (unsigned)ringLen, LOG_MSG_SIZE - 1,
        serialOut.size(), sizeof(longMsg) - 1);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::serial_full_mirror", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: logEvent prefix marker ──
  {
    test_begin("LogCapture", "logEvent prefixes event type");
    LogCapture::begin();
    LogCapture::logEvent("MODE_CHANGED", "to auto");
    LogEntry entries[2];
    size_t got = LogCapture::getEntries(0, LogCapture::epoch(), 2, LogLevel::Debug, entries, 2);
    bool hasPrefix = (got == 1 && entries[0].message[0] == '[' && strstr(entries[0].message, "MODE_CHANGED") != nullptr);
    rc = hasPrefix ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected '[MODE_CHANGED] ...', got '%s'", got == 1 ? entries[0].message : "(none)");
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::logevent", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: parseLevel ──
  {
    test_begin("LogCapture", "parseLevel case-insensitive");
    rc = (LogCapture::parseLevel("warning") == LogLevel::Warning && LogCapture::parseLevel("ERROR") == LogLevel::Error &&
           LogCapture::parseLevel("Debug") == LogLevel::Debug && LogCapture::parseLevel("bogus") == LogLevel::Info)
      ? 0
      : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "parseLevel mapping wrong");
      failed++;
    }
    test_suite_end("LogCapture::parselevel", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  // ── Test: reboot epoch distinguishes stale cursors ──
  {
    test_begin("LogCapture", "boot epoch changes across reboots");
    LogCapture::begin();
    const uint32_t e1 = LogCapture::epoch();
    LogCapture::begin();
    const uint32_t e2 = LogCapture::epoch();
    rc = (e2 != e1) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      test_fail(__FILE__, __LINE__, "epoch must change after begin()");
      failed++;
    }
    test_suite_end("LogCapture::reboot_epoch::changes", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  {
    test_begin("LogCapture", "stale pre-reboot cursor with new seq <= since returns whole ring");
    // Scenario from the review: boot 1 fills seq 1..20, client cursor since=20.
    // Reboot (begin) restarts seq at 0; by the first poll the new boot already
    // reached s_seq == 40. The old seq-only clamp (sinceSeq > s_seq) would NOT
    // trigger (20 <= 40) and silently skip new-boot entries 1..20. The epoch
    // mismatch must force a full re-read instead.
    LogCapture::begin();
    const uint32_t boot1 = LogCapture::epoch();
    for (uint32_t i = 0; i < 20; ++i) {
      LogCapture::log(LogLevel::Info, "boot1 %u", i);
    }
    LogCapture::begin();
    for (uint32_t i = 0; i < 40; ++i) {
      LogCapture::log(LogLevel::Info, "boot2 %u", i);
    }
    LogEntry entries[64];
    size_t got = LogCapture::getEntries(20, boot1, 64, LogLevel::Debug, entries, 64);
    rc = (got == 40 && entries[0].seq == 1 && entries[39].seq == 40) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 40 entries seq 1..40, got %u", (unsigned)got);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::reboot_epoch::stale_cursor", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  {
    test_begin("LogCapture", "current-boot cursor still filters by since");
    // Same ring as above, but the client now echoes the CURRENT epoch: the
    // cursor since=20 is trusted and only entries 21..40 are returned.
    LogCapture::begin();
    for (uint32_t i = 0; i < 40; ++i) {
      LogCapture::log(LogLevel::Info, "boot %u", i);
    }
    LogEntry entries[64];
    size_t got = LogCapture::getEntries(20, LogCapture::epoch(), 64, LogLevel::Debug, entries, 64);
    rc = (got == 20 && entries[0].seq == 21 && entries[19].seq == 40) ? 0 : 1;
    if (rc == 0) {
      test_pass(__FILE__, __LINE__);
      passed++;
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected 20 entries seq 21..40, got %u", (unsigned)got);
      test_fail(__FILE__, __LINE__, msg);
      failed++;
    }
    test_suite_end("LogCapture::reboot_epoch::current_cursor", rc == 0 ? 1 : 0, rc != 0 ? 1 : 0);
  }

  return passed + failed;
}
