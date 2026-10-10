// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_controller_snapshot_diagnostics.cpp
 * @brief Native contract tests for adapter-facing network and heap diagnostics.
 */

#include <type_traits>

#include "ControllerSnapshot.hpp"

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

static int test_adapter_diagnostics_are_bounded_value_copies() {
  test_begin("ControllerSnapshotDiagnostics", "network address and heap diagnostics are bounded value copies");

  PoolController::SystemSnapshot snapshot{};
  snapshot.network.wifiConnected = true;
  snapshot.network.apMode = true;
  snapshot.network.localIp.octets = {192, 168, 4, 1};
  snapshot.network.localIp.valid = true;
  snapshot.freeHeapBytes = 110000;
  snapshot.maxAllocHeapBytes = 64000;

  const PoolController::SystemSnapshot published = snapshot;

  snapshot.network.localIp.octets = {10, 0, 0, 1};
  snapshot.network.localIp.valid = false;
  snapshot.freeHeapBytes = 1;
  snapshot.maxAllocHeapBytes = 2;

  ASSERT_TRUE(published.network.wifiConnected);
  ASSERT_TRUE(published.network.apMode);
  ASSERT_TRUE(published.network.localIp.valid);
  ASSERT_EQ(published.network.localIp.octets[0], 192U);
  ASSERT_EQ(published.network.localIp.octets[1], 168U);
  ASSERT_EQ(published.network.localIp.octets[2], 4U);
  ASSERT_EQ(published.network.localIp.octets[3], 1U);
  ASSERT_EQ(published.freeHeapBytes, 110000U);
  ASSERT_EQ(published.maxAllocHeapBytes, 64000U);
  ASSERT_TRUE(std::is_trivially_copyable<PoolController::Ipv4AddressSnapshot>::value);
  ASSERT_TRUE(std::is_trivially_copyable<PoolController::SystemSnapshot>::value);
  return 0;
}

int run_controller_snapshot_diagnostics_tests() {
  int passed = 0;
  int failed = 0;
  if (test_adapter_diagnostics_are_bounded_value_copies() == 0) {
    ++passed;
  } else {
    ++failed;
  }
  test_suite_end("ControllerSnapshotDiagnostics", passed, failed);
  return failed;
}
