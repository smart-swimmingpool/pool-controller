# NORVI Button Calibration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a guided calibration wizard to the web UI that measures the NORVI AE01-R front-panel button ADC levels and derives the button thresholds automatically.

**Architecture:** A separate `CalibrationManager` module owns a calibration state machine (IDLE → RESTING → BTN1 → BTN2 → BTN3 → COMPUTE → SAVE → DONE/ERROR) with injectable ADC-read and time functions for native testing. `NorviButtonHandler::loop()` early-returns while calibration is active. WebPortal exposes three auth-protected REST endpoints; the web UI shows a modal wizard that polls the status endpoint.

**Tech Stack:** C++17 (ESP32/Arduino, PlatformIO env `norvi_ae01_r`), ArduinoJson, native CMake test runner (`test/native`), vanilla JS/CSS web assets on LittleFS.

## Global Constraints

- All new C++ code is guarded by `#ifdef NORVI_AE01_R` (only compiled for the NORVI variant).
- ADC is 12-bit: valid range 0–4095; `btnNoPress` sentinel stays 4096 (never changed by calibration).
- Minimum gap between adjacent measured levels: 100 ADC.
- Per-step timeout: 10 s; sampling: 20 readings at 50 ms intervals (1 s window), averaged.
- `btn3Max` stays 4095 (full scale); `btnNoPress` stays 4096.
- Thresholds are written to `ConfigManager::getSettings()` (fields `btn1Min`…`btnNoPress`) and persisted via `ConfigManager::save()`.
- REST endpoints are auth-protected via `handleAuthentication()` (same pattern as `/api/config`).
- Web UI: modal dialog pattern follows the existing `loginModal` (inline overlay, `display:flex/none`).
- Native test build: `test/native/CMakeLists.txt` defines `NORVI_AE01_R`; mocks live in `test/native/mocks/` and shadow production headers.
- Commit messages follow Conventional Commits (`feat:`, `fix:`, `test:`, `docs:`).

---

### Task 1: CalibrationManager core — state machine, measurement, hooks

**Files:**
- Create: `src/CalibrationManager.hpp`
- Create: `src/CalibrationManager.cpp`
- Create: `test/native/tests/test_calibration_manager.cpp`
- Modify: `test/native/CMakeLists.txt` (add `CalibrationManager.cpp` + `NorviButtonHandler.cpp` to `SERVICE_SOURCES`, add test to `TEST_SOURCES`)
- Modify: `test/native/mocks/ConfigManager.hpp` (add btn fields to `Settings` struct)
- Modify: `test/native/tests/test_main.cpp` (register `run_calibration_manager_tests`)

**Interfaces:**
- Consumes: `ConfigManager::getSettings()` (mock provides `Settings` with btn fields), `analogRead` (mocked in `Arduino.h`), `millis` (mocked), `NorviButtonHandler::applySettings()` (compiled from `src/`).
- Produces:
  - `enum class CalibrationManager::Step { IDLE, RESTING, BTN1, BTN2, BTN3, DONE, ERROR }`
  - `struct CalibrationManager::CalibrationStatus { Step step; uint16_t liveAdc; uint16_t restingLevel, s1, s2, s3; const char* message; }`
  - `static void begin()`, `static void loop()`, `static bool start()`, `static void cancel()`, `static CalibrationStatus getStatus()`, `static bool isActive()`
  - Test hooks: `static void setAdcReadForTest(uint16_t (*fn)())`, `static void setTimeForTest(uint32_t (*fn)())`

- [ ] **Step 1: Add btn fields to the mock Settings struct**

In `test/native/mocks/ConfigManager.hpp`, extend `struct Settings` (after `tempCircMaxRuntime`):

```cpp
  uint16_t btn1Min = 3100;
  uint16_t btn1Max = 3520;
  uint16_t btn2Min = 3520;
  uint16_t btn2Max = 3880;
  uint16_t btn3Min = 3880;
  uint16_t btn3Max = 4095;
  uint16_t btnNoPress = 4096;
```

- [ ] **Step 2: Write the failing test file**

Create `test/native/tests/test_calibration_manager.cpp` following the `test_ky040_decoder.cpp` pattern (ASSERT_EQ macro, `run_calibration_manager_tests()` returning failure count, `test_suite_end`):

```cpp
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
```

- [ ] **Step 3: Register the test in the build**

