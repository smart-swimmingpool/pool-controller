// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_ota_download_guard.cpp
 * @brief Tests for OtaDownloadGuard — regression for #196: a stalled OTA
 *        download must be aborted instead of blocking the controller forever.
 */

#include <cstdint>
#include <cstdio>

#include "OtaDownloadGuard.hpp"

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

using PoolController::OtaDownloadGuard;
using Status = PoolController::OtaDownloadGuard::Status;

static constexpr uint32_t kStall = 30000;
static constexpr uint32_t kTotal = 600000;

static int test_ok_while_data_flows() {
  test_begin("OtaDownloadGuard", "ok while data keeps arriving");
  OtaDownloadGuard guard(1000, kStall, kTotal);
  ASSERT_TRUE(guard.check(1000) == Status::OK);
  for (uint32_t t = 1000; t < 1000 + 500000; t += 20000) {
    guard.onData(t);
    ASSERT_TRUE(guard.check(t + 10000) == Status::OK);
  }
  return 0;
}

static int test_stall_detected() {
  test_begin("OtaDownloadGuard", "stall detected without data");
  OtaDownloadGuard guard(0, kStall, kTotal);
  guard.onData(5000);
  ASSERT_TRUE(guard.check(5000 + kStall - 1) == Status::OK);
  ASSERT_TRUE(guard.check(5000 + kStall) == Status::STALLED);
  return 0;
}

static int test_stall_without_any_data() {
  test_begin("OtaDownloadGuard", "stall detected when no byte ever arrives");
  OtaDownloadGuard guard(0, kStall, kTotal);
  ASSERT_TRUE(guard.check(kStall) == Status::STALLED);
  return 0;
}

static int test_total_timeout() {
  test_begin("OtaDownloadGuard", "overall timeout despite slow data");
  OtaDownloadGuard guard(0, kStall, kTotal);
  guard.onData(kTotal - 1000);
  ASSERT_TRUE(guard.check(kTotal - 1) == Status::OK);
  ASSERT_TRUE(guard.check(kTotal) == Status::TIMED_OUT);
  return 0;
}

static int test_millis_wrap() {
  test_begin("OtaDownloadGuard", "handles millis() wrap-around");
  const uint32_t start = 0xFFFFFFFFu - 10000u;
  OtaDownloadGuard guard(start, kStall, kTotal);
  guard.onData(start + 20000u);  // wrapped
  ASSERT_TRUE(guard.check(start + 40000u) == Status::OK);
  ASSERT_TRUE(guard.check(start + 20000u + kStall) == Status::STALLED);
  return 0;
}

int run_ota_download_guard_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {
    test_ok_while_data_flows, test_stall_detected, test_stall_without_any_data, test_total_timeout, test_millis_wrap};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("OtaDownloadGuard", passed, failed);
  return failed;
}
