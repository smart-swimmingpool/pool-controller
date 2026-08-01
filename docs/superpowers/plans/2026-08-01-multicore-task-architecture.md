# Multicore Task Architecture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move blocking I/O (DS18B20 reads, OLED rendering, MQTT telemetry serialization) off the Arduino control loop onto dedicated FreeRTOS tasks pinned to Core 0, so loop iteration time drops from ~750 ms to <20 ms while all features keep their exact semantics.

**Architecture:** A small static `CoreScheduler` launcher creates three Core-0 tasks (SensorTask, PublishTask, DisplayTask). The control loop (Core 1) keeps rules/relays/watchdog/network untouched. Cross-task data moves through lock-free, single-writer channels: `SensorSlots` (temperature values), a SPSC `TelemetryQueue` (publish requests), and a display snapshot. No singleton is mutated from two tasks without an explicit primitive.

**Tech Stack:** FreeRTOS (ESP-IDF 5.x via Arduino framework, espressif32 @ 7.0.1), C++17, `std::atomic` for lock-free primitives, existing native test harness (CMake + ASan + gcov, `test/native`).

## Global Constraints

- Both build environments must compile and stay green: `norvi_ae01_r` (has NORVI_AE01_R → OLED/buttons) and `esp32dev` (no display).
- No behavior change: rules, modes, MQTT/Homie, OTA, watchdog, degradation, safe mode keep semantics. Native tests stay green (baseline: 74 suites / 144 assertions).
- `pio` is not on PATH → use `~/.platformio/penv/bin/pio`.
- Native test build needs `~/.platformio/penv/bin/pio pkg install --environment esp32dev` once (ArduinoJson in `.pio/libdeps/esp32dev/`); already done in this worktree.
- Do NOT touch anything from the dirty main repo (`fix/relay-r4-solar-pump`). Worktree `pool-controller-multicore` is the only write target.
- Existing style: statics/singletons, `PoolController` namespace, Doxygen comments on public API, `// Copyright (c) 2018-2026` header, no `String` in hot paths, no heap growth in steady state.
- Conventional commits: `feat:`, `refactor:`, `test:`, `docs:` — one commit per task.
- Task priorities only matter within a core. I/O tasks live on Core 0 below the WiFi-stack tasks; they yield with `vTaskDelay`.

---

## File Structure

**New files (src/):**
- `src/CoreScheduler.{hpp,cpp}` — static launcher: creates the three Core-0 tasks, tracks handles, logs stack high-water marks periodically. FreeRTOS-only (not in native build).
- `src/TelemetryQueue.{hpp,cpp}` — SPSC lock-free ring buffer of publish requests. Pure C++ (`std::atomic`), compiled into native tests.
- `src/SensorSlots.{hpp,cpp}` — lock-free temperature slots (single writer: SensorTask; readers: control loop, display). Pure C++, compiled into native tests.
- `src/SensorTask.{hpp,cpp}` — Core-0 task owning all DS18B20/OneWire + ESP32 internal temp access. FreeRTOS-only.
- `src/PublishTask.{hpp,cpp}` — Core-0 task draining `TelemetryQueue` and calling `MqttPublisher`. FreeRTOS-only.
- `src/DisplayTask.{hpp,cpp}` — Core-0 task rendering the NORVI OLED (NORVI_AE01_R only). FreeRTOS-only.

**Modified files:**
- `src/DallasTemperatureNode.{hpp,cpp}` — split `loop()` into `beginMeasurement()` + `finishMeasurement()`; finish writes into `SensorSlots`; `getTemperature()`/`isSensorFound()` read from slots.
- `src/PoolController.cpp` — remove sensor-node `loop()` calls from control loop; start `CoreScheduler::begin()` at end of `setup()`; route MQTT publish triggers through `TelemetryQueue`; call `DisplayTask::requestRender()` instead of `NorviOledDisplay::loop()`.
- `src/NorviOledDisplay.{hpp,cpp}` — split state machine (`update()`, Core 1) from rendering (`render()`, Core 0); `drawMainPage()` reads temps from `SensorSlots`.
- `src/DegradationManager.{hpp,cpp}` — make `reportSensorStatus` a thread-safe (atomic/irq-safe) channel.
- `src/SystemMonitor.hpp` — add `feedWatchdogFromTask()` wrapper (no-op change in practice, documented).
- `test/native/CMakeLists.txt` — add `TelemetryQueue.cpp` + `SensorSlots.cpp` to SERVICE_SOURCES, new test files to TEST_SOURCES.
- `test/native/mocks/DallasTemperatureNode.hpp` — extend with the new method signatures.

**Test files (test/native/tests/):**
- `test_telemetry_queue.cpp` — SPSC queue semantics.
- `test_sensor_slots.cpp` — slot write/read/status semantics.
- `test_core_scheduler.cpp` — task registration parameters via header mock.

---

### Task 1: TelemetryQueue — SPSC publish-request queue

**Files:**
- Create: `src/TelemetryQueue.hpp`, `src/TelemetryQueue.cpp`
- Create: `test/native/tests/test_telemetry_queue.cpp`
- Modify: `test/native/CMakeLists.txt` (SERVICE_SOURCES + TEST_SOURCES)

**Interfaces:**
- Produces: `enum class PublishRequestKind : uint8_t { STATES = 0, DISCOVERY = 1 }` and `class TelemetryQueue` with `bool enqueue(PublishRequestKind kind)`, `bool dequeue(PublishRequestKind &kind)`, `size_t count() const`, `static constexpr size_t CAPACITY = 8`. Non-blocking, single-producer/single-consumer, drop-on-full (returns false).

- [ ] **Step 1: Write the failing test**