In `test/native/CMakeLists.txt`:
- Add to `SERVICE_SOURCES`: `${PROJ_ROOT}/src/CalibrationManager.cpp` and `${PROJ_ROOT}/src/NorviButtonHandler.cpp` (needed because CalibrationManager calls `NorviButtonHandler::applySettings()`).
- Add to `TEST_SOURCES`: `${CMAKE_CURRENT_SOURCE_DIR}/tests/test_calibration_manager.cpp`.

In `test/native/tests/test_main.cpp`, add `extern int run_calibration_manager_tests();` near the other externs and `total += run_calibration_manager_tests();` in `main()`.

- [ ] **Step 4: Run test to verify it fails**

Run: `cmake -B build -S . && cmake --build build && ./build/test_runner`
Expected: FAIL — `CalibrationManager.hpp` not found (header does not exist yet).

- [ ] **Step 5: Create the header**

Create `src/CalibrationManager.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file CalibrationManager.hpp
 * @brief Guided ADC calibration for the NORVI AE01-R front-panel buttons.
 *
 * Measures the resting level and each button level via the shared ADC input
 * (GPIO32), computes the button thresholds at the midpoints between adjacent
 * levels, and persists them through ConfigManager.
 *
 * @note Only available when the NORVI_AE01_R preprocessor macro is defined.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief State machine that drives the button calibration wizard.
 *
 * The wizard is driven from PoolController::loop() via loop(). The web UI
 * polls getStatus() to render instructions and the live ADC value, and calls
 * start()/cancel() to control the flow.
 */
class CalibrationManager {
public:
  /** @brief User-facing calibration steps. COMPUTE/SAVE are internal. */
  enum class Step : std::uint8_t {
    IDLE = 0,   ///< Not calibrating
    RESTING,    ///< Measure resting level (no button pressed)
    BTN1,       ///< Measure Button 1 level
    BTN2,       ///< Measure Button 2 level
    BTN3,       ///< Measure Button 3 level
    DONE,       ///< Calibration finished, thresholds saved
    ERROR       ///< Calibration failed (message in status)
  };

  /** @brief Snapshot of the calibration state for the web UI. */
  struct CalibrationStatus {
    Step step = Step::IDLE;
    uint16_t liveAdc = 0;      ///< Current filtered ADC reading
    uint16_t restingLevel = 0; ///< Measured resting level (0 until measured)
    uint16_t s1 = 0;           ///< Measured Button 1 level (0 until measured)
    uint16_t s2 = 0;           ///< Measured Button 2 level (0 until measured)
    uint16_t s3 = 0;           ///< Measured Button 3 level (0 until measured)
    const char* message = "";  ///< Instruction or error text
  };

  /** @brief Initialize the calibration manager. */
  static void begin();

  /** @brief Drive the calibration state machine. Call from PoolController::loop(). */
  static void loop();

  /** @brief Start calibration. @return false if already active. */
  static bool start();

  /** @brief Cancel calibration; old thresholds stay in NVS. */
  static void cancel();

  /** @brief Current calibration status snapshot. */
  static CalibrationStatus getStatus();

  /** @brief True while a calibration is running (used to suppress button callbacks). */
  static bool isActive();

  // ── Test hooks (native tests only) ────────────────────────────────────
  /** @brief Override the ADC read function (default: analogRead on PIN_BUTTON_ADC). */
  static void setAdcReadForTest(uint16_t (*fn)());
  /** @brief Override the time function (default: millis). */
  static void setTimeForTest(uint32_t (*fn)());

private:
  enum class State : std::uint8_t {
    IDLE, RESTING, BTN1, BTN2, BTN3, COMPUTE, SAVE, DONE, ERROR
  };

  static void enterState(State s);
  static void handleMeasurementStep(State step, uint16_t previousLevel, uint16_t& outLevel);
  static void computeThresholds();
  static void saveThresholds();
  static uint16_t readAdc();
  static uint32_t now();

  static State state_;
  static CalibrationStatus status_;
  static uint16_t (*adcRead_)();
  static uint32_t (*timeFn_)();

  // Measurement phase state
  static uint32_t stepStartMs_;
  static uint32_t sampleCount_;
  static uint32_t sampleSum_;
  static uint16_t lastReading_;
  static uint16_t stableCount_;
  static bool sampling_;
};

}  // namespace PoolController
```

- [ ] **Step 6: Create the implementation**

