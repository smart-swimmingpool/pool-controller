// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "ControllerSnapshotPublisher.hpp"

#include <cstring>

#include "ConfigManager.hpp"
#include "ControllerSnapshot.hpp"
#include "ControllerSnapshotStore.hpp"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "NetworkManager.hpp"
#include "OperationModeNode.hpp"
#include "RelayModuleNode.hpp"

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

using PoolController::ControllerSnapshotPublisher;
using PoolController::ControllerSnapshotStore;
using PoolController::OperationMode;
using PoolController::SystemSnapshot;
using PoolController::TimeDegradationState;

struct SnapshotFixture {
  OperationModeNode operation{"operation-mode", "Operation Mode", 300};
  RelayModuleNode poolPump{"pool", "Pool", 1};
  RelayModuleNode solarPump{"solar", "Solar", 2};
  DallasTemperatureNode solarTemperature{"solar-temp", "Solar Temperature", 3};
  DallasTemperatureNode poolTemperature{"pool-temp", "Pool Temperature", 4};
  ESP32TemperatureNode controllerTemperature{"controller-temp", "Controller Temperature"};
  ControllerSnapshotStore store;
  ControllerSnapshotPublisher publisher{ControllerSnapshotPublisher::Dependencies{
    operation, poolPump, solarPump, solarTemperature, poolTemperature, controllerTemperature, store}};
};