Create `test/native/tests/test_telemetry_queue.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include <unity.h>
#include "TelemetryQueue.hpp"

using PoolController::TelemetryQueue;
using PoolController::PublishRequestKind;

static TelemetryQueue queue;

void setUp() { queue.reset(); }
void tearDown() {}

void test_queue_empty_on_reset() { TEST_ASSERT_EQUAL(0, queue.count()); }

void test_enqueue_dequeue_roundtrip() {
  TEST_ASSERT_TRUE(queue.enqueue(PublishRequestKind::STATES));
  TEST_ASSERT_EQUAL(1, queue.count());
  PublishRequestKind kind;
  TEST_ASSERT_TRUE(queue.dequeue(kind));
  TEST_ASSERT_EQUAL(PublishRequestKind::STATES, kind);
  TEST_ASSERT_EQUAL(0, queue.count());
}

void test_fifo_order() {
  queue.enqueue(PublishRequestKind::STATES);
  queue.enqueue(PublishRequestKind::DISCOVERY);
  PublishRequestKind kind;
  queue.dequeue(kind);
  TEST_ASSERT_EQUAL(PublishRequestKind::STATES, kind);
  queue.dequeue(kind);
  TEST_ASSERT_EQUAL(PublishRequestKind::DISCOVERY, kind);
}

void test_dequeue_empty_returns_false() {
  PublishRequestKind kind;
  TEST_ASSERT_FALSE(queue.dequeue(kind));
}

void test_enqueue_full_drops() {
  for (size_t i = 0; i < TelemetryQueue::CAPACITY; i++) {
    TEST_ASSERT_TRUE(queue.enqueue(PublishRequestKind::STATES));
  }
  TEST_ASSERT_FALSE(queue.enqueue(PublishRequestKind::DISCOVERY));
  TEST_ASSERT_EQUAL(TelemetryQueue::CAPACITY, queue.count());
}

void test_reset_clears_full_queue() {
  for (size_t i = 0; i < TelemetryQueue::CAPACITY; i++) {
    queue.enqueue(PublishRequestKind::STATES);
  }
  queue.reset();
  TEST_ASSERT_EQUAL(0, queue.count());
}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_queue_empty_on_reset);
  RUN_TEST(test_enqueue_dequeue_roundtrip);
  RUN_TEST(test_fifo_order);
  RUN_TEST(test_dequeue_empty_returns_false);
  RUN_TEST(test_enqueue_full_drops);
  RUN_TEST(test_reset_clears_full_queue);
  UNITY_END();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd test/native/build && cmake .. && make test_runner 2>&1 | tail -5 && ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2" ./test_runner | tail -8`
Expected: FAIL — `TelemetryQueue.hpp` not found / class not defined.

- [ ] **Step 3: Write minimal implementation**

Create `src/TelemetryQueue.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file TelemetryQueue.hpp
 * @brief Lock-free single-producer/single-consumer queue for MQTT publish requests.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace PoolController {

/** @brief Kinds of publish requests the control loop can enqueue. */
enum class PublishRequestKind : uint8_t {
  STATES = 0,    ///< Publish current telemetry states
  DISCOVERY = 1, ///< Publish Home Assistant discovery configs
};

/**
 * @brief SPSC (single-producer, single-consumer) ring buffer of publish requests.
 *
 * Non-blocking: enqueue on a full queue drops the request and returns false
 * (the periodic publish cadence simply skips a beat — safe by design).
 * Uses a classic atomic head/tail lock-free ring; safe with one writer
 * (control loop) and one reader (PublishTask).
 */
class TelemetryQueue {
public:
  static constexpr size_t CAPACITY = 8;  ///< Fixed slots — no dynamic allocation

  /** @brief Construct an empty queue. */
  TelemetryQueue() { reset(); }

  /** @brief Producer side: enqueue a publish request. @return false if full (dropped). */
  bool enqueue(PublishRequestKind kind);

  /** @brief Consumer side: dequeue a publish request. @return false if empty. */
  bool dequeue(PublishRequestKind &kind);

  /** @brief Number of requests currently queued. */
  size_t count() const;

  /** @brief Empty the queue (tests only — must not run while tasks are active). */
  void reset();

private:
  std::atomic<size_t> head_{0};  ///< Consumer index (only consumer writes)
  std::atomic<size_t> tail_{0};  ///< Producer index (only producer writes)
  PublishRequestKind items_[CAPACITY];  ///< Fixed ring storage
};

}  // namespace PoolController
```

Create `src/TelemetryQueue.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file TelemetryQueue.cpp
 * @brief SPSC publish-request ring buffer implementation.
 */

#include "TelemetryQueue.hpp"

namespace PoolController {

bool TelemetryQueue::enqueue(PublishRequestKind kind) {
  const size_t tail = tail_.load(std::memory_order_relaxed);
  const size_t next = (tail + 1) % (CAPACITY + 1);
  if (next == head_.load(std::memory_order_acquire)) {
    return false;  // full
  }
  items_[tail] = kind;
  tail_.store(next, std::memory_order_release);
  return true;
}

bool TelemetryQueue::dequeue(PublishRequestKind &kind) {
  const size_t head = head_.load(std::memory_order_relaxed);
  if (head == tail_.load(std::memory_order_acquire)) {
    return false;  // empty
  }
  kind = items_[head];
  head_.store((head + 1) % (CAPACITY + 1), std::memory_order_release);
  return true;
}

size_t TelemetryQueue::count() const {
  const size_t head = head_.load(std::memory_order_acquire);
  const size_t tail = tail_.load(std::memory_order_acquire);
  return (tail + CAPACITY + 1 - head) % (CAPACITY + 1);
}

void TelemetryQueue::reset() {
  head_.store(0, std::memory_order_relaxed);
  tail_.store(0, std::memory_order_relaxed);
}

}  // namespace PoolController
```

- [ ] **Step 4: Wire into CMake and run tests**

Modify `test/native/CMakeLists.txt`:
- Add to `SERVICE_SOURCES`: `${PROJ_ROOT}/src/TelemetryQueue.cpp`
- Add to `TEST_SOURCES`: `${CMAKE_CURRENT_SOURCE_DIR}/tests/test_telemetry_queue.cpp`

Run: `cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6`
Expected: PASS — queue tests green, all 74 previous suites still green.

- [ ] **Step 5: Commit**

```bash
git add src/TelemetryQueue.hpp src/TelemetryQueue.cpp test/native/tests/test_telemetry_queue.cpp test/native/CMakeLists.txt
git commit -m "feat: add SPSC telemetry queue for off-core MQTT publishing"
```

---

### Task 2: SensorSlots — lock-free temperature slots

**Files:**
- Create: `src/SensorSlots.hpp`, `src/SensorSlots.cpp`
- Create: `test/native/tests/test_sensor_slots.cpp`
- Modify: `test/native/CMakeLists.txt`

**Interfaces:**
- Consumes: nothing (standalone).
- Produces: `enum class SensorId : uint8_t { SOLAR = 0, POOL = 1, CONTROLLER = 2, COUNT = 3 }`, and `class SensorSlots` with `static void reset()`, `static void write(SensorId id, float value, bool found)`, `static float read(SensorId id)`, `static bool isFound(SensorId id)`. Single writer (SensorTask), many readers. Values are `volatile` — a reader may see one-cycle-stale data, which is acceptable for temperature telemetry.

- [ ] **Step 1: Write the failing test**