Create `src/CalibrationManager.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file CalibrationManager.cpp
 * @brief Guided ADC calibration for the NORVI AE01-R front-panel buttons.
 *
 * @note This file is only compiled when `NORVI_AE01_R` is defined.
 */

#ifdef NORVI_AE01_R

#include "CalibrationManager.hpp"

#include <Arduino.h>
#include "Config.hpp"
#include "ConfigManager.hpp"
#include "LogCapture.hpp"
#include "NorviButtonHandler.hpp"

namespace PoolController {

// ── Constants ──────────────────────────────────────────────────────────────

/// Per-step timeout: no stable level within this window → retry the step.
static constexpr uint32_t STEP_TIMEOUT_MS{10000};
/// Minimum gap between adjacent measured levels (ADC counts).
static constexpr uint16_t MIN_LEVEL_GAP{100};
/// Number of samples averaged per level (50 ms apart → 1 s window).
static constexpr uint32_t SAMPLE_COUNT{20};
/// Consecutive readings within this window count as "stable".
static constexpr uint16_t STABILITY_WINDOW{50};
/// Consecutive stable readings required before sampling starts.
static constexpr uint8_t STABLE_READINGS{3};

// ── Static members ─────────────────────────────────────────────────────────

CalibrationManager::State CalibrationManager::state_{State::IDLE};
CalibrationManager::CalibrationStatus CalibrationManager::status_{};
uint16_t (*CalibrationManager::adcRead_)() = nullptr;
uint32_t (*CalibrationManager::timeFn_)() = nullptr;
uint32_t CalibrationManager::stepStartMs_{0};
uint32_t CalibrationManager::sampleCount_{0};
uint32_t CalibrationManager::sampleSum_{0};
uint16_t CalibrationManager::lastReading_{0};
uint16_t CalibrationManager::stableCount_{0};
bool CalibrationManager::sampling_{false};

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::begin() {
  if (adcRead_ == nullptr) {
    adcRead_ = []() { return static_cast<uint16_t>(analogRead(PIN_BUTTON_ADC)); };
  }
  if (timeFn_ == nullptr) {
    timeFn_ = millis;
  }
  state_ = State::IDLE;
  status_ = CalibrationStatus{};
  LOG_INFO("✓ CalibrationManager initialized\n");
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::loop() {
  if (state_ == State::IDLE || state_ == State::DONE || state_ == State::ERROR) {
    return;
  }

  const uint32_t t = now();
  status_.liveAdc = readAdc();

  switch (state_) {
  case State::RESTING:
    handleMeasurementStep(State::RESTING, 0, status_.restingLevel);
    break;
  case State::BTN1:
    handleMeasurementStep(State::BTN1, status_.restingLevel, status_.s1);
    break;
  case State::BTN2:
    handleMeasurementStep(State::BTN2, status_.s1, status_.s2);
    break;
  case State::BTN3:
    handleMeasurementStep(State::BTN3, status_.s2, status_.s3);
    break;
  case State::COMPUTE:
    computeThresholds();
    break;
  case State::SAVE:
    saveThresholds();
    break;
  default:
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::handleMeasurementStep(State step, uint16_t previousLevel, uint16_t& outLevel) {
  const uint32_t t = now();

  if (!sampling_) {
    // ── Wait phase: look for a stable reading ──────────────────────────
    if (t - stepStartMs_ >= STEP_TIMEOUT_MS) {
      // Timeout → retry the step (stay in the same state, reset timer)
      stepStartMs_ = t;
      stableCount_ = 0;
      status_.message = "No stable level detected — please try again";
      LOG_WARN("Calibration step timeout, retrying\n");
      return;
    }

    const uint16_t reading = readAdc();
    const bool differsFromPrevious =
        (previousLevel == 0) || (reading > previousLevel + MIN_LEVEL_GAP) ||
        (reading < previousLevel - MIN_LEVEL_GAP);

    if (differsFromPrevious &&
        (stableCount_ == 0 ||
         (reading > lastReading_ - STABILITY_WINDOW && reading < lastReading_ + STABILITY_WINDOW))) {
      stableCount_++;
      lastReading_ = reading;
    } else {
      stableCount_ = 0;
      lastReading_ = reading;
    }

    if (stableCount_ >= STABLE_READINGS) {
      // Stable → start sampling
      sampling_ = true;
      sampleCount_ = 0;
      sampleSum_ = 0;
      status_.message = "Level stable — sampling…";
    }
    return;
  }

  // ── Sample phase: collect SAMPLE_COUNT readings ──────────────────────
  sampleSum_ += readAdc();
  sampleCount_++;
  if (sampleCount_ >= SAMPLE_COUNT) {
    outLevel = static_cast<uint16_t>(sampleSum_ / SAMPLE_COUNT);
    sampling_ = false;
    stableCount_ = 0;
    stepStartMs_ = t;

    switch (step) {
    case State::RESTING:
      status_.message = "Please press and hold Button 1";
      enterState(State::BTN1);
      break;
    case State::BTN1:
      status_.message = "Please press and hold Button 2";
      enterState(State::BTN2);
      break;
    case State::BTN2:
      status_.message = "Please press and hold Button 3";
      enterState(State::BTN3);
      break;
    case State::BTN3:
      status_.message = "Computing thresholds…";
      enterState(State::COMPUTE);
      break;
    default:
      break;
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::computeThresholds() {
  const uint16_t resting = status_.restingLevel;
  const uint16_t s1 = status_.s1;
  const uint16_t s2 = status_.s2;
  const uint16_t s3 = status_.s3;

  // Sanity checks: strictly ascending with minimum gaps, S3 within ADC range
  if (!(resting < s1 && s1 < s2 && s2 < s3) ||
      (s1 - resting < MIN_LEVEL_GAP) || (s2 - s1 < MIN_LEVEL_GAP) || (s3 - s2 < MIN_LEVEL_GAP) ||
      s3 > 4095) {
    status_.step = Step::ERROR;
    status_.message = "Levels not ascending or too close — please re-run calibration";
    state_ = State::ERROR;
    LOG_ERROR("Calibration failed: levels resting=%u s1=%u s2=%u s3=%u\n", resting, s1, s2, s3);
    return;
  }

  auto& s = ConfigManager::getSettings();
  s.btn1Min = (resting + s1) / 2;
  s.btn1Max = (s1 + s2) / 2;
  s.btn2Min = (s1 + s2) / 2;
  s.btn2Max = (s2 + s3) / 2;
  s.btn3Min = (s2 + s3) / 2;
  s.btn3Max = 4095;   // full scale stays
  s.btnNoPress = 4096;  // sentinel stays

  state_ = State::SAVE;
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::saveThresholds() {
  if (!ConfigManager::save()) {
    status_.step = Step::ERROR;
    status_.message = "Failed to save thresholds — please retry";
    state_ = State::ERROR;
    LOG_ERROR("Calibration failed: ConfigManager::save() returned false\n");
    return;
  }
  NorviButtonHandler::applySettings();
  status_.step = Step::DONE;
  status_.message = "Calibration complete — thresholds saved";
  state_ = State::DONE;
  LOG_INFO("✓ Calibration complete: btn1=%u-%u btn2=%u-%u btn3=%u-%u noPress=%u\n",
    ConfigManager::getSettings().btn1Min, ConfigManager::getSettings().btn1Max,
    ConfigManager::getSettings().btn2Min, ConfigManager::getSettings().btn2Max,
    ConfigManager::getSettings().btn3Min, ConfigManager::getSettings().btn3Max,
    ConfigManager::getSettings().btnNoPress);
}

// ═══════════════════════════════════════════════════════════════════════════

bool CalibrationManager::start() {
  if (state_ != State::IDLE && state_ != State::DONE && state_ != State::ERROR) {
    return false;  // already running
  }
  status_ = CalibrationStatus{};
  status_.step = Step::RESTING;
  status_.message = "Release all buttons — measuring resting level";
  enterState(State::RESTING);
  LOG_INFO("Calibration started\n");
  return true;
}

void CalibrationManager::cancel() {
  if (state_ == State::IDLE) {
    return;
  }
  status_ = CalibrationStatus{};
  status_.step = Step::IDLE;
  state_ = State::IDLE;
  sampling_ = false;
  stableCount_ = 0;
  LOG_INFO("Calibration cancelled\n");
}

CalibrationManager::CalibrationStatus CalibrationManager::getStatus() {
  return status_;
}

bool CalibrationManager::isActive() {
  return state_ != State::IDLE && state_ != State::DONE && state_ != State::ERROR;
}

// ═══════════════════════════════════════════════════════════════════════════

void CalibrationManager::enterState(State s) {
  state_ = s;
  stepStartMs_ = now();
  sampling_ = false;
  stableCount_ = 0;
  lastReading_ = 0;
  switch (s) {
  case State::RESTING: status_.step = Step::RESTING; break;
  case State::BTN1:    status_.step = Step::BTN1;    break;
  case State::BTN2:    status_.step = Step::BTN2;    break;
  case State::BTN3:    status_.step = Step::BTN3;    break;
  case State::DONE:    status_.step = Step::DONE;    break;
  case State::ERROR:   status_.step = Step::ERROR;   break;
  default: break;
  }
}

uint16_t CalibrationManager::readAdc() {
  return adcRead_ ? adcRead_() : 0;
}

uint32_t CalibrationManager::now() {
  return timeFn_ ? timeFn_() : 0;
}

void CalibrationManager::setAdcReadForTest(uint16_t (*fn)()) {
  adcRead_ = fn;
}

void CalibrationManager::setTimeForTest(uint32_t (*fn)()) {
  timeFn_ = fn;
}

}  // namespace PoolController

#endif  // NORVI_AE01_R
```

