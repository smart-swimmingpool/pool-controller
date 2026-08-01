---
title: Multicore Architecture
summary: How the firmware uses both ESP32 cores — dedicated I/O tasks (sensors, display, MQTT telemetry) on Core 0 and a deterministic control loop on Core 1
date: "2026-08-01"
lastmod: "2026-08-01"
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

The ESP32 has two Xtensa LX6 cores, but a single-loop Arduino sketch only uses one:
the WiFi/BT stack runs on Core 0 and the Arduino `loop()` on Core 1. Everything
else — sensor reads, display updates, rules, network, MQTT — runs serially inside
`loop()`.

The firmware restructures this into a **task architecture with explicit core
separation**:

| Core | Role | Contents |
| ---- | ---- | -------- |
| **Core 0** (PRO_CPU) | I/O core | SensorTask (DS18B20 + internal temp), DisplayTask (OLED/buttons, NORVI only), PublishTask (MQTT telemetry serialization) |
| **Core 1** (APP_CPU) | Control core | Arduino `loop()`: watchdog, degradation, rules, relays, StatusLED, async network managers, OTA |

## Why

Blocking work used to stall the entire control loop. The most expensive operation is
the DS18B20 temperature conversion (`requestTemperatures()`), which blocks for about
**750 ms** at 12-bit resolution. During that time the watchdog feeding, rule
evaluation, and relay actuation all wait.

Moving that I/O to dedicated tasks on Core 0 gives three benefits:

1. **Low loop latency** — the control loop stays in the low millisecond range.
2. **Isolation** — a hung sensor bus or I2C display can no longer block the
   safety-critical control logic on Core 1.
3. **Headroom** — capacity for future features (more sensors, web UI, logging).

## Task model

All I/O tasks are created in `setup()` by the `CoreScheduler` and stay static (no
dynamic task creation at runtime, no heap growth).

| Task | Core | Priority | Stack | Runs on |
| ---- | ---- | -------- | ----- | ------- |
| SensorTask | 0 | 2 | 6 KB | all builds |
| PublishTask | 0 | 1 | 4 KB | all builds |
| DisplayTask | 0 | 1 | 3 KB | NORVI only (`#ifdef NORVI_AE01_R`) |

FreeRTOS priorities only matter within a core: the I/O tasks yield via
`vTaskDelay` and stay below the WiFi-stack tasks on Core 0, so they never preempt
the control loop on Core 1.

## Data flow

```text
SensorTask (Core 0) ── lock-free slots ──▶ control loop (Core 1): rules/relays/watchdog
SensorTask ── status ────────────────────▶ DegradationManager (Core 1)
control loop ── snapshot ────────────────▶ DisplayTask (Core 0, NORVI)
DisplayTask ── button queue ─────────────▶ control loop
control loop ── telemetry queue ─────────▶ PublishTask (Core 0) ──▶ MQTT
control loop ── async network/OTA ──────── (unchanged, Core 1)
```

Every cross-task data path is **single-writer**:

- Sensor values: lock-free slots (atomic/single-word) — SensorTask writes, control
  loop reads.
- Display state: mutex-protected snapshot — control loop writes, DisplayTask reads.
- Button input: small queue / atomic flags — DisplayTask writes, control loop reads.
- Telemetry: fixed-capacity FreeRTOS queue — control loop enqueues, PublishTask
  serializes and publishes.

The MQTT *connection* and the web/OTA managers stay on the control loop — they are
already non-blocking (`AsyncMqttClient`, async web server). Only the telemetry
serialization (JSON build, HA Discovery payloads) is offloaded to PublishTask.

## Reliability

- The control loop keeps feeding the task watchdog; I/O tasks feed it during long
  waits (DS18B20 conversion, OTA pause).
- `SystemMonitor` reports task stack high-water marks so stack sizing is visible in
  logs and degradation.
- Safe mode and degradation semantics are unchanged — sensor faults are reported to
  `DegradationManager` over a thread-safe status channel.
- During OTA, PublishTask pauses publishing but keeps draining its queue.

## Design document

The full design, including the thread-safety audit of the existing singletons,
migration phases, risks, and success criteria, lives in
[`docs/superpowers/specs/2026-08-01-multicore-task-architecture-design.md`](../superpowers/specs/2026-08-01-multicore-task-architecture-design.md).