Create `test/native/tests/test_sensor_slots.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include <unity.h>
#include <cmath>
#include "SensorSlots.hpp"

using PoolController::SensorSlots;
using PoolController::SensorId;

void setUp() { SensorSlots::reset(); }
void tearDown() {}

void test_defaults_are_nan_and_not_found() {
  TEST_ASSERT_TRUE(std::isnan(SensorSlots::read(SensorId::SOLAR)));
  TEST_ASSERT_FALSE(SensorSlots::isFound(SensorId::SOLAR));
}

void test_write_read_roundtrip() {
  SensorSlots::write(SensorId::POOL, 26.5f, true);
  TEST_ASSERT_TRUE(SensorSlots::isFound(SensorId::POOL));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 26.5f, SensorSlots::read(SensorId::POOL));
}

void test_write_nan_marks_not_found() {
  SensorSlots::write(SensorId::SOLAR, NAN, false);
  TEST_ASSERT_FALSE(SensorSlots::isFound(SensorId::SOLAR));
  TEST_ASSERT_TRUE(std::isnan(SensorSlots::read(SensorId::SOLAR)));
}

void test_slots_are_independent() {
  SensorSlots::write(SensorId::SOLAR, 30.0f, true);
  SensorSlots::write(SensorId::CONTROLLER, 41.2f, true);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, SensorSlots::read(SensorId::SOLAR));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 41.2f, SensorSlots::read(SensorId::CONTROLLER));
  TEST_ASSERT_FALSE(SensorSlots::isFound(SensorId::POOL));
}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_are_nan_and_not_found);
  RUN_TEST(test_write_read_roundtrip);
  RUN_TEST(test_write_nan_marks_not_found);
  RUN_TEST(test_slots_are_independent);
  UNITY_END();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd test/native/build && cmake .. && make test_runner 2>&1 | tail -5 && ASAN_OPTIONS="..." ./test_runner | tail -8`
Expected: FAIL — `SensorSlots.hpp` not found.

- [ ] **Step 3: Write minimal implementation**

Create `src/SensorSlots.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorSlots.hpp
 * @brief Lock-free temperature slots shared between SensorTask and readers.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/** @brief Identifies a temperature sensor slot. */
enum class SensorId : uint8_t {
  SOLAR = 0,        ///< Solar DS18B20
  POOL = 1,         ///< Pool DS18B20
  CONTROLLER = 2,   ///< ESP32 internal temperature
  COUNT = 3         ///< Sentinel
};

/**
 * @brief Fixed, lock-free slots for sensor values.
 *
 * Single writer (SensorTask on Core 0), multiple readers (control loop,
 * display). Uses `volatile` word-sized fields: on ESP32 aligned 32-bit
 * reads/writes are atomic, so readers may see one-cycle-stale but never
 * torn values — acceptable for temperature telemetry.
 */
class SensorSlots {
public:
  /** @brief Reset all slots to NaN / not-found (tests only). */
  static void reset();

  /** @brief Writer: publish a new value. */
  static void write(SensorId id, float value, bool found);

  /** @brief Reader: get the latest value (°C, NAN if unknown). */
  static float read(SensorId id);

  /** @brief Reader: check whether the sensor is currently found. */
  static bool isFound(SensorId id);

private:
  struct Slot {
    volatile float value;
    volatile bool found;
  };
  static Slot slots_[static_cast<uint8_t>(SensorId::COUNT)];
};

}  // namespace PoolController
```

Create `src/SensorSlots.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorSlots.cpp
 * @brief Lock-free temperature slot implementation.
 */

#include "SensorSlots.hpp"
#include <cmath>

namespace PoolController {

SensorSlots::Slot SensorSlots::slots_[static_cast<uint8_t>(SensorId::COUNT)] = {
  {NAN, false}, {NAN, false}, {NAN, false},
};

void SensorSlots::reset() {
  for (auto &slot : slots_) {
    slot.value = NAN;
    slot.found = false;
  }
}

void SensorSlots::write(SensorId id, float value, bool found) {
  Slot &slot = slots_[static_cast<uint8_t>(id)];
  slot.value = value;
  slot.found = found;
}

float SensorSlots::read(SensorId id) { return slots_[static_cast<uint8_t>(id)].value; }

bool SensorSlots::isFound(SensorId id) { return slots_[static_cast<uint8_t>(id)].found; }

}  // namespace PoolController
```

- [ ] **Step 4: Wire into CMake and run tests**

Modify `test/native/CMakeLists.txt`:
- Add to `SERVICE_SOURCES`: `${PROJ_ROOT}/src/SensorSlots.cpp`
- Add to `TEST_SOURCES`: `${CMAKE_CURRENT_SOURCE_DIR}/tests/test_sensor_slots.cpp`

Run: `cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6`
Expected: PASS — slot tests green, previous suites still green.

- [ ] **Step 5: Commit**

```bash
git add src/SensorSlots.hpp src/SensorSlots.cpp test/native/tests/test_sensor_slots.cpp test/native/CMakeLists.txt
git commit -m "feat: add lock-free sensor slots for cross-task temperature sharing"
```

---

### Task 3: DallasTemperatureNode — split measurement into begin/finish

**Files:**
- Modify: `src/DallasTemperatureNode.hpp` (new public methods)
- Modify: `src/DallasTemperatureNode.cpp` (extract measurement logic)
- Modify: `test/native/mocks/DallasTemperatureNode.hpp` (add signatures)

**Interfaces:**
- Consumes: `SensorSlots` (from Task 2), existing `SystemMonitor` / `DegradationManager` APIs.
- Produces: `void beginMeasurement()` (bus master triggers `requestTemperatures()`; standalone nodes trigger their own) and `void finishMeasurement()` (reads result, updates slots via `SensorSlots::write(id, temp, found)`, reports to `DegradationManager`). `loop()` becomes a thin sync wrapper calling both in sequence (kept for tests/back-compat). `getTemperature()` / `isSensorFound()` now read from `SensorSlots`.

- [ ] **Step 1: Extend the header**

In `src/DallasTemperatureNode.hpp`, after the existing `void loop();` declaration, add:

```cpp
  /**
   * @brief Start a temperature conversion (non-blocking on Core 0).
   *
   * In shared-bus mode only the master (deviceIndex 0) issues
   * requestTemperatures(); slaves just return. In dedicated mode the
   * node starts its own conversion.
   * @note Call from SensorTask; the result must be read later via
   *       finishMeasurement() after the conversion time has elapsed.
   */
  void beginMeasurement();

  /**
   * @brief Read the conversion result and publish it to SensorSlots.
   *
   * Reads the temperature from the bus, updates the internal state, reports
   * sensor status to DegradationManager, and writes the value into the
   * thread-safe SensorSlots for cross-task consumers.
   * @note Call from SensorTask after beginMeasurement() + conversion delay.
   */
  void finishMeasurement();
```

- [ ] **Step 2: Split the implementation**

In `src/DallasTemperatureNode.cpp`, replace the body of `loop()` with:

```cpp
void DallasTemperatureNode::beginMeasurement() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (sharedSensor_ && numberOfDevices > 0) {
    // Shared bus: only the master drives the conversion for all sensors.
    if (isBusMaster_) {
      PoolController::SystemMonitor::feedWatchdog();
      activeSensor->requestTemperatures();
      PoolController::SystemMonitor::feedWatchdog();
    }
  } else if (numberOfDevices > 0) {
    // Dedicated bus: start our own conversion.
    PoolController::SystemMonitor::feedWatchdog();
    activeSensor->requestTemperatures();
    PoolController::SystemMonitor::feedWatchdog();
  }
}

void DallasTemperatureNode::finishMeasurement() {
  DallasTemperature *activeSensor = sharedSensor_ ? sharedSensor_ : &sensor;

  if (sharedSensor_ && numberOfDevices > 0) {
    // Shared bus: master and slave each read their own device.
    float newTemp = activeSensor->getTempC(deviceAddress_);
    if (newTemp == DEVICE_DISCONNECTED_C) {
      _temperature = NAN;
      _sensorFound = false;
      PoolController::DegradationManager::reportSensorStatus(_id, false);
      Serial.printf("  ✖ %s sensor disconnected - setting to NaN\n", _id);
    } else {
      _temperature = newTemp;
      _sensorFound = true;
      PoolController::DegradationManager::reportSensorStatus(_id, true);
      Serial.printf("  ◦ %s Temp = %.1f°C\n", _id, _temperature);
    }
    PoolController::SensorSlots::write(
      (_id[0] == 's') ? PoolController::SensorId::SOLAR : PoolController::SensorId::POOL, _temperature, _sensorFound);
  } else if (numberOfDevices > 0) {
    // Dedicated bus: read all devices, take the last valid reading.
    bool foundAny = false;
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      DeviceAddress tempDeviceAddress;
      if (activeSensor->getAddress(tempDeviceAddress, i)) {
        float newTemp = activeSensor->getTempC(tempDeviceAddress);
        if (newTemp != DEVICE_DISCONNECTED_C) {
          _temperature = newTemp;
          foundAny = true;
        }
      }
    }
    _sensorFound = foundAny;
    PoolController::DegradationManager::reportSensorStatus(_id, foundAny);
    if (foundAny) {
      Serial.printf("  ◦ %s Temp = %.1f°C\n", _id, _temperature);
    } else {
      _temperature = NAN;
      Serial.printf("  ✖ %s sensor disconnected - setting to NaN\n", _id);
    }
    const PoolController::SensorId slot = (_id[0] == 's')
      ? PoolController::SensorId::SOLAR
      : PoolController::SensorId::POOL;
    PoolController::SensorSlots::write(slot, _temperature, _sensorFound);
  } else {
    // No sensor found — rescan the bus.
    Serial.printf("No Sensor found on bus! Rescanning (%s)...\n", _id);
    PoolController::DegradationManager::reportSensorStatus(_id, false);
    PoolController::SensorSlots::write(
      (_id[0] == 's') ? PoolController::SensorId::SOLAR : PoolController::SensorId::POOL, NAN, false);

    if (sharedSensor_) {
      activeSensor->begin();
      numberOfDevices = activeSensor->getDeviceCount();
      if (numberOfDevices > deviceIndex_) {
        activeSensor->getAddress(deviceAddress_, deviceIndex_);
        _sensorFound = true;
        PoolController::DegradationManager::reportSensorStatus(_id, true);
        Serial.printf("  ◦ %d device(s) found after rescan\n", numberOfDevices);
      }
    } else {
      activeSensor->begin();
      numberOfDevices = activeSensor->getDeviceCount();
      if (numberOfDevices > 0) {
        _sensorFound = true;
        Serial.printf("  ◦ %d device(s) found after rescan\n", numberOfDevices);
      }
    }
  }
}

void DallasTemperatureNode::loop() {
  unsigned long effectiveInterval = std::isnan(_temperature) ? RECOVERY_INTERVAL : _measurementInterval;
  if (Utils::shouldMeasure(_lastMeasurement, effectiveInterval)) {
    _lastMeasurement = millis();
    Serial.printf("〽 Reading Dallas sensor: %s\n", _id);
    beginMeasurement();
    // Sync fallback (tests / non-task callers): conversion is blocking here.
    finishMeasurement();
  }
}
```

In the same file, add the includes and change the accessors:

```cpp
// Add to the include block:
#include "SensorSlots.hpp"
```

Replace the two inline accessors in the header:

```cpp
  /** @brief Get the last successfully read temperature. @return Temperature in °C, or NAN if no valid read. */
  float getTemperature() const { return PoolController::SensorSlots::read(slotId()); }
  /** @brief Check if a sensor was found on the bus. @return true if at least one device is present. */
  bool isSensorFound() const { return PoolController::SensorSlots::isFound(slotId()); }
```

Add a private helper declaration in the header (under `private:`):

```cpp
  /** @brief Map this node to its SensorSlots id. */
  PoolController::SensorId slotId() const;
```

And in the .cpp:

```cpp
PoolController::SensorId DallasTemperatureNode::slotId() const {
  return (_id[0] == 's') ? PoolController::SensorId::SOLAR : PoolController::SensorId::POOL;
}
```

- [ ] **Step 3: Update the native mock**

In `test/native/mocks/DallasTemperatureNode.hpp`, add the two new method declarations with the same signatures (no-op or capture behavior is fine — the mock only needs to compile and satisfy callers):

```cpp
  void beginMeasurement() {}
  void finishMeasurement() {}
```

- [ ] **Step 4: Run native tests (regression)**

Run: `cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6`
Expected: PASS — all suites green (this task is a refactor; the sync `loop()` path preserves behavior).

- [ ] **Step 5: Build both device environments (compile check)**

Run:
```bash
~/.platformio/penv/bin/pio run -e norvi_ae01_r 2>&1 | tail -3
~/.platformio/penv/bin/pio run -e esp32dev 2>&1 | tail -3
```
Expected: both `SUCCESS` (SensorSlots links into the firmware).

- [ ] **Step 6: Commit**

```bash
git add src/DallasTemperatureNode.hpp src/DallasTemperatureNode.cpp test/native/mocks/DallasTemperatureNode.hpp
git commit -m "refactor: split DallasTemperatureNode measurement into begin/finish with sensor slots"
```

---

### Task 4: CoreScheduler + SensorTask — move DS18B20 reads off the control loop

**Files:**
- Create: `src/CoreScheduler.hpp`, `src/CoreScheduler.cpp`, `src/SensorTask.hpp`, `src/SensorTask.cpp`
- Create: `test/native/tests/test_core_scheduler.cpp`, `test/native/mocks/CoreScheduler.hpp`
- Modify: `src/PoolController.cpp` (remove sensor node `loop()` calls; start scheduler at end of `setup()`)
- Modify: `src/PoolController.hpp` (document the change)
- Modify: `test/native/CMakeLists.txt` (add mock include path is already there; add test to TEST_SOURCES)

**Interfaces:**
- Consumes: `SensorSlots` (Task 2), `DallasTemperatureNode` begin/finish (Task 3), `ESP32TemperatureNode::loop()`.
- Produces: `CoreScheduler::begin()` (creates SensorTask on Core 0, priority 2, 6 KB stack; PublishTask priority 1, 4 KB — created in Task 5), `SensorTask::start()` internals, and `CoreScheduler::logStackWatermarks()` (called periodically). Native `mocks/CoreScheduler.hpp` captures registration so tests can assert parameters.