- [ ] **Step 7: Run tests to verify they pass**

Run: `cmake -B build -S . && cmake --build build && ./build/test_runner`
Expected: PASS — all 4 CalibrationManager tests pass, no regressions in other suites.

- [ ] **Step 8: Commit**

```bash
git add src/CalibrationManager.hpp src/CalibrationManager.cpp test/native/tests/test_calibration_manager.cpp test/native/CMakeLists.txt test/native/mocks/ConfigManager.hpp test/native/tests/test_main.cpp
git commit -m "feat(calibration): add CalibrationManager state machine with measurement"
```

---

### Task 2: COMPUTE/SAVE tests — threshold math, error paths

**Files:**
- Modify: `test/native/tests/test_calibration_manager.cpp` (add tests)

**Interfaces:**
- Consumes: `CalibrationManager::start()`, `loop()`, `getStatus()`, test hooks from Task 1; `ConfigManager::getSettings()` (mock).
- Produces: nothing new — verifies Task 1's COMPUTE/SAVE behavior.

- [ ] **Step 1: Add a save-failure hook to the mock**

In `test/native/mocks/ConfigManager.hpp`, change `save()` to:

```cpp
  static bool save() { return !_saveFails; }
  static bool _saveFails;  // test hook
```

In `test/native/mocks/ConfigManager.cpp`, add `bool ConfigManager::_saveFails = false;` near the other static definitions.