static int test_complete_runtime_state_is_projected() {
  test_begin("ControllerSnapshotPublisher", "publishes complete immutable runtime state");
  SnapshotFixture fixture;

  auto &settings = PoolController::ConfigManager::getSettings();
  settings.loopInterval = 42;
  settings.timezoneIndex = 3;
  settings.timeLossGreenHours = 2;
  settings.timeLossRedHours = 18;
  settings.tempCircThreshold = 25.5F;
  settings.tempCircFactor = 35;
  settings.tempCircMaxRuntime = 600;
  settings.btn1Min = 3010;
  settings.btn1Max = 3510;
  settings.btn2Min = 3511;
  settings.btn2Max = 3870;
  settings.btn3Min = 3871;
  settings.btn3Max = 4094;
  settings.btnNoPress = 4096;

  fixture.operation.setMode(OperationMode::BOOST);
  fixture.operation.setPoolMaxTemperature(29.5F);
  fixture.operation.setSolarMinTemperature(48.0F);
  fixture.operation.setTemperatureHysteresis(1.5F);

  TimerSetting timer{};
  timer.timerStartHour = 7;
  timer.timerStartMinutes = 15;
  timer.timerEndHour = 20;
  timer.timerEndMinutes = 45;
  fixture.operation.setTimerSetting(timer);

  fixture.poolPump.setSwitch(true);
  fixture.solarPump.setSwitch(false);
  fixture.poolTemperature.setTemperature(26.25F);
  fixture.poolTemperature.setSensorFound(true);
  fixture.solarTemperature.setTemperature(51.5F);
  fixture.solarTemperature.setSensorFound(true);
  fixture.controllerTemperature.setTemperature(38.0F);

  const std::uint8_t solarAddress[8] = {0x28, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const std::uint8_t poolAddress[8] = {0x28, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
  PoolController::ConfigManager::saveSensorMapping(solarAddress, poolAddress);
  fixture.solarTemperature.setAddressFilter(solarAddress);
  fixture.poolTemperature.setAddressFilter(poolAddress);

  PoolController::NetworkManager::setWiFiConnected(true);
  PoolController::NetworkManager::setMqttConnected(true);
  PoolController::NetworkManager::setApMode(false);
  PoolController::NetworkManager::setWiFiRSSI(-61);

  fixture.publisher.publish();

  SystemSnapshot snapshot{};
  ASSERT_TRUE(fixture.store.read(snapshot));
  ASSERT_EQ(snapshot.mode, OperationMode::BOOST);
  ASSERT_TRUE(snapshot.poolPumpOn);
  ASSERT_TRUE(!snapshot.solarPumpOn);
  ASSERT_EQ(snapshot.poolMaxTemperature, 29.5F);
  ASSERT_EQ(snapshot.solarMinTemperature, 48.0F);
  ASSERT_EQ(snapshot.temperatureHysteresis, 1.5F);

  ASSERT_TRUE(snapshot.sensors.pool.valid);
  ASSERT_TRUE(snapshot.sensors.solar.valid);
  ASSERT_TRUE(snapshot.sensors.controller.valid);
  ASSERT_EQ(snapshot.sensors.pool.value, 26.25F);
  ASSERT_EQ(snapshot.sensors.solar.value, 51.5F);
  ASSERT_EQ(snapshot.sensors.controller.value, 38.0F);
  ASSERT_TRUE(snapshot.sensors.poolMapping.configured);
  ASSERT_TRUE(snapshot.sensors.poolMapping.found);
  ASSERT_TRUE(snapshot.sensors.solarMapping.configured);
  ASSERT_TRUE(snapshot.sensors.solarMapping.found);
  ASSERT_EQ(std::memcmp(snapshot.sensors.poolMapping.address.data(), poolAddress, sizeof(poolAddress)), 0);
  ASSERT_EQ(std::memcmp(snapshot.sensors.solarMapping.address.data(), solarAddress, sizeof(solarAddress)), 0);

  ASSERT_TRUE(snapshot.network.wifiConnected);
  ASSERT_TRUE(snapshot.network.mqttConnected);
  ASSERT_TRUE(!snapshot.network.apMode);
  ASSERT_EQ(snapshot.network.wifiRssi, -61);
  ASSERT_TRUE(!snapshot.health.safeMode);
  ASSERT_TRUE(snapshot.health.timeValid);
  ASSERT_EQ(snapshot.health.timeDegradation, TimeDegradationState::GREEN);
  ASSERT_TRUE(snapshot.health.poolSensorValid);
  ASSERT_TRUE(snapshot.health.solarSensorValid);

  ASSERT_EQ(snapshot.circulation.threshold, 25.5F);
  ASSERT_EQ(snapshot.circulation.factorMinutesPerDegree, 35);
  ASSERT_EQ(snapshot.circulation.maxRuntimeMinutes, 600);
  ASSERT_EQ(snapshot.timer.startHour, 7);
  ASSERT_EQ(snapshot.timer.startMinute, 15);
  ASSERT_EQ(snapshot.timer.endHour, 20);
  ASSERT_EQ(snapshot.timer.endMinute, 45);

  ASSERT_EQ(snapshot.settings.loopInterval, 42U);
  ASSERT_EQ(snapshot.settings.timezoneIndex, 3);
  ASSERT_EQ(snapshot.settings.timeLossGreenHours, 2);
  ASSERT_EQ(snapshot.settings.timeLossRedHours, 18);
  ASSERT_EQ(snapshot.settings.button1Min, 3010);
  ASSERT_EQ(snapshot.settings.button3Max, 4094);
  ASSERT_EQ(snapshot.settings.buttonNoPress, 4096);
  ASSERT_EQ(snapshot.freeHeapBytes, 180000U);
  ASSERT_EQ(snapshot.sensors.generation, 1U);
  return 0;
}

static int test_mapping_found_requires_selected_configured_address() {
  test_begin("ControllerSnapshotPublisher", "configured mapping is not found when selected ROM differs");
  SnapshotFixture fixture;

  const std::uint8_t configuredSolar[8] = {0x28, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};
  const std::uint8_t selectedSolar[8] = {0x28, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
  const std::uint8_t poolAddress[8] = {0};
  PoolController::ConfigManager::saveSensorMapping(configuredSolar, poolAddress);
  fixture.solarTemperature.setAddressFilter(selectedSolar);
  fixture.solarTemperature.setSensorFound(true);
  fixture.solarTemperature.setTemperature(50.0F);

  fixture.publisher.publish();
  SystemSnapshot snapshot{};
  ASSERT_TRUE(fixture.store.read(snapshot));
  ASSERT_TRUE(snapshot.sensors.solarMapping.configured);
  ASSERT_TRUE(!snapshot.sensors.solarMapping.found);
  return 0;
}

static int test_generation_advances_per_complete_publication() {
  test_begin("ControllerSnapshotPublisher", "read-model generation advances atomically per publish");
  SnapshotFixture fixture;

  fixture.publisher.publish();
  SystemSnapshot first{};
  ASSERT_TRUE(fixture.store.read(first));

  fixture.publisher.publish();
  SystemSnapshot second{};
  ASSERT_TRUE(fixture.store.read(second));
  ASSERT_EQ(first.sensors.generation, 1U);
  ASSERT_EQ(second.sensors.generation, 2U);
  return 0;
}

int run_controller_snapshot_publisher_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_complete_runtime_state_is_projected, test_mapping_found_requires_selected_configured_address,
    test_generation_advances_per_complete_publication};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("ControllerSnapshotPublisher", passed, failed);
  return failed;
}
