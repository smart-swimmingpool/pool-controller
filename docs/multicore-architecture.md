---
title: Multicore Architecture
summary: How the firmware isolates blocking sensor I/O on ESP32 Core 0 while keeping mutable controller state serialized on Core 1
date: "2026-08-01"
lastmod: "2026-10-04"
draft: false
toc: true
type: docs
featured: false
tags: ["docs", "controller", "architecture", "multicore", "tasks"]
menu:
  docs:
    parent: Pool Controller
    name: Multicore Architecture
    weight: 33
---

## Overview

The firmware uses an explicit ownership model instead of moving every I/O operation to the second core.
The safety-critical controller model remains single-writer on the Arduino loop task, while the blocking
DS18B20 conversion is isolated on Core 0.

| Core | Role | Contents |
| ---- | ---- | -------- |
| **Core 0** (PRO_CPU) | Sensor I/O | `SensorTask`: DS18B20 conversion/readout and internal ESP32 temperature |
| **Core 1** (APP_CPU) | Control and stateful I/O | Arduino `loop()`: watchdog, degradation, rules, relays, StatusLED, network/OTA, MQTT serialization, front-panel UI and OLED rendering |

## Why this boundary

A 12-bit DS18B20 conversion can take roughly 750 ms. Running it in the Arduino loop stalls rule
evaluation, watchdog handling and relay decisions. `SensorTask` removes that blocking conversion from
the control loop.

MQTT serialization and OLED rendering are intentionally kept on Core 1. Both read mutable controller
state such as operation mode, rules, relay state and configuration. Running those readers concurrently
with the control loop would create cross-core data races around objects such as Arduino `String` and
rule state. Keeping them on the owning task is simpler and safer than protecting the complete object
graph with locks.

## Task model

`CoreScheduler` creates one required worker task:

| Task | Core | Priority | Stack | Purpose |
| ---- | ---- | -------- | ----- | ------- |
| `SensorTask` | 0 | 2 | 6 KB | DS18B20 and internal temperature acquisition |

If `SensorTask` cannot be created, the controller restarts. A persistent failure is handled by the
existing boot-loop detection and safe-mode path.

MQTT requests still use `TelemetryQueue`. `PoolController::loop()` drains that queue directly on Core 1
and invokes `MqttPublisher` there. `CoreScheduler` is responsible only for `SensorTask` lifecycle and
stack high-water logging.

The NORVI display follows the same rule: button handling, UI state transitions and OLED rendering are
serialized on Core 1 by `DisplayCoordinator`. The obsolete `PublishTask` and `DisplayTask` worker
implementations were removed after the concurrency review narrowed the ownership boundary.

## Sensor ownership

After startup, `SensorTask` is the exclusive owner of `OneWire` and `DallasTemperature` operations.
Core-1 code never scans or reads the bus directly.

`DallasTemperatureNode` therefore exposes two cross-core channels:

- Measurements are published through `SensorSlots`.
- Sensor discovery metadata and selected ROM addresses are copied into a small cached snapshot guarded
  by a short atomic lock. Web and MQTT code read only this cache.

Runtime sensor-mapping changes are stored as pending configuration. `SensorTask` applies them at the
start of a later measurement cycle, so calls from WebPortal or MQTT never execute OneWire transactions
on Core 1.

## DS18B20 measurement cycle

`DallasTemperature` is configured with `setWaitForConversion(false)`. A measurement cycle is:

1. Start conversions on the required bus or buses.
2. Yield `SensorTask` for 800 ms.
3. Read the completed values.
4. Publish the result to `SensorSlots` and refresh the discovery cache.

This avoids the former double wait where `requestTemperatures()` blocked internally and the worker then
waited another 800 ms.

If either Dallas sensor is missing or invalid, the common cycle uses the shorter 5-second recovery
interval. Once both sensors are valid, the configured `loopInterval` applies again.

## Data flow

```text
SensorTask (Core 0) ── SensorSlots ──────▶ control loop (Core 1): rules / relays
SensorTask (Core 0) ── discovery cache ──▶ WebPortal / MQTT / UI (Core 1)
control loop (Core 1) ── TelemetryQueue ─▶ MQTT serialization (Core 1)
DisplayCoordinator: input + state + render ───────────────────▶ Core 1
```

The important rule is ownership: hardware bus state belongs to `SensorTask`; mutable controller state
belongs to the control loop. Cross-core communication uses bounded snapshots instead of shared mutable
objects.

## Reliability

- Sensor values and found-state are transferred consistently through `SensorSlots`.
- `DegradationManager` uses atomic cross-core status flags.
- `SensorTask` registers with and feeds the task watchdog.
- Stack high-water logging remains available for the worker task.
- OTA handling and MQTT state mutation stay serialized on Core 1.

## Historical design documents

The original design and migration plan remain available under
[`docs/superpowers/specs/2026-08-01-multicore-task-architecture-design.md`](../superpowers/specs/2026-08-01-multicore-task-architecture-design.md)
and `docs/superpowers/plans/2026-08-01-multicore-task-architecture.md`. They document the earlier
three-worker proposal and are retained as implementation history. This page describes the authoritative
runtime architecture after the concurrency review.