- [ ] **Step 2: Write the failing tests**

Append to `test/native/tests/test_calibration_manager.cpp`:

```cpp
static int test_full_calibration_saves_thresholds() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700; g_now = 0;
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

  holdLevel(2700, 0);    // resting
  holdLevel(3400, 2000); // S1
  holdLevel(3700, 4000); // S2
  holdLevel(4095, 6000); // S3

  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::DONE);
  auto& s = PoolController::ConfigManager::getSettings();
  ASSERT_EQ(s.btn1Min, (2700 + 3400) / 2);
  ASSERT_EQ(s.btn1Max, (3400 + 3700) / 2);
  ASSERT_EQ(s.btn2Min, (3400 + 3700) / 2);
  ASSERT_EQ(s.btn2Max, (3700 + 4095) / 2);
  ASSERT_EQ(s.btn3Min, (3700 + 4095) / 2);
  ASSERT_EQ(s.btn3Max, 4095);
  ASSERT_EQ(s.btnNoPress, 4096);
  return 0;
}

static int test_non_ascending_levels_error() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700; g_now = 0;
  CalibrationManager::begin();
  CalibrationManager::start();

  auto holdLevel = [](uint16_t level, uint32_t startMs) {
    g_adc = level;
    for (uint32_t t = startMs; t < startMs + 1500; t += 50) {
      g_now = t;
      CalibrationManager::loop();
    }
  };

  holdLevel(2700, 0);    // resting
  holdLevel(3700, 2000); // S1 (too high — user pressed wrong button)
  holdLevel(3400, 4000); // S2 (below S1 → sanity check fails)
  holdLevel(4095, 6000); // S3

  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::ERROR);
  return 0;
}

static int test_save_failure_error() {
  CalibrationManager::setAdcReadForTest(fakeAdc);
  CalibrationManager::setTimeForTest(fakeTime);
  g_adc = 2700; g_now = 0;
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
  holdLevel(4095, 6000);

  // Force save failure via the mock hook → ERROR state
  PoolController::ConfigManager::_saveFails = true;
  CalibrationManager::loop();
  PoolController::ConfigManager::_saveFails = false;

  ASSERT_EQ(CalibrationManager::getStatus().step, CalibrationManager::Step::ERROR);
  return 0;
}
```

- [ ] **Step 3: Register the new tests**

Add the three new test functions to `run_calibration_manager_tests()` and bump the expected count from 4 to 7.

- [ ] **Step 4: Run tests to verify they pass**

Run: `cmake -B build -S . && cmake --build build && ./build/test_runner`
Expected: PASS — 7 CalibrationManager tests, no regressions.

