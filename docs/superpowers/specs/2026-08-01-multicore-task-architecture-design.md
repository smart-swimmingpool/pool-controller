# Design: Multicore Task Architecture for Pool Controller

**Date:** 2026-08-01
**Status:** Approved (brainstorming, Approach 1)
**Scope:** Restructure the pool controller firmware to use both ESP32 cores — dedicated
I/O tasks on Core 0 (sensors, display, telemetry publishing) and a deterministic control
loop on Core 1 (rules, relays, watchdog). Covers both build environments
(`norvi_ae01_r` and `esp32dev`).

## Motivation

The controller currently runs a single Arduino `loop()` on Core 1 while Core 0 only
serves the WiFi/BT stack. Everything — sensor reads, display updates, rules, network,
MQTT — is serialized into one loop iteration:

| Blocking point | Typical cost |
| -------------- | ------------ |
| DS18B20 `requestTemperatures()` (12-bit conversion, shared bus) | ~750 ms |
| OLED full-buffer push over I2C (`display.display()`) | ~10–40 ms |
| StatusLED, button debounce, MQTT publish serialization | <1 ms each |

A DS18B20 conversion therefore stalls the entire control loop — watchdog feeding,
rule evaluation, and relay actuation — for up to 750 ms per measurement cycle. This
couples I/O latency to control determinism and leaves half the chip idle.

## Requirements (from brainstorming)

1. **Loop latency:** Remove blocking sensor/display work from the control loop so
   loop iteration time stays in the low millisecond range.
2. **Isolation:** A hung or slow I/O subsystem (sensor bus, I2C display) must never
   block rule evaluation or relay actuation.
3. **Headroom:** Free capacity for future features (more sensors, web UI, logging).
4. **Scope:** Both build environments (`norvi_ae01_r` with OLED/buttons, `esp32dev`
   without). NORVI-specific tasks are guarded by `#ifdef NORVI_AE01_R`.
5. **No behavior change:** Existing features (rules, modes, MQTT/Homie, OTA, watchdog,
   degradation, safe mode) keep their semantics. Native tests stay green.

## Approach (chosen: A — task architecture with explicit core separation)

| Core | Role | Contents |
| ---- | ---- | -------- |
| **Core 0** (PRO_CPU) | I/O core | SensorTask (DS18B20 + internal temp), DisplayTask (OLED/buttons, NORVI only), PublishTask (MQTT telemetry serialization) |
| **Core 1** (APP_CPU) | Control core | Arduino `loop()`: watchdog, degradation, rules, relays, StatusLED, async network managers, OTA |

Rationale: Core 1 keeps the safety-critical, deterministic work. Core 0 absorbs the
blocking I/O. The WiFi/BT stack already lives on Core 0; its protocol tasks run at
higher priority than our I/O tasks, so network responsiveness is not degraded.

**Deliberately NOT moved:** the MQTT *connection* and web/OTA managers stay in the
control loop — `AsyncMqttClient` and the web server are already non-blocking. Only the
*telemetry serialization + publish* work (JSON build, HA Discovery payloads) is
offloaded via a queue to PublishTask.

## Architecture

### 1. Task framework (`src/CoreScheduler.{hpp,cpp}` — new)

Small, static, heap-friendly task launcher. No dynamic task creation after setup.

- `CoreScheduler::begin()` — called from `setup()` after `initializeController()`.
  Creates the I/O tasks pinned to Core 0 with explicit priorities and stack sizes.
- Task list (all Core 0, priority/stack tuned, no heap growth in steady state):

| Task | Priority | Stack | Runs | Notes |
| ---- | -------- | ----- | ---- | ----- |
| SensorTask | 2 | 6 KB | always | DS18B20 buses + ESP32 internal temp |
| PublishTask | 1 | 4 KB | always | drains telemetry queue → MQTT |
| DisplayTask | 1 | 3 KB | NORVI only | OLED render + button scan |

