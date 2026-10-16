// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_controller_snapshot_store.cpp
 * @brief Tests for coherent publication of controller read models.
 */

#include "ControllerSnapshotStore.hpp"

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

using PoolController::ControllerSnapshotStore;
using PoolController::OperationMode;
using PoolController::SystemSnapshot;

static int test_empty_store() {
  test_begin("ControllerSnapshotStore", "read fails until first complete snapshot is published");
  ControllerSnapshotStore store;
  SystemSnapshot snapshot{};
  ASSERT_TRUE(!store.hasSnapshot());
  ASSERT_TRUE(!store.read(snapshot));
  return 0;
}

static int test_complete_snapshot_copy() {
  test_begin("ControllerSnapshotStore", "read returns one complete published snapshot");
  ControllerSnapshotStore store;
  SystemSnapshot published{};
  published.sensors.generation = 7;
  published.sensors.pool.value = 27.25F;
  published.sensors.pool.valid = true;
  published.mode = OperationMode::BOOST;
  published.poolPumpOn = true;
  published.solarPumpOn = true;
  published.freeHeapBytes = 123456;

  store.publish(published);

  SystemSnapshot read{};
  ASSERT_TRUE(store.hasSnapshot());
  ASSERT_TRUE(store.read(read));
  ASSERT_EQ(read.sensors.generation, 7U);
  ASSERT_EQ(read.sensors.pool.value, 27.25F);
  ASSERT_TRUE(read.sensors.pool.valid);
  ASSERT_EQ(read.mode, OperationMode::BOOST);
  ASSERT_TRUE(read.poolPumpOn && read.solarPumpOn);
  ASSERT_EQ(read.freeHeapBytes, 123456U);
  return 0;
}

static int test_new_publish_replaces_whole_snapshot() {
  test_begin("ControllerSnapshotStore", "new publication atomically replaces the previous value object");
  ControllerSnapshotStore store;
  SystemSnapshot first{};
  first.sensors.generation = 1;
  first.poolPumpOn = true;
  first.solarPumpOn = false;
  store.publish(first);

  SystemSnapshot second{};
  second.sensors.generation = 2;
  second.poolPumpOn = false;
  second.solarPumpOn = true;
  store.publish(second);

  SystemSnapshot read{};
  ASSERT_TRUE(store.read(read));
  ASSERT_EQ(read.sensors.generation, 2U);
  ASSERT_TRUE(!read.poolPumpOn);
  ASSERT_TRUE(read.solarPumpOn);
  return 0;
}

int run_controller_snapshot_store_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_empty_store, test_complete_snapshot_copy, test_new_publish_replaces_whole_snapshot};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("ControllerSnapshotStore", passed, failed);
  return failed;
}