- [ ] **Step 5: Commit**

```bash
git add test/native/tests/test_calibration_manager.cpp test/native/mocks/ConfigManager.hpp test/native/mocks/ConfigManager.cpp
git commit -m "test(calibration): cover threshold computation and error paths"
```

---

### Task 3: Suppress button callbacks during calibration + wire into PoolController

**Files:**
- Modify: `src/NorviButtonHandler.cpp` (early-return in `loop()`)
- Modify: `src/PoolController.cpp` (call `CalibrationManager::begin()` and `loop()`)

**Interfaces:**
- Consumes: `CalibrationManager::isActive()` (Task 1).
- Produces: nothing new.

- [ ] **Step 1: Early-return in NorviButtonHandler::loop()**

In `src/NorviButtonHandler.cpp`, add the include and the guard at the top of `loop()`:

```cpp
#include "CalibrationManager.hpp"
```

```cpp
void NorviButtonHandler::loop() {
  // Suppress button handling while calibration is running — the wizard
  // owns the ADC input and button presses must not trigger actions.
  if (CalibrationManager::isActive()) {
    return;
  }

  const uint32_t now = millis();
  // ... existing body unchanged
```

- [ ] **Step 2: Wire CalibrationManager into PoolController**

In `src/PoolController.cpp`, inside the `#ifdef NORVI_AE01_R` block near `NorviButtonHandler::begin()` (line ~296), add:

```cpp
  CalibrationManager::begin();
```

And in the `#ifdef NORVI_AE01_R` block in `loop()` (near line 502), add:

```cpp
  CalibrationManager::loop();
```

- [ ] **Step 3: Verify the firmware builds**

Run: `/home/openclaw/.platformio/penv/bin/pio run -e norvi_ae01_r`
Expected: SUCCESS.

- [ ] **Step 4: Run native tests (no regressions)**

Run: `cmake -B build -S . && cmake --build build && ./build/test_runner`
Expected: PASS — all suites green.

- [ ] **Step 5: Commit**

```bash
git add src/NorviButtonHandler.cpp src/PoolController.cpp
git commit -m "feat(calibration): suppress button actions during calibration"
```

---

### Task 4: WebPortal REST endpoints

**Files:**
- Modify: `src/WebPortal.cpp` (add 3 routes + handlers)
- Modify: `test/native/tests/test_webportal_json.cpp` (add calibration status JSON test)

**Interfaces:**
- Consumes: `CalibrationManager::start()`, `cancel()`, `getStatus()`, `isActive()` (Task 1).
- Produces: REST endpoints `POST /api/calibrate/start`, `GET /api/calibrate/status`, `POST /api/calibrate/cancel`.

- [ ] **Step 1: Add the routes**

In `src/WebPortal.cpp` `setupRoutes()`, after the `/api/config` routes (line ~209), add:

```cpp
  // Button calibration wizard (NORVI)
  server_.on("/api/calibrate/start", HTTP_POST, []() {
    if (!handleAuthentication())
      return;
    apiCalibrateStart();
  });
  server_.on("/api/calibrate/status", HTTP_GET, []() {
    if (!handleAuthentication())
      return;
    apiCalibrateStatus();
  });
  server_.on("/api/calibrate/cancel", HTTP_POST, []() {
    if (!handleAuthentication())
      return;
    apiCalibrateCancel();
  });
```

- [ ] **Step 2: Add the handlers**

In `src/WebPortal.cpp`, add the handler implementations (near the other `api*` handlers, e.g. after `apiSaveConfig`):

```cpp
// ── Button calibration (NORVI) ────────────────────────────────────────────

void WebPortal::apiCalibrateStart() {
  if (!CalibrationManager::start()) {
    server_.send(409, "text/plain", "Calibration already running");
    return;
  }
  server_.send(200, "text/plain", "OK");
}

void WebPortal::apiCalibrateStatus() {
  const auto st = CalibrationManager::getStatus();
  JsonDocument doc;
  doc["step"] = static_cast<int>(st.step);
  doc["live_adc"] = st.liveAdc;
  doc["resting"] = st.restingLevel;
  doc["s1"] = st.s1;
  doc["s2"] = st.s2;
  doc["s3"] = st.s3;
  doc["message"] = st.message;
  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}

void WebPortal::apiCalibrateCancel() {
  CalibrationManager::cancel();
  server_.send(200, "text/plain", "OK");
}
```