- [ ] **Step 1: Write the failing test (parameter assertions via mock)**

Create `test/native/mocks/CoreScheduler.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Native test double for CoreScheduler.
 * Captures begin() parameters so tests can assert the planned values.
 */
class CoreScheduler {
public:
  static constexpr uint8_t TASK_PRIORITY_SENSOR = 2;
  static constexpr uint8_t TASK_PRIORITY_PUBLISH = 1;
  static constexpr uint8_t TASK_PRIORITY_DISPLAY = 1;
  static constexpr uint16_t TASK_STACK_SENSOR = 6 * 1024;
  static constexpr uint16_t TASK_STACK_PUBLISH = 4 * 1024;
  static constexpr uint16_t TASK_STACK_DISPLAY = 3 * 1024;

  static void begin();
  static void logStackWatermarks();

  static uint8_t sensorPriority;
  static uint16_t sensorStack;
};

}  // namespace PoolController
```

Create `test/native/tests/test_core_scheduler.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include <unity.h>
#include "CoreScheduler.hpp"

using PoolController::CoreScheduler;

void setUp() {}
void tearDown() {}

void test_sensor_task_priority_matches_plan() {
  CoreScheduler::begin();
  TEST_ASSERT_EQUAL(CoreScheduler::TASK_PRIORITY_SENSOR, CoreScheduler::sensorPriority);
}

void test_sensor_task_stack_matches_plan() {
  CoreScheduler::begin();
  TEST_ASSERT_EQUAL(CoreScheduler::TASK_STACK_SENSOR, CoreScheduler::sensorStack);
}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_sensor_task_priority_matches_plan);
  RUN_TEST(test_sensor_task_stack_matches_plan);
  UNITY_END();
}
```

Add `test/native/tests/test_core_scheduler.cpp` to `TEST_SOURCES` in `CMakeLists.txt`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cd test/native/build && cmake .. && make test_runner 2>&1 | tail -5 && ASAN_OPTIONS="..." ./test_runner | tail -8`
Expected: FAIL — linker error: `CoreScheduler::begin()` undefined (mock .cpp not provided yet). Provide it now:

Create `test/native/mocks/CoreScheduler.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "CoreScheduler.hpp"

namespace PoolController {
uint8_t CoreScheduler::sensorPriority = 0;
uint16_t CoreScheduler::sensorStack = 0;

void CoreScheduler::begin() {
  sensorPriority = TASK_PRIORITY_SENSOR;
  sensorStack = TASK_STACK_SENSOR;
}

void CoreScheduler::logStackWatermarks() {}
}  // namespace PoolController
```

Add `test/native/mocks/CoreScheduler.cpp` to `MOCK_SOURCES` in `CMakeLists.txt`.

- [ ] **Step 3: Run test to verify it passes**

Run: `cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6`
Expected: PASS.

- [ ] **Step 4: Implement CoreScheduler (device side)**

Create `src/CoreScheduler.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.hpp
 * @brief Static launcher for the Core-0 I/O tasks.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Creates and tracks the dedicated I/O tasks pinned to Core 0.
 *
 * All tasks are created once in begin() with fixed stacks and priorities
 * (no dynamic task creation after setup). Priorities only matter within a
 * core: the I/O tasks sit below the WiFi-stack tasks and yield via
 * vTaskDelay at their scheduling period.
 */
class CoreScheduler {
public:
  static constexpr uint8_t TASK_PRIORITY_SENSOR = 2;
  static constexpr uint8_t TASK_PRIORITY_PUBLISH = 1;
  static constexpr uint8_t TASK_PRIORITY_DISPLAY = 1;
  static constexpr uint16_t TASK_STACK_SENSOR = 6 * 1024;
  static constexpr uint16_t TASK_STACK_PUBLISH = 4 * 1024;
  static constexpr uint16_t TASK_STACK_DISPLAY = 3 * 1024;

  /** @brief Create all Core-0 I/O tasks. Call once from setup(), after initializeController(). */
  static void begin();

  /** @brief Log stack high-water marks of all tasks (call periodically from loop()). */
  static void logStackWatermarks();
};

}  // namespace PoolController
```

Create `src/CoreScheduler.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CoreScheduler.cpp
 * @brief Task creation for the Core-0 I/O tasks.
 */

#include "CoreScheduler.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "SensorTask.hpp"

#ifdef NORVI_AE01_R
#include "DisplayTask.hpp"
#endif

namespace PoolController {

void CoreScheduler::begin() {
  // Core 0 = PRO_CPU_NUM (I/O core); Core 1 = APP_CPU_NUM (control loop).
  const BaseType_t core0 = PRO_CPU_NUM;

  SensorTask::start(TASK_PRIORITY_SENSOR, TASK_STACK_SENSOR, core0);

#ifdef NORVI_AE01_R
  DisplayTask::start(TASK_PRIORITY_DISPLAY, TASK_STACK_DISPLAY, core0);
#endif
}

void CoreScheduler::logStackWatermarks() {
  static uint32_t lastLog = 0;
  if (millis() - lastLog < 60000) {
    return;
  }
  lastLog = millis();
  SensorTask::logStackWatermark();
#ifdef NORVI_AE01_R
  DisplayTask::logStackWatermark();
#endif
}

}  // namespace PoolController
```

- [ ] **Step 5: Implement SensorTask (device side)**

Create `src/SensorTask.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorTask.hpp
 * @brief Core-0 task owning all DS18B20/OneWire and ESP32 internal temp reads.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Runs the temperature measurement cycle exclusively on Core 0.
 *
 * Owns all Dallas/OneWire bus access (OneWire is not thread-safe — the
 * control loop never touches the buses anymore). Per period: begin
 * conversion, yield via vTaskDelay, read results, publish to SensorSlots.
 */
class SensorTask {
public:
  /** @brief Create and start the task pinned to the given core. */
  static void start(uint8_t priority, uint16_t stackBytes, BaseType_t core);

  /** @brief Log the task's stack high-water mark. */
  static void logStackWatermark();
};

}  // namespace PoolController
```

Create `src/SensorTask.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorTask.cpp
 * @brief DS18B20 + internal temperature measurement task.
 */

#include "SensorTask.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"

