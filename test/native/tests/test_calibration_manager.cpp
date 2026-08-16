#include <cstdio>
#include "CalibrationManager.hpp"

extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_EQ(a, b)                                              \
  do {                                                               \
    auto _a = (a);                                                   \
    auto _b = (b);                                                   \
    if (_a != _b) {                                                  \
      printf("    ✗ %s:%d expected equality\n", __FILE__, __LINE__); \
      return 1;                                                      \
    }                                                                \
  } while (0)

using PoolController::CalibrationManager;

// Test hooks: controllable ADC + clock
static uint16_t g_adc = 0;
static uint32_t g_now = 0;
static uint16_t fakeAdc() { return g_adc; }
static uint32_t fakeTime() { return g_now; }

static int test_start_from_idle() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700; g_now = 0;
  CalibrationManager::begin();
  ASSERT_EQ(CalibrationManager::isActive(), false);
  ASSERT_EQ(CalibrationManager::start(), true);
  ASSERT_EQ(CalibrationManager::isActive(), true);
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::RESTING);
  return 0;
}

static int test_resting_measurement() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700; g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();
  // Stable resting level: advance through the wait + sample phases
  for (uint32_t t = 0; t < 2000; t += 50) {
    g_now = t;
    CalibrationManager::loop();
  }
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::BTN1);
  ASSERT_EQ(CalibrationManager::getStatus().restingLevel, 2700);
  return 0;
}

static int test_timeout_retries_step() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700; g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();
  // No stable level: ADC oscillates wildly, never stabilizes
  g_adc = 100;
  for (uint32_t t = 0; t < 11000; t += 50) {
    g_now = t;
    g_adc = (g_adc + 500) % 4096;  // never stable
    CalibrationManager::loop();
  }
  // Still in RESTING (retried), not advanced
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::RESTING);
  return 0;
}

static int test_cancel_from_step() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700; g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();
  CalibrationManager::cancel();
  ASSERT_EQ(CalibrationManager::isActive(), false);
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::IDLE);
  return 0;
}

int run_calibration_manager_tests() {
  int failures = 0;
  failures += test_start_from_idle();
  failures += test_resting_measurement();
  failures += test_timeout_retries_step();
  failures += test_cancel_from_step();
  test_suite_end("CalibrationManager", 4 - failures, failures);
  if (failures == 0) {
    printf("  CalibrationManager Tests: 4 passed, 0 failed\n");
  }
  return failures;
}