- Control loop keeps running on Core 1 with its existing implicit priority (Arduino
  `loopTask`, above tskIDLE). FreeRTOS priorities only matter within a core, so the
  I/O tasks never preempt the control loop directly; on Core 0 they yield via
  `vTaskDelay` at their scheduling period so they stay below the WiFi-stack tasks.
- All tasks feed the task watchdog when they take long paths (DS18B20 conversion
  wait, OTA pause).

### 2. SensorTask (`src/SensorTask.{hpp,cpp}` — new)

Owns the DS18B20 buses and the ESP32 internal temperature sensor.

- Runs the Dallas/OneWire access *exclusively* (OneWire is not thread-safe; the
  control loop must never touch the buses anymore).
- Per-period sequence: request conversion → yield (`vTaskDelay`) → read results →
  publish to consumers via thread-safe slots (see §4).
- Reuses the existing address-filter/rescan logic of `DallasTemperatureNode` /
  `ESP32TemperatureNode`, but calls their I/O methods from this task. The node
  `loop()` methods are replaced by `SensorTask`-driven measurement steps.
- Sensor/measurement intervals stay configurable as today.

### 3. DisplayTask (NORVI only, `src/NorviOledDisplay.*`)

Moves `NorviOledDisplay::loop()` and `NorviButtonHandler::loop()` to Core 0.

- Renders from a shared, mutex-protected snapshot of display state (temps, modes,
  network status) produced by the control loop — the display task never reads live
  singletons directly.
- Keeps existing burn-in shift logic and menu/setup flows; button input is written
  back to the control loop via a small input queue/flag so setup state machine
  semantics are unchanged.
- Guarded by `#ifdef NORVI_AE01_R`; `esp32dev` gets no DisplayTask.

### 4. PublishTask + telemetry queue

- A fixed-capacity queue (FreeRTOS `xQueueCreate`, e.g. 8 slots) carries publish
  requests from the control loop to PublishTask.
- PublishTask builds JSON/HA payloads and calls `AsyncMqttClient::publish()` — the
  call is non-blocking from the library side; heavy serialization no longer runs in
  the control loop.
- Discovery + state publishing on (re)connect keeps its current trigger points; they
  simply enqueue instead of serializing inline.

### 5. Shared state & synchronization

| Channel | Mechanism | Producer → Consumer |
| ------- | --------- | ------------------- |
| Sensor values | lock-free slots (atomics/`volatile` + seqlock where >1 word) | SensorTask → control loop, DisplayTask |
| Display state | mutex-protected snapshot struct | control loop → DisplayTask |
| Button input | small queue / atomic flags | DisplayTask → control loop |
| Telemetry | `xQueueCreate` publish queue | control loop → PublishTask |
| Log capture | existing `LogCapture` critical section | any task |

Rule: every cross-task data path is single-writer. No singleton is mutated from two
tasks without an explicit sync primitive (see §7 audit).

### 6. Watchdog & reliability integration

- Control loop keeps feeding the task watchdog as today; SensorTask/DisplayTask/
  PublishTask feed it during long I/O waits.
- `SystemMonitor::checkMemory()` still runs in the control loop; task stacks are
  sized and asserted (`uxTaskGetStackHighWaterMark`) at startup + periodic check so
  stack overflow risk is visible in logs/degradation.
- Safe mode / degradation semantics unchanged: `DegradationManager` stays a control
  loop concern; sensor faults are reported to it from SensorTask via a thread-safe
  status channel.
- OTA: during an update, PublishTask pauses publishing (flag) but keeps draining its
  queue to avoid buildup.

### 7. Thread-safety audit (existing singletons)

Required before merge — each singleton's access pattern is reviewed and, where a
method is called from a non-control task, hardened:

| Singleton | Cross-task access today | Action |
| --------- | ----------------------- | ------ |
| `DegradationManager` | from SensorTask (status) | add mutex/atomic status channel; keep `evaluate()` on Core 1 |
| `MqttPublisher` | from PublishTask (enqueue → serialize) | move serialization into PublishTask; control loop only enqueues |
| `NetworkManager` | control loop only | no change (verify) |
| `ConfigManager` | control loop only (setup) | no change (verify) |
| `SystemMonitor` | watchdog feed from I/O tasks | add thread-safe feed wrapper |
| `OperationModeNode` / rules | control loop only | no change (verify) |
| `LogCapture` | any task | already has critical section (verify coverage) |
| `NorviOledDisplay` / buttons | DisplayTask only | display state via snapshot; input via queue |

### 8. Error handling

- SensorTask on bus error: keeps retry/rescan logic, reports to `DegradationManager`,
  publishes NaN — identical semantics to today, just executed off-core.
- Task watchdog timeout → ESP32 resets via TWDT (unchanged global behavior).
- If a task crashes: core panic handler / TWDT behavior unchanged; `SystemMonitor`
  boot-loop detection still applies.

## Data flow (steady state)

```text
SensorTask (Core 0) ── lock-free slots ──▶ control loop (Core 1): rules/relays/watchdog
SensorTask ── status ────────────────────▶ DegradationManager (Core 1)
control loop ── snapshot ────────────────▶ DisplayTask (Core 0, NORVI)
DisplayTask ── button queue ─────────────▶ control loop
control loop ── telemetry queue ─────────▶ PublishTask (Core 0) ──▶ MQTT
control loop ── async network/OTA ──────── (unchanged, Core 1)
```

## Testing

- **Native unit tests (`test/native`)**: add tests for `CoreScheduler` (task
  creation/priority/stack assertions), telemetry queue, sensor slot
  synchronization, and the thread-safety wrappers. Mocks already cover
  DallasTemperature/SSD1306/AsyncMqttClient; extend mocks where the new
  cross-task API requires it.
- **Relay safety regression tests (`test/native/relay_safety`)**: must stay green —
  relay logic remains on Core 1 unchanged.
- **CI**: native tests + coverage unchanged; new files must stay under the same
  coverage expectations.
- **Manual on-device**: verify loop iteration time (log/debug counter) drops from
  ~750 ms to <20 ms during sensor reads; verify OLED still renders and buttons work;
  verify MQTT telemetry cadence unchanged.

## Migration plan

1. **Phase 1 — framework + SensorTask:** `CoreScheduler`, sensor slots, move DS18B20
   reads off-core. (Biggest latency win, foundation for everything else.)
2. **Phase 2 — PublishTask + queue:** telemetry serialization off the control loop.
3. **Phase 3 — DisplayTask (NORVI):** OLED/buttons off-core.
4. **Phase 4 — audit + hardening:** thread-safety pass on all singletons, watchdog
   feeds, stack high-water-mark logging, native tests for each new module.
5. **Phase 5 — docs:** `docs/multicore-architecture.md` + DE variant; update
   `software-guide` if it documents the loop.

Each phase keeps the build green and tests passing; phases land in the same PR branch
sequentially.

## Risks & mitigations

| Risk | Mitigation |
| ---- | ---------- |
| Thread-safety bugs in singletons | §7 audit + native tests; strict single-writer rule |
| OneWire bus contention | only SensorTask touches Dallas/OneWire |
| Heap cost of task stacks (~13 KB) | fixed static stacks, no dynamic growth; monitor via existing heap checks |
| Core 0 contention with WiFi stack | I/O tasks at low priority, `vTaskDelay`-based yields |
| OTA timing | PublishTask pause flag during update |
| I2C display hangs | DisplayTask on Core 0; control loop never waits on I2C |

## Success criteria

1. Loop iteration time during sensor reads: **<20 ms** (from ~750 ms).
2. All native tests green (ASan), relay safety regression green.
3. No heap growth in steady state (min free heap stable over 24 h).
4. No watchdog resets attributable to the new tasks.
5. OLED/buttons (NORVI) and MQTT telemetry behave as before the change.