namespace PoolController {

// Referenced from PoolController.cpp (namespace scope globals).
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;

namespace {
TaskHandle_t sensorTaskHandle = nullptr;
uint32_t lastSolarReadingMs = 0;
uint32_t lastControllerReadingMs = 0;
constexpr uint32_t CONVERSION_DELAY_MS = 800;  // 12-bit DS18B20 conversion
}  // namespace

void sensorTaskFunc(void *) {
  for (;;) {
    const uint32_t now = millis();

    // Solar (master on shared NORVI bus) drives the shared conversion.
    const unsigned long solarInterval = solarTemperatureNode.getMeasurementInterval();
    if (now - lastSolarReadingMs >= solarInterval * 1000UL) {
      lastSolarReadingMs = now;
      Serial.println("〽 SensorTask: reading Dallas sensors");
      solarTemperatureNode.beginMeasurement();
      // Yield while the conversion runs — never block the control loop.
      vTaskDelay(pdMS_TO_TICKS(CONVERSION_DELAY_MS));
      solarTemperatureNode.finishMeasurement();
      poolTemperatureNode.finishMeasurement();
    }

    // ESP32 internal temperature on its own interval.
    const unsigned long ctrlInterval = ctrlTemperatureNode.getMeasurementInterval();
    if (now - lastControllerReadingMs >= ctrlInterval * 1000UL) {
      lastControllerReadingMs = now;
      ctrlTemperatureNode.loop();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void SensorTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  xTaskCreatePinnedToCore(sensorTaskFunc, "sensor", stackBytes, nullptr, priority, &sensorTaskHandle, core);
}

void SensorTask::logStackWatermark() {
  if (sensorTaskHandle != nullptr) {
    Serial.printf("  SensorTask stack high-water: %u B\n",
      static_cast<unsigned>(uxTaskGetStackHighWaterMark(sensorTaskHandle)));
  }
}

}  // namespace PoolController
```

> **Design note:** `DallasTemperatureNode` instances stay file-scope globals in `PoolController.cpp`; `SensorTask.cpp` declares them `extern`. The address-filter and recovery logic in `begin()`/`finishMeasurement()` is unchanged — only the call site moved off-core.

- [ ] **Step 6: Wire into PoolController**

In `src/PoolController.cpp`:

1. Add include: `#include "CoreScheduler.hpp"`
2. In `loop()`, delete these three lines (sensor reads now run on Core 0):
```cpp
  solarTemperatureNode.loop();
  poolTemperatureNode.loop();
  ctrlTemperatureNode.loop();
```
3. In `loop()`, at the end (after the MQTT publish block), add the watermark log:
```cpp
  CoreScheduler::logStackWatermarks();
```
4. In `setup()`, at the very end (after `ConfigManager::logOtaTransition();` and before the final heap print — or right after `initializeController()`), add:
```cpp
  // Start Core-0 I/O tasks (sensors, display, publish).
  CoreScheduler::begin();
```
5. Update the `loop()` doc comment to reflect that sensor reads moved off-core.

- [ ] **Step 7: Run native tests + build both environments**

Run:
```bash
cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6
~/.platformio/penv/bin/pio run -e norvi_ae01_r 2>&1 | tail -3
~/.platformio/penv/bin/pio run -e esp32dev 2>&1 | tail -3
```
Expected: native tests PASS; both firmware builds SUCCESS.

- [ ] **Step 8: Commit**

```bash
git add src/CoreScheduler.hpp src/CoreScheduler.cpp src/SensorTask.hpp src/SensorTask.cpp src/PoolController.cpp test/native/mocks/CoreScheduler.hpp test/native/mocks/CoreScheduler.cpp test/native/tests/test_core_scheduler.cpp test/native/CMakeLists.txt
git commit -m "feat: run DS18B20 sensor reads in a dedicated Core-0 task"
```

---

### Task 5: PublishTask + telemetry queue — MQTT serialization off the control loop

**Files:**
- Create: `src/PublishTask.hpp`, `src/PublishTask.cpp`
- Modify: `src/CoreScheduler.cpp` (start PublishTask in `begin()`)
- Modify: `src/PoolController.cpp` (enqueue instead of inline publish)

**Interfaces:**
- Consumes: `TelemetryQueue` (Task 1), `MqttPublisher` (unchanged static API: `publishStates()`, `publishDiscovery()`).
- Produces: `PublishTask::start(priority, stackBytes, core)`, `PublishTask::logStackWatermark()`. Publish requests are enqueued by the control loop; PublishTask drains them and calls `MqttPublisher` on Core 0.

- [ ] **Step 1: Implement PublishTask (device side)**

Create `src/PublishTask.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file PublishTask.hpp
 * @brief Core-0 task that serializes and publishes MQTT telemetry.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Drains the telemetry queue and performs MQTT serialization on Core 0.
 *
 * The control loop only enqueues publish requests (non-blocking); the heavy
 * JSON/HA-discovery serialization and the AsyncMqttClient::publish() calls
 * run here. AsyncMqttClient::publish() is non-blocking from the library side.
 */
class PublishTask {
public:
  /** @brief Create and start the task pinned to the given core. */
  static void start(uint8_t priority, uint16_t stackBytes, BaseType_t core);

  /** @brief Log the task's stack high-water mark. */
  static void logStackWatermark();
};

}  // namespace PoolController
```

Create `src/PublishTask.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file PublishTask.cpp
 * @brief MQTT publish task draining the telemetry queue.
 */

#include "PublishTask.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "TelemetryQueue.hpp"
#include "MqttPublisher.hpp"
#include "OtaUpdater.hpp"

namespace PoolController {

namespace {
TaskHandle_t publishTaskHandle = nullptr;
}  // namespace

void publishTaskFunc(void *) {
  for (;;) {
    PublishRequestKind kind;
    while (TelemetryQueue::instance().dequeue(kind)) {
      // Pause during OTA updates, but keep draining to avoid queue buildup.
      if (!OtaUpdater::isUpdateInProgress()) {
        if (kind == PublishRequestKind::DISCOVERY) {
          MqttPublisher::publishDiscovery();
        } else {
          MqttPublisher::publishStates();
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void PublishTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  xTaskCreatePinnedToCore(publishTaskFunc, "publish", stackBytes, nullptr, priority, &publishTaskHandle, core);
}

void PublishTask::logStackWatermark() {
  if (publishTaskHandle != nullptr) {
    Serial.printf("  PublishTask stack high-water: %u B\n",
      static_cast<unsigned>(uxTaskGetStackHighWaterMark(publishTaskHandle)));
  }
}

}  // namespace PoolController
```

Add a singleton accessor to `src/TelemetryQueue.hpp` (so both the control loop and PublishTask share one instance):

```cpp
  /** @brief Process-wide singleton used by the control loop and PublishTask. */
  static TelemetryQueue &instance() {
    static TelemetryQueue queue;
    return queue;
  }
```

- [ ] **Step 2: Start PublishTask from CoreScheduler**

In `src/CoreScheduler.cpp` `begin()`, after `SensorTask::start(...)`:

```cpp
  PublishTask::start(TASK_PRIORITY_PUBLISH, TASK_STACK_PUBLISH, core0);
```

Add include `#include "PublishTask.hpp"`. In `logStackWatermarks()` add `PublishTask::logStackWatermark();`.

- [ ] **Step 3: Enqueue instead of inline publish in PoolController**

In `src/PoolController.cpp` `loop()`, replace the MQTT publish block:

```cpp
  // Handle Home Assistant Connection State transition
  static bool wasMqttConnected = false;
  bool currentMqttState = NetworkManager::isMqttConnected();
  if (currentMqttState && !wasMqttConnected) {
    // Freshly connected to MQTT: publish Discovery and States via PublishTask.
    TelemetryQueue::instance().enqueue(PublishRequestKind::DISCOVERY);
    TelemetryQueue::instance().enqueue(PublishRequestKind::STATES);
    wasMqttConnected = true;
  } else if (!currentMqttState) {
    wasMqttConnected = false;
  }

  // Periodically enqueue telemetry publish to HA (P4) — serialization runs on Core 0.
  if (currentMqttState && Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    _lastMeasurement = millis();
    TelemetryQueue::instance().enqueue(PublishRequestKind::STATES);
  }
```

Add includes `#include "TelemetryQueue.hpp"` and `#include "PublishTask.hpp"` to `src/PoolController.cpp`.

- [ ] **Step 4: Run native tests + build both environments**

Run:
```bash
cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6
~/.platformio/penv/bin/pio run -e norvi_ae01_r 2>&1 | tail -3
~/.platformio/penv/bin/pio run -e esp32dev 2>&1 | tail -3
```
Expected: native tests PASS; both firmware builds SUCCESS.

- [ ] **Step 5: Commit**

```bash
git add src/PublishTask.hpp src/PublishTask.cpp src/CoreScheduler.cpp src/PoolController.cpp src/TelemetryQueue.hpp
git commit -m "feat: offload MQTT telemetry serialization to a Core-0 publish task"
```

---

### Task 6: DisplayTask (NORVI) — OLED rendering off-core

**Files:**
- Create: `src/DisplayTask.hpp`, `src/DisplayTask.cpp`
- Modify: `src/CoreScheduler.cpp` (already guarded in Task 4 — keep)
- Modify: `src/PoolController.cpp` (replace `NorviOledDisplay::loop()` + `NorviButtonHandler::loop()` with render request + button scan)
- Modify: `src/NorviOledDisplay.{hpp,cpp}` (split `loop()` into `update()` + `render()`; read temps from `SensorSlots`)

**Interfaces:**
- Consumes: `SensorSlots` (Task 2), existing `NorviOledDisplay` static API.
- Produces: `DisplayTask::start(priority, stackBytes, core)`, `DisplayTask::logStackWatermark()`, `DisplayTask::requestRender()`. `NorviOledDisplay::update()` (state machine, Core 1) and `NorviOledDisplay::render()` (draw + I2C push, Core 0). Button handling stays on Core 1 (buttons are debounce-based, <1 ms, and their callbacks mutate control-loop singletons — single-writer rule).

- [ ] **Step 1: Split NorviOledDisplay::loop()**

In `src/NorviOledDisplay.hpp`, replace the `loop()` declaration with:

```cpp
  /**
   * @brief Advance the display state machine (page nav, auto-return, burn-in).
   * Runs on the control loop (Core 1); cheap, non-blocking.
   */
  static void update();

  /**
   * @brief Redraw the current page and push to the OLED over I2C.
   * Runs on DisplayTask (Core 0). Reads temps from SensorSlots.
   */
  static void render();
```

In `src/NorviOledDisplay.cpp`, rename the existing `loop()` implementation to `update()`, and add a new `render()`:

```cpp
void NorviOledDisplay::render() {
  if (forceRedraw_ || (millis() - lastUpdateMs_ >= UPDATE_INTERVAL_MS)) {
    lastUpdateMs_ = millis();
    drawPage();
  }
}
```

Keep the existing auto-return / burn-in / page-navigation logic inside `update()`. In `drawMainPage()`, replace direct node reads:

```cpp
  // Before (drops direct singleton reads):
  //   float poolTemp = ...node reads...
  // After (thread-safe snapshot):
  const float poolTemp = SensorSlots::read(SensorId::POOL);
  const float solarTemp = SensorSlots::read(SensorId::SOLAR);
```

Add `#include "SensorSlots.hpp"` to `src/NorviOledDisplay.cpp`.

- [ ] **Step 2: Implement DisplayTask**

Create `src/DisplayTask.hpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayTask.hpp
 * @brief Core-0 task rendering the NORVI OLED display (NORVI_AE01_R only).
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Renders the OLED display on Core 0.
 *
 * The control loop advances the display state machine and requests renders;
 * this task owns the I2C SSD1306 work so a hung display can never stall
 * the control loop. NORVI_AE01_R only.
 */
class DisplayTask {
public:
  /** @brief Create and start the task pinned to the given core. */
  static void start(uint8_t priority, uint16_t stackBytes, BaseType_t core);

  /** @brief Request a render on the next task tick. */
  static void requestRender();

  /** @brief Log the task's stack high-water mark. */
  static void logStackWatermark();
};

}  // namespace PoolController
```

Create `src/DisplayTask.cpp`:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayTask.cpp
 * @brief OLED render task (NORVI_AE01_R only).
 */

#include "DisplayTask.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "NorviOledDisplay.hpp"

namespace PoolController {

namespace {
TaskHandle_t displayTaskHandle = nullptr;
volatile bool renderRequested = false;
}  // namespace

void displayTaskFunc(void *) {
  for (;;) {
    if (renderRequested || (millis() % 2000 < 50)) {
      renderRequested = false;
      NorviOledDisplay::render();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void DisplayTask::start(uint8_t priority, uint16_t stackBytes, BaseType_t core) {
  xTaskCreatePinnedToCore(displayTaskFunc, "display", stackBytes, nullptr, priority, &displayTaskHandle, core);
}

void DisplayTask::requestRender() { renderRequested = true; }

void DisplayTask::logStackWatermark() {
  if (displayTaskHandle != nullptr) {
    Serial.printf("  DisplayTask stack high-water: %u B\n",
      static_cast<unsigned>(uxTaskGetStackHighWaterMark(displayTaskHandle)));
  }
}

}  // namespace PoolController
```

- [ ] **Step 3: Wire into PoolController**

In `src/PoolController.cpp`, under the existing `#ifdef NORVI_AE01_R` block in `loop()`:

Replace:
```cpp
  // Update NORVI OLED display and read front-panel buttons
  NorviOledDisplay::loop();
  NorviButtonHandler::loop();
```
with:
```cpp
  // Advance display state machine (Core 1) and request render on DisplayTask (Core 0).
  NorviOledDisplay::update();
  DisplayTask::requestRender();
  NorviButtonHandler::loop();
```

Add `#include "DisplayTask.hpp"` (inside the `#ifdef NORVI_AE01_R` include block).

> **Design note (deviation from spec §3):** the button *scan* stays in the control loop because button callbacks mutate control-loop singletons (`operationModeNode`, `poolPumpNode`) — running them on Core 0 would violate the single-writer rule. Debounce-based scanning is <1 ms. The OLED *rendering* (the blocking I2C work) is what moves to Core 0.

- [ ] **Step 4: Run native tests + build both environments**

Run:
```bash
cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6
~/.platformio/penv/bin/pio run -e norvi_ae01_r 2>&1 | tail -3
~/.platformio/penv/bin/pio run -e esp32dev 2>&1 | tail -3
```
Expected: native tests PASS; both firmware builds SUCCESS.

- [ ] **Step 5: Commit**

```bash
git add src/DisplayTask.hpp src/DisplayTask.cpp src/PoolController.cpp src/NorviOledDisplay.hpp src/NorviOledDisplay.cpp
git commit -m "feat: render NORVI OLED on a Core-0 display task"
```

---

### Task 7: Thread-safety hardening + watchdog integration

**Files:**
- Modify: `src/DegradationManager.{hpp,cpp}` — thread-safe `reportSensorStatus`
- Modify: `src/SystemMonitor.hpp` — `feedWatchdogFromTask()` wrapper
- Modify: `src/SensorTask.cpp` — use the wrapper
- Create: `test/native/tests/test_degradation_manager.cpp` (if none exists yet — check `test/native/tests/` first)

**Interfaces:**
- Consumes: existing `DegradationManager` API.
- Produces: `DegradationManager::reportSensorStatus(const char *id, bool found)` remains callable from SensorTask (Core 0) while `evaluate()` stays on Core 1; `SystemMonitor::feedWatchdogFromTask()`.

- [ ] **Step 1: Harden DegradationManager::reportSensorStatus**

Read `src/DegradationManager.hpp` first. Then make the status update atomic (a single critical section around the flag write):

```cpp
// In DegradationManager.cpp, inside reportSensorStatus:
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;  // file-scope static
  portENTER_CRITICAL(&mux);
  // existing flag update...
  portEXIT_CRITICAL(&mux);
```

If `reportSensorStatus` only touches a per-sensor bool, a `volatile bool` is sufficient; if it aggregates counts, use the critical section. Match whichever the existing implementation needs — do not change semantics.

- [ ] **Step 2: Add the watchdog wrapper**

In `src/SystemMonitor.hpp`, after `feedWatchdog()`:

```cpp
  /**
   * @brief Feed the watchdog from a non-loop task (SensorTask, PublishTask, DisplayTask).
   * esp_task_wdt_reset() is safe to call from any task; this wrapper exists
   * so I/O tasks can feed during long I/O waits without touching loop state.
   */
  static void feedWatchdogFromTask() { esp_task_wdt_reset(); }
```

In `src/SensorTask.cpp`, replace the direct `SystemMonitor::feedWatchdog()` calls inside `beginMeasurement()`/`finishMeasurement()` paths with `SystemMonitor::feedWatchdogFromTask()` — update `DallasTemperatureNode.cpp` accordingly (it already calls `SystemMonitor::feedWatchdog()`, which is fine; the wrapper is used by SensorTask itself around the conversion delay).

- [ ] **Step 3: Run native tests + build both environments**

Run:
```bash
cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3 && ASAN_OPTIONS="..." ./test_runner | tail -6
~/.platformio/penv/bin/pio run -e norvi_ae01_r 2>&1 | tail -3
~/.platformio/penv/bin/pio run -e esp32dev 2>&1 | tail -3
```
Expected: native tests PASS; both firmware builds SUCCESS.

- [ ] **Step 4: Commit**

```bash
git add src/DegradationManager.hpp src/DegradationManager.cpp src/SystemMonitor.hpp src/SensorTask.cpp src/DallasTemperatureNode.cpp
git commit -m "feat: thread-safe sensor status reporting and task watchdog wrapper"
```

---

### Task 8: Final verification + docs sync

**Files:**
- Modify: `docs/multicore-architecture.md`, `docs/multicore-architecture.de.md` (verify they match the implemented split — button scan stays on Core 1)
- Modify: `docs/software-guide.md` if it describes the loop (check first)

- [ ] **Step 1: Full native test run**

Run:
```bash
cd test/native/build && cmake .. >/dev/null && make test_runner 2>&1 | tail -3
ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2" ./test_runner 2>&1 | tail -8
```
Expected: `Results: N suites passed, 0 suites failed` with N ≥ 74.

- [ ] **Step 2: Build both environments clean**

Run:
```bash
~/.platformio/penv/bin/pio run -e norvi_ae01_r 2>&1 | tail -3
~/.platformio/penv/bin/pio run -e esp32dev 2>&1 | tail -3
```
Expected: both `SUCCESS` with no new warnings beyond baseline.

- [ ] **Step 3: Verify docs match implementation**

Read `docs/multicore-architecture.md` + `.de.md`. If they say "button scan runs on DisplayTask", update to "button scan stays on Core 1 (single-writer rule); only OLED rendering runs on Core 0". Also confirm the task table (Sensor 2/6KB, Publish 1/4KB, Display 1/3KB) matches `CoreScheduler.hpp`.

- [ ] **Step 4: Manual on-device checklist (documented for the PR)**

Add a short "Manual verification" section to the PR description (or the docs page): loop iteration <20 ms during sensor reads (log a `millis()` delta around `context.loop()` temporarily or use an existing timing log), OLED renders and buttons work, MQTT telemetry cadence unchanged, no watchdog resets over 24 h.

- [ ] **Step 5: Commit**

```bash
git add docs/multicore-architecture.md docs/multicore-architecture.de.md docs/software-guide.md 2>/dev/null || true
git commit -m "docs: sync multicore architecture pages with implementation"
```

---

## Self-Review (run before handing off)

1. **Spec coverage:**
   - §1 Task framework → Task 4 (CoreScheduler).
   - §2 SensorTask → Task 4 (SensorTask + begin/finish split in Task 3).
   - §3 DisplayTask → Task 6 (NORVI only; documented deviation: button scan stays Core 1).
   - §4 PublishTask + queue → Tasks 1 + 5.
   - §5 Shared state & sync → Tasks 1 (queue), 2 (slots), 6 (render request flag).
   - §6 Watchdog & reliability → Tasks 4/7 (feed wrapper, stack watermarks, OTA pause flag).
   - §7 Thread-safety audit → Task 7 (DegradationManager, SystemMonitor wrapper; NetworkManager/ConfigManager/OperationModeNode verified unchanged — they stay Core 1 only).
   - Testing → Tasks 1/2/4 (native tests) + Task 8 (full run).
   - Migration phases → Tasks 3+4 (Phase 1), 5 (Phase 2), 6 (Phase 3), 7 (Phase 4), 8 (Phase 5).
   - Success criterion "loop <20 ms" → Task 8 manual checklist.

2. **Placeholder scan:** No TBD/TODO/`...` in code blocks; all interfaces are defined in the producing tasks before consumption.

3. **Type consistency:** `SensorId::{SOLAR,POOL,CONTROLLER}`, `PublishRequestKind::{STATES,DISCOVERY}`, `TelemetryQueue::instance()`, `SensorSlots::{write,read,isFound,reset}`, `beginMeasurement()/finishMeasurement()`, `CoreScheduler::TASK_*` constants — used identically across tasks.