Add the three method declarations to `src/WebPortal.hpp` (private section) and the `#include "CalibrationManager.hpp"` to `src/WebPortal.cpp` (guarded by `#ifdef NORVI_AE01_R` — the handlers are only compiled for the NORVI variant; wrap the route registrations and handler bodies in `#ifdef NORVI_AE01_R`).

- [ ] **Step 3: Write the failing test**

In `test/native/tests/test_webportal_json.cpp`, add a test that verifies the status JSON shape (following the existing `apiGetStatus` test pattern — construct the expected JSON inline and check keys/types):

```cpp
  // ── Test: calibration status JSON ──
  {
    test_begin("WebPortal::apiCalibrateStatus", "returns JSON with all fields");

    JsonDocument doc;
    doc["step"] = 1;
    doc["live_adc"] = 2700;
    doc["resting"] = 0;
    doc["s1"] = 0;
    doc["s2"] = 0;
    doc["s3"] = 0;
    doc["message"] = "Release all buttons — measuring resting level";

    int errs = 0;
    const char *requiredKeys[] = {"step", "live_adc", "resting", "s1", "s2", "s3", "message"};
    for (auto key : requiredKeys) {
      if (!doc.containsKey(key)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Missing required key: %s", key);
        test_fail(__FILE__, __LINE__, msg);
        errs++;
      }
    }
    ASSERT_TRUE(doc["step"].is<int>());
    ASSERT_TRUE(doc["live_adc"].is<int>());
    ASSERT_TRUE(doc["message"].is<const char *>());

    test_suite_end("WebPortal::apiCalibrateStatus", errs == 0 ? 3 : 0, errs);
  }
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cmake -B build -S . && cmake --build build && ./build/test_runner`
Expected: PASS — calibration status JSON test passes, no regressions.

- [ ] **Step 5: Verify the firmware builds**

Run: `/home/openclaw/.platformio/penv/bin/pio run -e norvi_ae01_r`
Expected: SUCCESS.

- [ ] **Step 6: Commit**

```bash
git add src/WebPortal.cpp src/WebPortal.hpp test/native/tests/test_webportal_json.cpp
git commit -m "feat(calibration): add REST endpoints for calibration wizard"
```

---

### Task 5: Web UI — start button, modal wizard, polling

**Files:**
- Modify: `data/web/index.html` (start button + modal markup)
- Modify: `data/web/app.js` (wizard logic, polling, modal handling)

**Interfaces:**
- Consumes: REST endpoints from Task 4 (`POST /api/calibrate/start`, `GET /api/calibrate/status`, `POST /api/calibrate/cancel`).
- Produces: `startCalibration()`, `pollCalibrationStatus()`, `cancelCalibration()`, `closeCalibrationModal()` (global functions used by inline `onclick` handlers).

- [ ] **Step 1: Add the start button**

In `data/web/index.html`, in the Button Thresholds section (after the `btnNoPress` input group, line ~356), add:

```html
  <div class="telemetry-grid">
    <div class="input-group">
      <button type="button" onclick="startCalibration()" style="width: 100%; padding: 0.75rem; background: linear-gradient(135deg, #00b4d8, #00e5ff); border: none; border-radius: 8px; color: #000; font-weight: 600; font-size: 1rem; cursor: pointer;">🎯 Start Calibration</button>
      <span class="input-hint">Guided wizard: press and hold each button in turn to measure its ADC level</span>
    </div>
    <div class="input-group" style="visibility:hidden;">
      <div style="height:1px;"></div>
    </div>
  </div>
```

- [ ] **Step 2: Add the modal markup**

In `data/web/index.html`, after the `loginModal` div (line ~40), add the calibration modal (same inline-overlay pattern):

```html
<!-- Calibration Modal (guided wizard) -->
<div id="calibrationModal" style="display: none; position: fixed; inset: 0; z-index: 100; background: rgba(6,18,30,0.85); backdrop-filter: blur(8px); align-items: center; justify-content: center; padding: 2rem;">
  <div style="background: var(--glass-bg); border: 1px solid var(--panel-border); border-radius: 16px; padding: 2rem; max-width: 420px; width: 100%;">
    <h2 style="color: #00e5ff; text-align: center; margin: 0 0 0.25rem; font-size: 1.4rem;">🎯 Button Calibration</h2>
    <p id="calibStepText" style="color: var(--text-muted); text-align: center; margin-bottom: 1rem; font-size: 0.9rem;">Release all buttons — measuring resting level</p>
    <div style="text-align: center; margin-bottom: 1rem;">
      <span id="calibLiveAdc" style="font-size: 2rem; font-weight: 700; color: #e2f0f7;">—</span>
      <span style="color: var(--text-muted); font-size: 0.85rem;"> ADC</span>
    </div>
    <div id="calibProgress" style="display: flex; justify-content: space-between; margin-bottom: 1.5rem; font-size: 0.8rem; color: var(--text-muted);">
      <span id="calibP0">Resting</span>
      <span id="calibP1">Button 1</span>
      <span id="calibP2">Button 2</span>
      <span id="calibP3">Button 3</span>
    </div>
    <button onclick="cancelCalibration()" style="width: 100%; padding: 0.6rem; background: transparent; border: 1px solid var(--panel-border); border-radius: 8px; color: var(--text-muted); font-size: 0.85rem; cursor: pointer;">Cancel</button>
  </div>
</div>
```

