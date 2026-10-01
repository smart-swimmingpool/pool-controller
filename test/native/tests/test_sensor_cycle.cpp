// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_sensor_cycle.cpp
 * @brief Tests for runDallasMeasurementCycle() — every bus must have a
 *        conversion started before its sensors are read, for the shared
 *        (NORVI) and the dedicated (esp32dev) topology (review #170).
 */

#include <cstdio>

#include "SensorCycle.hpp"

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

namespace {

// A OneWire bus: counts conversion requests and remembers whether a
// conversion was running when a sensor on it was read.
struct FakeBus {
  int conversions = 0;
  int readsWithConversion = 0;
  int readsWithoutConversion = 0;
};

// Mirrors DallasTemperatureNode: on a shared bus only the master starts the
// conversion; on a dedicated bus every node starts its own.
struct FakeNode {
  FakeBus *bus;
  bool shared;
  bool master;
  void beginMeasurement() {
    if (!shared || master) {
      bus->conversions++;
    }
  }
  void finishMeasurement() {
    if (bus->conversions > 0) {
      bus->readsWithConversion++;
    } else {
      bus->readsWithoutConversion++;
    }
  }
};

}  // namespace

static int test_dedicated_buses() {
  test_begin("SensorCycle", "dedicated buses: each bus starts its own conversion");
  FakeBus solarBus;
  FakeBus poolBus;
  FakeNode solar{&solarBus, false, false};
  FakeNode pool{&poolBus, false, false};
  int waits = 0;
  PoolController::runDallasMeasurementCycle(solar, pool, [&waits] { waits++; });
  ASSERT_EQ(waits, 1);
  ASSERT_EQ(solarBus.conversions, 1);
  ASSERT_EQ(poolBus.conversions, 1);
  ASSERT_EQ(poolBus.readsWithoutConversion, 0);
  ASSERT_EQ(poolBus.readsWithConversion, 1);
  return 0;
}

static int test_shared_bus() {
  test_begin("SensorCycle", "shared bus: one conversion for both sensors");
  FakeBus bus;
  FakeNode solar{&bus, true, true};
  FakeNode pool{&bus, true, false};
  int waits = 0;
  PoolController::runDallasMeasurementCycle(solar, pool, [&waits] { waits++; });
  ASSERT_EQ(waits, 1);
  ASSERT_EQ(bus.conversions, 1);
  ASSERT_EQ(bus.readsWithConversion, 2);
  ASSERT_EQ(bus.readsWithoutConversion, 0);
  return 0;
}

static int test_reads_happen_after_wait() {
  test_begin("SensorCycle", "both conversions start before the single wait");
  FakeBus solarBus;
  FakeBus poolBus;
  FakeNode solar{&solarBus, false, false};
  FakeNode pool{&poolBus, false, false};
  bool startedBeforeWait = false;
  bool readBeforeWait = false;
  PoolController::runDallasMeasurementCycle(solar, pool, [&] {
    startedBeforeWait = solarBus.conversions == 1 && poolBus.conversions == 1;
    readBeforeWait = (solarBus.readsWithConversion + poolBus.readsWithConversion) > 0;
  });
  ASSERT_TRUE(startedBeforeWait);
  ASSERT_TRUE(!readBeforeWait);
  return 0;
}

int run_sensor_cycle_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_dedicated_buses, test_shared_bus, test_reads_happen_after_wait};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("SensorCycle", passed, failed);
  return failed;
}
