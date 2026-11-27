/**
 * @file test_core_scheduler.cpp
 * @brief Unit tests for the real CoreScheduler against a recording SensorTask fake.
 */

#include "Arduino.h"
#include "CoreScheduler.hpp"
#include "SensorTask.hpp"
#include "test_assert.h"

using namespace PoolController;  // NOLINT(build/namespaces)

namespace {
struct SensorTaskFake {
  bool startResult = true;
  int startCalls = 0;
  uint8_t priority = 0;
  uint16_t stackBytes = 0;
  BaseType_t core = -1;
  int watermarkLogs = 0;
};
SensorTaskFake g_sensorTask;
}  // namespace

// Fake SensorTask: CoreScheduler links against it instead of FreeRTOS.
namespace PoolController {
bool SensorTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  ++g_sensorTask.startCalls;
  g_sensorTask.priority = priority;
  g_sensorTask.stackBytes = stackBytes;
  g_sensorTask.core = core;
  return g_sensorTask.startResult;
}
void SensorTask::logStackWatermark() {
  ++g_sensorTask.watermarkLogs;
}
}  // namespace PoolController

static int test_begin_pins_sensor_task_to_core0() {
  test_begin("CoreScheduler", "begin() starts SensorTask on Core 0 with the planned priority and stack");
  g_sensorTask = SensorTaskFake{};
  ESPClass::s_restartCount = 0;

  CoreScheduler::begin();
  ASSERT_EQ(g_sensorTask.startCalls, 1);
  ASSERT_EQ(g_sensorTask.priority, CoreScheduler::TASK_PRIORITY_SENSOR);
  ASSERT_EQ(g_sensorTask.stackBytes, CoreScheduler::TASK_STACK_SENSOR);
  ASSERT_EQ(g_sensorTask.core, static_cast<BaseType_t>(PRO_CPU_NUM));
  ASSERT_EQ(ESPClass::s_restartCount, 0);
  return 0;
}

static int test_begin_restarts_when_task_cannot_be_created() {
  test_begin("CoreScheduler", "begin() restarts the controller when SensorTask cannot be created");
  g_sensorTask = SensorTaskFake{};
  g_sensorTask.startResult = false;
  ESPClass::s_restartCount = 0;

  CoreScheduler::begin();
  ASSERT_EQ(ESPClass::s_restartCount, 1);
  ESPClass::s_restartCount = 0;
  return 0;
}

static int test_watermark_logging_is_rate_limited() {
  test_begin("CoreScheduler", "stack watermark is logged at most once per minute");
  g_sensorTask = SensorTaskFake{};

  // Move past any earlier log so the next call is due.
  advanceMillisForTest(60001);
  CoreScheduler::logStackWatermarks();
  CoreScheduler::logStackWatermarks();
  ASSERT_EQ(g_sensorTask.watermarkLogs, 1);

  advanceMillisForTest(60001);
  CoreScheduler::logStackWatermarks();
  ASSERT_EQ(g_sensorTask.watermarkLogs, 2);
  return 0;
}

int run_core_scheduler_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_begin_pins_sensor_task_to_core0, test_begin_restarts_when_task_cannot_be_created,
    test_watermark_logging_is_rate_limited};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("CoreScheduler", passed, failed);
  return failed;
}