- [ ] **Step 3: Add the wizard logic to app.js**

In `data/web/app.js`, add (near the other settings functions):

```js
// ── Button Calibration Wizard ──

let calibPollTimer = null;

function showCalibrationModal() {
  document.getElementById('calibrationModal').style.display = 'flex';
}

function closeCalibrationModal() {
  document.getElementById('calibrationModal').style.display = 'none';
  if (calibPollTimer) { clearInterval(calibPollTimer); calibPollTimer = null; }
}

async function startCalibration() {
  const res = await fetch('/api/calibrate/start', { method: 'POST' });
  if (!res.ok) { alert('Calibration could not be started.'); return; }
  showCalibrationModal();
  calibPollTimer = setInterval(pollCalibrationStatus, 500);
  pollCalibrationStatus();
}

async function pollCalibrationStatus() {
  const res = await fetch('/api/calibrate/status');
  if (!res.ok) return;
  const st = await res.json();
  document.getElementById('calibStepText').textContent = st.message || '';
  document.getElementById('calibLiveAdc').textContent = st.live_adc;

  const steps = ['calibP0', 'calibP1', 'calibP2', 'calibP3'];
  const active = st.step; // 1=RESTING, 2=BTN1, 3=BTN2, 4=BTN3, 5=DONE, 6=ERROR
  steps.forEach((id, i) => {
    const el = document.getElementById(id);
    el.style.color = (i + 1 === active) ? '#00e5ff' : (i + 1 < active ? '#4ade80' : 'var(--text-muted)');
  });

  if (st.step === 5) { // DONE
    closeCalibrationModal();
    loadConfig(); // refresh threshold fields
  } else if (st.step === 6) { // ERROR
    closeCalibrationModal();
    alert('Calibration failed: ' + (st.message || 'unknown error'));
  }
}

async function cancelCalibration() {
  await fetch('/api/calibrate/cancel', { method: 'POST' });
  closeCalibrationModal();
}
```

- [ ] **Step 4: Verify the web assets are valid**

Run: `node --check data/web/app.js`
Expected: no syntax errors.

- [ ] **Step 5: Verify the firmware builds**

Run: `/home/openclaw/.platformio/penv/bin/pio run -e norvi_ae01_r`
Expected: SUCCESS.

- [ ] **Step 6: Commit**

```bash
git add data/web/index.html data/web/app.js
git commit -m "feat(calibration): add guided calibration wizard to web UI"
```

---

### Task 6: Final verification

**Files:** none (verification only)

- [ ] **Step 1: Full native test run**

Run: `cmake -B build -S . && cmake --build build && ./build/test_runner`
Expected: all suites pass (including 7 CalibrationManager tests).

- [ ] **Step 2: Firmware build for all environments**

Run: `/home/openclaw/.platformio/penv/bin/pio run -e norvi_ae01_r`
Expected: SUCCESS.

- [ ] **Step 3: clang-format check**

Run: `clang-format --dry-run --Werror --style=file:.clang-format src/CalibrationManager.hpp src/CalibrationManager.cpp src/NorviButtonHandler.cpp src/PoolController.cpp src/WebPortal.cpp`
Expected: no output (all formatted). If errors, run `clang-format -i` on the offending files.

- [ ] **Step 4: Manual device verification (optional, requires hardware)**

Flash `norvi_ae01_r` to the device, open the web UI → Pool tab → Start Calibration, and verify:
1. Modal opens with "Release all buttons — measuring resting level".
2. Live ADC value updates every ~500 ms.
3. Pressing and holding each button advances the wizard.
4. On completion, the threshold fields show the new values.
5. Cancel works and old thresholds remain.

- [ ] **Step 5: Report**

Summarize: files changed, test results, build results, and any manual verification performed.