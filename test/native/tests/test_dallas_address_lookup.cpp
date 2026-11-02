// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_dallas_address_lookup.cpp
 * @brief Tests for findDallasDeviceByAddress — regression for the shared-bus
 *        rescan: a node must find its own sensor by address and must never
 *        fall back to whatever device now sits at its old bus index.
 */

#include <cstdint>
#include <cstring>
#include <vector>

#include "DallasAddressLookup.hpp"

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

using PoolController::findDallasDeviceByAddress;
using PoolController::kDallasAddressSize;

namespace {

struct Address {
  uint8_t bytes[kDallasAddressSize];
};

constexpr Address kSolarProbe{{0x28, 0x01, 0, 0, 0, 0, 0, 0xA1}};
constexpr Address kPoolProbe{{0x28, 0x02, 0, 0, 0, 0, 0, 0xB2}};

struct FakeBus {
  std::vector<Address> devices;

  bool getAddress(uint8_t *addr, uint8_t index) const {
    if (index >= devices.size()) {
      return false;
    }
    memcpy(addr, devices[index].bytes, kDallasAddressSize);
    return true;
  }

  uint8_t count() const { return static_cast<uint8_t>(devices.size()); }
};

}  // namespace

static int test_finds_device_at_its_index() {
  test_begin("DallasAddressLookup", "finds the sensor at its current bus index");
  FakeBus bus{{kSolarProbe, kPoolProbe}};
  uint8_t index = 0xFF;

  ASSERT_TRUE(findDallasDeviceByAddress(bus, bus.count(), kPoolProbe.bytes, index));
  ASSERT_EQ(index, 1);
  return 0;
}

static int test_missing_solar_probe_does_not_match_pool_probe() {
  test_begin("DallasAddressLookup", "missing solar probe is not replaced by the pool probe at index 0");
  // Solar probe dropped off the shared bus; the pool probe moved to index 0.
  FakeBus bus{{kPoolProbe}};
  uint8_t index = 0xFF;

  ASSERT_TRUE(!findDallasDeviceByAddress(bus, bus.count(), kSolarProbe.bytes, index));
  ASSERT_EQ(index, 0xFF);
  return 0;
}

static int test_sensor_found_again_after_reconnect() {
  test_begin("DallasAddressLookup", "reconnected sensor is found at its new index");
  FakeBus bus{{kPoolProbe, kSolarProbe}};
  uint8_t index = 0xFF;

  ASSERT_TRUE(findDallasDeviceByAddress(bus, bus.count(), kSolarProbe.bytes, index));
  ASSERT_EQ(index, 1);
  return 0;
}

static int test_empty_bus() {
  test_begin("DallasAddressLookup", "empty bus finds nothing");
  FakeBus bus;
  uint8_t index = 0xFF;

  ASSERT_TRUE(!findDallasDeviceByAddress(bus, bus.count(), kSolarProbe.bytes, index));
  return 0;
}

int run_dallas_address_lookup_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_finds_device_at_its_index, test_missing_solar_probe_does_not_match_pool_probe,
    test_sensor_found_again_after_reconnect, test_empty_bus};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("DallasAddressLookup", passed, failed);
  return failed;
}
