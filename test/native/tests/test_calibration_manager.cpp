#include <cstdio>
#include "CalibrationManager.hpp"
#include "ConfigManager.hpp"

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
static uint16_t fakeAdc() {
  return g_adc;
}
static uint32_t fakeTime() {
  return g_now;
}

static int test_start_from_idle() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700;
  g_now = 0;
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
  g_adc = 2700;
  g_now = 0;
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
  g_adc = 2700;
  g_now = 0;
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
  g_adc = 2700;
  g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();
  CalibrationManager::cancel();
  ASSERT_EQ(CalibrationManager::isActive(), false);
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::IDLE);
  return 0;
}

static int test_full_calibration_saves_thresholds() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700;
  g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();

  // Helper: hold a level for the wait + sample phases, then switch
  auto holdLevel = [](uint16_t level, uint32_t startMs) {
    g_adc = level;
    for (uint32_t t = startMs; t < startMs + 1500; t += 50) {
      g_now = t;
      CalibrationManager::loop();
    }
  };

  holdLevel(2700, 0);     // resting
  holdLevel(3400, 2000);  // S1
  holdLevel(3700, 4000);  // S2
  holdLevel(4095, 6000);  // S3

  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::DONE);
  auto &s = PoolController::ConfigManager::getSettings();
  ASSERT_EQ(s.btn1Min, (2700 + 3400) / 2);
  ASSERT_EQ(s.btn1Max, (3400 + 3700) / 2);
  ASSERT_EQ(s.btn2Min, (3400 + 3700) / 2);
  ASSERT_EQ(s.btn2Max, (3700 + 4095) / 2);
  ASSERT_EQ(s.btn3Min, (3700 + 4095) / 2);
  ASSERT_EQ(s.btn3Max, 4095);
  ASSERT_EQ(s.btnNoPress, 4096);
  return 0;
}

static int test_sample_spacing_gate() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700;
  g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();

  // Wait phase: 3 stable readings → sampling starts at t=100
  for (uint32_t t = 0; t <= 100; t += 50) {
    g_now = t;
    CalibrationManager::loop();
  }
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::RESTING);

  // Rapid loop() calls without time advance must not collect samples —
  // the sample phase is gated on SAMPLE_INTERVAL_MS.
  for (int i = 0; i < 100; i++) {
    CalibrationManager::loop();
  }
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::RESTING);

  // Advance time in 50 ms steps → sampling completes over ~1 s window
  for (uint32_t t = 150; t < 2000; t += 50) {
    g_now = t;
    CalibrationManager::loop();
  }
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::BTN1);
  ASSERT_EQ(CalibrationManager::getStatus().restingLevel, 2700);
  return 0;
}

static int test_sample_restart_on_level_change() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700;
  g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();

  auto holdLevel = [](uint16_t level, uint32_t startMs) {
    g_adc = level;
    for (uint32_t t = startMs; t < startMs + 1500; t += 50) {
      g_now = t;
      CalibrationManager::loop();
    }
  };

  holdLevel(2700, 0);  // resting → BTN1 at t=1100

  // BTN1: stabilize at 3400, then release mid-sampling → restart
  g_adc = 3400;
  for (uint32_t t = 2000; t <= 2200; t += 50) {  // wait t=2000..2100, samples t=2150,2200
    g_now = t;
    CalibrationManager::loop();
  }
  g_adc = 2700;  // release during sampling
  for (uint32_t t = 2250; t < 3000; t += 50) {
    g_now = t;
    CalibrationManager::loop();
  }
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::BTN1);
  ASSERT_EQ(CalibrationManager::getStatus().s1, 0);

  // Press again → sampling completes with a clean level
  holdLevel(3400, 3000);
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::BTN2);
  ASSERT_EQ(CalibrationManager::getStatus().s1, 3400);
  return 0;
}

static int test_release_level_rejected() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700;
  g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();

  auto holdLevel = [](uint16_t level, uint32_t startMs) {
    g_adc = level;
    for (uint32_t t = startMs; t < startMs + 1500; t += 50) {
      g_now = t;
      CalibrationManager::loop();
    }
  };

  holdLevel(2700, 0);     // resting
  holdLevel(3700, 2000);  // S1
  holdLevel(2700, 4000);  // release level (below S1) — must be rejected
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::BTN2);
  holdLevel(3900, 5500);  // valid S2
  holdLevel(4095, 8000);  // S3
  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::DONE);
  return 0;
}

static int test_exact_minimum_gap_accepted() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700;
  g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();

  auto holdLevel = [](uint16_t level, uint32_t startMs) {
    g_adc = level;
    for (uint32_t t = startMs; t < startMs + 1500; t += 50) {
      g_now = t;
      CalibrationManager::loop();
    }
  };

  // Every level sits exactly MIN_LEVEL_GAP (100) above the previous one —
  // the inclusive boundary must accept these.
  holdLevel(2700, 0);     // resting
  holdLevel(2800, 2000);  // S1 = resting + 100
  holdLevel(2900, 4000);  // S2 = S1 + 100
  holdLevel(3000, 6000);  // S3 = S2 + 100

  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::DONE);
  return 0;
}

static int test_save_failure_error() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700;
  g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();

  auto holdLevel = [](uint16_t level, uint32_t startMs) {
    g_adc = level;
    for (uint32_t t = startMs; t < startMs + 1500; t += 50) {
      g_now = t;
      CalibrationManager::loop();
    }
  };

  holdLevel(2700, 0);
  holdLevel(3400, 2000);
  holdLevel(3700, 4000);

  // Snapshot the live thresholds before the failed save
  auto &s = PoolController::ConfigManager::getSettings();
  const uint16_t prevBtn1Min = s.btn1Min;
  const uint16_t prevBtn1Max = s.btn1Max;
  const uint16_t prevBtn2Min = s.btn2Min;
  const uint16_t prevBtn2Max = s.btn2Max;
  const uint16_t prevBtn3Min = s.btn3Min;
  const uint16_t prevBtn3Max = s.btn3Max;
  const uint16_t prevBtnNoPress = s.btnNoPress;

  // Force save failure before the final step completes → ERROR state
  PoolController::ConfigManager::_saveFails = true;
  holdLevel(4095, 6000);
  PoolController::ConfigManager::_saveFails = false;

  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::ERROR);
  // Live Settings must be untouched when persistence fails
  ASSERT_EQ(s.btn1Min, prevBtn1Min);
  ASSERT_EQ(s.btn1Max, prevBtn1Max);
  ASSERT_EQ(s.btn2Min, prevBtn2Min);
  ASSERT_EQ(s.btn2Max, prevBtn2Max);
  ASSERT_EQ(s.btn3Min, prevBtn3Min);
  ASSERT_EQ(s.btn3Max, prevBtn3Max);
  ASSERT_EQ(s.btnNoPress, prevBtnNoPress);
  return 0;
}

int run_calibration_manager_tests() {
  int failures = 0;
  failures += test_start_from_idle();
  failures += test_resting_measurement();
  failures += test_timeout_retries_step();
  failures += test_cancel_from_step();
  failures += test_full_calibration_saves_thresholds();
  failures += test_sample_spacing_gate();
  failures += test_sample_restart_on_level_change();
  failures += test_release_level_rejected();
  failures += test_exact_minimum_gap_accepted();
  failures += test_save_failure_error();
  test_suite_end("CalibrationManager", 10 - failures, failures);
  if (failures == 0) {
    printf("  CalibrationManager Tests: 10 passed, 0 failed\n");
  }
  return failures;
}
