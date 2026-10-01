// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_ota_download_session.cpp
 * @brief Tests for OtaDownloadSession — scheduling contract of the OTA
 *        download (#196): one loop() step moves at most one buffer and never
 *        waits for data, so a running update cannot monopolize the control loop.
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "OtaDownloadSession.hpp"

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

#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))

using PoolController::OtaDownloadSession;
using Result = PoolController::OtaDownloadSession::Result;

namespace {

constexpr uint32_t kStallMs = 30000;
constexpr uint32_t kTotalMs = 600000;
constexpr size_t kBufferSize = 4096;

// Stream with a configurable number of immediately available bytes
struct FakeStream {
  size_t availableBytes = 0;
  size_t readCalls = 0;
  int available() const { return static_cast<int>(availableBytes); }
  size_t readBytes(uint8_t *buffer, size_t len) {
    readCalls++;
    size_t n = len < availableBytes ? len : availableBytes;
    for (size_t i = 0; i < n; i++) {
      buffer[i] = static_cast<uint8_t>(i);
    }
    availableBytes -= n;
    return n;
  }
};

struct FakeSink {
  size_t written = 0;
  size_t largestWrite = 0;
  bool failWrites = false;
  size_t write(const uint8_t *, size_t len) {
    if (failWrites) {
      return 0;
    }
    written += len;
    if (len > largestWrite) {
      largestWrite = len;
    }
    return len;
  }
};

}  // namespace

static int test_step_moves_at_most_one_buffer() {
  test_begin("OtaDownloadSession", "one step moves at most one buffer");
  OtaDownloadSession session(0, 1024 * 1024, kStallMs, kTotalMs);
  FakeStream stream;
  stream.availableBytes = 1024 * 1024;  // whole firmware already buffered
  FakeSink sink;
  uint8_t buffer[kBufferSize];
  ASSERT_TRUE(session.step(stream, sink, true, 10, buffer, sizeof(buffer)) == Result::IN_PROGRESS);
  ASSERT_TRUE(session.bytesWritten() == kBufferSize);
  ASSERT_EQ(stream.readCalls, 1);
  return 0;
}

static int test_step_without_data_returns_immediately() {
  test_begin("OtaDownloadSession", "step without data does not read or wait");
  OtaDownloadSession session(0, 100000, kStallMs, kTotalMs);
  FakeStream stream;
  FakeSink sink;
  uint8_t buffer[kBufferSize];
  ASSERT_TRUE(session.step(stream, sink, true, 10, buffer, sizeof(buffer)) == Result::IN_PROGRESS);
  ASSERT_EQ(stream.readCalls, 0);
  ASSERT_EQ(sink.written, 0);
  return 0;
}

static int test_download_spread_over_loop_iterations() {
  test_begin("OtaDownloadSession", "1 MB firmware needs many bounded loop steps");
  const size_t total = 1024 * 1024;
  OtaDownloadSession session(0, total, kStallMs, kTotalMs);
  FakeStream stream;
  stream.availableBytes = total + 500;  // more than announced — must not over-read
  FakeSink sink;
  uint8_t buffer[kBufferSize];
  size_t steps = 0;
  Result result = Result::IN_PROGRESS;
  while (result == Result::IN_PROGRESS && steps < 10000) {
    result = session.step(stream, sink, true, static_cast<uint32_t>(steps), buffer, sizeof(buffer));
    steps++;
  }
  ASSERT_TRUE(result == Result::COMPLETE);
  ASSERT_TRUE(steps == total / kBufferSize);
  ASSERT_TRUE(sink.largestWrite <= kBufferSize);
  ASSERT_TRUE(sink.written == total);
  ASSERT_EQ(session.progressPercent(), 100);
  return 0;
}

static int test_partial_last_chunk() {
  test_begin("OtaDownloadSession", "last step reads only the remaining bytes");
  OtaDownloadSession session(0, 10000, kStallMs, kTotalMs);
  FakeStream stream;
  stream.availableBytes = 20000;
  FakeSink sink;
  uint8_t buffer[kBufferSize];
  ASSERT_TRUE(session.step(stream, sink, true, 1, buffer, sizeof(buffer)) == Result::IN_PROGRESS);
  ASSERT_TRUE(session.step(stream, sink, true, 2, buffer, sizeof(buffer)) == Result::IN_PROGRESS);
  ASSERT_TRUE(session.step(stream, sink, true, 3, buffer, sizeof(buffer)) == Result::COMPLETE);
  ASSERT_EQ(sink.written, 10000);
  ASSERT_EQ(stream.availableBytes, 10000);
  return 0;
}

static int test_stall_detected_across_steps() {
  test_begin("OtaDownloadSession", "no data for the stall timeout aborts");
  OtaDownloadSession session(0, 100000, kStallMs, kTotalMs);
  FakeStream stream;
  FakeSink sink;
  uint8_t buffer[kBufferSize];
  ASSERT_TRUE(session.step(stream, sink, true, kStallMs - 1, buffer, sizeof(buffer)) == Result::IN_PROGRESS);
  ASSERT_TRUE(session.step(stream, sink, true, kStallMs, buffer, sizeof(buffer)) == Result::STALLED);
  return 0;
}

static int test_total_timeout() {
  test_begin("OtaDownloadSession", "slow trickle hits the overall timeout");
  OtaDownloadSession session(0, 100000, kStallMs, kTotalMs);
  FakeStream stream;
  FakeSink sink;
  uint8_t buffer[kBufferSize];
  // 10 bytes every 10 s: never stalls, but never finishes in time
  for (uint32_t t = 10000; t < kTotalMs; t += 10000) {
    stream.availableBytes = 10;
    ASSERT_TRUE(session.step(stream, sink, true, t, buffer, sizeof(buffer)) == Result::IN_PROGRESS);
  }
  stream.availableBytes = 10;
  ASSERT_TRUE(session.step(stream, sink, true, kTotalMs, buffer, sizeof(buffer)) == Result::TIMED_OUT);
  return 0;
}

static int test_disconnect_and_write_error() {
  test_begin("OtaDownloadSession", "closed connection and flash write errors abort");
  uint8_t buffer[kBufferSize];
  {
    OtaDownloadSession session(0, 100000, kStallMs, kTotalMs);
    FakeStream stream;
    FakeSink sink;
    ASSERT_TRUE(session.step(stream, sink, false, 1, buffer, sizeof(buffer)) == Result::DISCONNECTED);
  }
  {
    OtaDownloadSession session(0, 100000, kStallMs, kTotalMs);
    FakeStream stream;
    stream.availableBytes = 5000;
    FakeSink sink;
    sink.failWrites = true;
    ASSERT_TRUE(session.step(stream, sink, true, 1, buffer, sizeof(buffer)) == Result::WRITE_ERROR);
  }
  return 0;
}

int run_ota_download_session_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_step_moves_at_most_one_buffer, test_step_without_data_returns_immediately,
    test_download_spread_over_loop_iterations, test_partial_last_chunk, test_stall_detected_across_steps, test_total_timeout,
    test_disconnect_and_write_error};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("OtaDownloadSession", passed, failed);
  return failed;
}
