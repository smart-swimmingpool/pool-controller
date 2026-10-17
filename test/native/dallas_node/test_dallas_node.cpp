// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_dallas_node.cpp
 * @brief Tests the real DallasTemperatureNode (failure counting, rescan and
 *        shared-bus address handling) against a programmable bus fake.
 */

#include <cmath>
#include <cstdio>

#include "DallasTemperature.h"
#include "DallasTemperatureNode.hpp"
#include "SensorSlots.hpp"

using PoolController::SensorId;

namespace {

int g_passed = 0;
int g_failed = 0;

void check(bool cond, const char *file, int line, const char *expr) {
  if (cond) {
    g_passed++;
  } else {
    g_failed++;
    fprintf(stderr, "  ✗ FAIL %s:%d — %s\n", file, line, expr);
  }
}

#define CHECK(cond) check((cond), __FILE__, __LINE__, #cond)

void section(const char *name) {
  printf("\n== %s ==\n", name);
}

const DeviceAddress kSolarProbe = {0x28, 0x01, 0, 0, 0, 0, 0, 0xA1};
const DeviceAddress kPoolProbe = {0x28, 0x02, 0, 0, 0, 0, 0, 0xB2};

void measure(DallasTemperatureNode &node) {
  node.beginMeasurement();
  node.finishMeasurement();
}

void test_shared_bus_never_hands_pool_probe_to_solar() {
  section("shared bus: a missing solar probe is never replaced by the pool probe");
  PoolController::SensorSlots::reset();
  DallasTemperature bus;
  bus.addDevice(kSolarProbe, 40.0f);
  bus.addDevice(kPoolProbe, 24.0f);
  DallasTemperatureNode solar(SensorId::SOLAR, "solar-temp", "Solar", &bus, 0);
  DallasTemperatureNode pool(SensorId::POOL, "pool-temp", "Pool", &bus, 1);
  solar.begin();
  pool.begin();

  measure(solar);
  measure(pool);
  CHECK(solar.getTemperature() == 40.0f);
  CHECK(pool.getTemperature() == 24.0f);

  // The solar probe drops off: the pool probe moves to index 0.
  bus.removeDevice(kSolarProbe);
  for (int cycle = 0; cycle < 6; ++cycle) {
    measure(solar);
    measure(pool);
    CHECK(std::isnan(solar.getTemperature()));
    CHECK(!solar.isSensorFound());
    CHECK(pool.getTemperature() == 24.0f);
  }

  // The probe comes back: the solar node reads it again by address.
  bus.addDevice(kSolarProbe, 41.0f);
  measure(solar);
  CHECK(solar.getTemperature() == 41.0f);
  CHECK(solar.isSensorFound());
}

void test_three_failed_reads_trigger_one_rescan() {
  section("three failed reads trigger exactly one bus rescan");
  PoolController::SensorSlots::reset();
  DallasTemperature bus;
  bus.addDevice(kSolarProbe, DEVICE_DISCONNECTED_C);
  DallasTemperatureNode solar(SensorId::SOLAR, "solar-temp", "Solar", &bus, 0);
  solar.begin();
  const int beginsAfterSetup = bus.beginCalls();

  measure(solar);
  measure(solar);
  CHECK(bus.beginCalls() == beginsAfterSetup);
  measure(solar);
  CHECK(bus.beginCalls() == beginsAfterSetup + 1);
  CHECK(!solar.isSensorFound());
}

void test_unconfirmed_power_on_value_is_not_a_failure() {
  section("unconfirmed 85 °C after a failed read does not count towards a rescan");
  PoolController::SensorSlots::reset();
  DallasTemperature bus;
  bus.addDevice(kSolarProbe, DEVICE_DISCONNECTED_C);
  DallasTemperatureNode solar(SensorId::SOLAR, "solar-temp", "Solar", &bus, 0);
  solar.begin();
  const int beginsAfterSetup = bus.beginCalls();

  measure(solar);  // failed read
  bus.setReading(kSolarProbe, 85.0f);
  measure(solar);  // unconfirmed
  measure(solar);  // unconfirmed
  CHECK(std::isnan(solar.getTemperature()));
  CHECK(bus.beginCalls() == beginsAfterSetup);

  measure(solar);  // repetition never confirms the power-on value
  CHECK(std::isnan(solar.getTemperature()));
  CHECK(bus.beginCalls() == beginsAfterSetup);

  bus.setReading(kSolarProbe, 42.0f);
  measure(solar);  // supply recovered
  CHECK(solar.getTemperature() == 42.0f);
}

void test_mapping_change_resets_filter_history() {
  section("a mapping change to another probe does not hold the previous probe's value");
  PoolController::SensorSlots::reset();
  DallasTemperature bus;
  bus.addDevice(kSolarProbe, 40.0f);
  bus.addDevice(kPoolProbe, 24.0f);
  DallasTemperatureNode solar(SensorId::SOLAR, "solar-temp", "Solar", &bus, 0);
  solar.begin();
  measure(solar);
  CHECK(solar.getTemperature() == 40.0f);

  // Re-map the role to the other probe, which reports its power-on value.
  bus.setReading(kPoolProbe, 85.0f);
  solar.setAddressFilter(kPoolProbe);
  measure(solar);
  CHECK(std::isnan(solar.getTemperature()));  // not HOLD of the old probe's 40 °C
  CHECK(!solar.isSensorFound());
}

}  // namespace

int main() {
  test_shared_bus_never_hands_pool_probe_to_solar();
  test_three_failed_reads_trigger_one_rescan();
  test_unconfirmed_power_on_value_is_not_a_failure();
  test_mapping_change_resets_filter_history();

  printf("\n══════════════════════════════════════════════════\n");
  printf("  Dallas Node Results: %d passed, %d failed\n", g_passed, g_failed);
  printf("══════════════════════════════════════════════════\n");

  return g_failed == 0 ? 0 : 1;
}
