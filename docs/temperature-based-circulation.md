---
title: "Temperature-Based Circulation Time"
description: "Dynamic pump runtime extension based on water temperature"
weight: 50
---

# Temperature-Based Circulation Time

**Optimize pool circulation for better water quality with minimal power consumption.**

## 1. Motivation

The pool pump is the largest power consumer in a swimming pool. At the same time, it determines water quality. The required circulation volume strongly depends on water temperature:

| Water Temperature | Risk | Recommended Circulation |
|---|---|---|
| **> 26 °C** | High algae growth, turbidity | Maximum (8–12 h/day) |
| **20–26 °C** | Normal operation | Standard (4–8 h/day) |
| **< 20 °C** | Minimal algae growth | Minimal (1–4 h/day) |

A fixed timer is either **oversized** (wasting power in cold weather) or **undersized** (poor water quality during heat waves).

**Goal:** Automatically adjust daily pump runtime based on measured pool temperature.

## 2. Concept

The timer-configured runtime is dynamically extended when water temperature exceeds a threshold. The extension is proportional to the temperature difference.

### Formula

```
effectiveRuntime = baseRuntime + max(0, (poolTemp - tempCircThreshold) × tempCircFactor)
effectiveRuntime = min(effectiveRuntime, tempCircMaxRuntime)
```

### Parameters

| Parameter | Type | Default | Range | Description |
|---|---|---|---|---|
| `tempCircThreshold` | double | 24.0 °C | 0–40 °C | Temperature above which runtime is extended |
| `tempCircFactor` | uint16_t | 30 min/°C | 0–120 min/°C | Extra minutes **per °C** above threshold |
| `tempCircMaxRuntime` | uint16_t | 720 min | 60–1440 min | Absolute upper limit for total runtime |

### Computed Fields (read-only)

| Field | Description |
|---|---|
| `effectiveRuntime` | Calculated runtime in minutes (for status/logging) |
| `effectiveEndTime` | Resulting switch-off time |

### Behavior During Temperature Changes

The calculation runs **continuously** (every `loop()`), but the end time may **only move later**. Once the pump is running, the highest calculated end time is remembered.

```
Start (10:00, 24 °C):   end = 10:00 + 480 min = 18:00
14:00, 28 °C → +4×30:   newEnd = max(18:00, 20:00) = 20:00 ✓
16:00, 26 °C → +2×30:   newEnd = max(20:00, 19:00) = 20:00 ⛔ stays
```

**Rule:** The pump switches off **no earlier** than the original timer end time — even if it cools down later. This prevents surprise shutoffs and erratic behavior.

```cpp
// Core logic in RuleTimer/RuleAuto::loop():
uint16_t newEndMinutes = calculateEffectiveEndMinutes(
    baseStartMinutes, baseEndMinutes, poolTemp);

// Only extend, never shorten:
if (newEndMinutes > activeEndMinutes) {
    activeEndMinutes = newEndMinutes;
}
```

## 3. Examples

### Summer (Pool 30 °C / 86 °F)

```
Timer:         10:00 – 18:00  (base = 480 min)
Threshold:     24 °C
Difference:    6 °C
Extension:     6 × 30 = 180 min
Effective:     480 + 180 = 660 min
Switch-off:    10:00 + 660 min = 21:00
```

→ Pump runs 3 hours longer. More circulation during heat prevents algae.

### Mild (Pool 22 °C / 72 °F)

```
Timer:         10:00 – 18:00
Threshold:     24 °C
Difference:    22 < 24 → No extension
Effective:     = base = 480 min
Switch-off:    18:00
```

→ Normal runtime, no change.

### Hot (Pool 34 °C, max runtime reached)

```
Timer:         10:00 – 18:00
Difference:    10 °C
Extension:     10 × 30 = 300 min
Subtotal:      480 + 300 = 780 min
Max runtime:   720 min (12 h)
Effective:     min(780, 720) = 720 min
Switch-off:    10:00 + 720 min = 22:00
```

→ The upper limit prevents excessive pump wear.

## 4. System Integration

### Affected Components

```
ConfigManager (ControllerSettings)
  + tempCircThreshold: double
  + tempCircFactor: uint16_t
  + tempCircMaxRuntime: uint16_t

RuleTimer / RuleAuto
  + calculateEffectiveRuntime()  — shared helper
  + uses calculated runtime instead of fixed end time

MqttPublisher
  + 3 new Number entities via HA Discovery
  + Command handlers for /temp-circ-*/set

WebPortal (later)
  + Input fields in Config tab
```

### Core Logic

```cpp
uint16_t calculateEffectiveRuntime(uint16_t baseMinutes, float poolTemp) {
    auto &s = ConfigManager::getSettings();

    // NaN or below threshold: use base runtime
    if (poolTemp != poolTemp || poolTemp <= s.tempCircThreshold) {
        return baseMinutes;
    }

    float diff = poolTemp - s.tempCircThreshold;
    float extraMinutes = diff * s.tempCircFactor;

    // Round to whole minutes
    uint16_t extra = static_cast<uint16_t>(extraMinutes + 0.5f);
    uint16_t total = baseMinutes + extra;

    // Apply upper limit
    return (total > s.tempCircMaxRuntime) ? s.tempCircMaxRuntime : total;
}
```

### Midnight Crossing

The timer logic in `checkPoolPumpTimer()` already supports end times that cross midnight (e.g. start 22:00, end 02:00). The temperature-based extension can use the same mechanism — the effective end time is not capped at 24:00.

### NVS Persistence

The 3 parameters are stored in the `ControllerSettings` struct via NVS. They are configuration values (rarely changed), so wear is not a concern.

## 5. Home Assistant Integration

Each parameter is published as a Number entity via HA Discovery:

| Entity | Topic Suffix | Min | Max | Step |
|---|---|---|---|---|
| `temp-circ-threshold` | `/number/pool-controller/temp-circ-threshold/…` | 0 | 40 | 0.5 |
| `temp-circ-factor` | `/number/pool-controller/temp-circ-factor/…` | 0 | 120 | 5 |
| `temp-circ-max-runtime` | `/number/pool-controller/temp-circ-max-runtime/…` | 60 | 1440 | 15 |

Plus a sensor for computed effective runtime:

| Entity | Topic Suffix | Unit |
|---|---|---|
| `effective-runtime` | `/sensor/pool-controller/effective-runtime/…` | min |

## 6. Implementation Plan

| Step | File(s) | Effort |
|---|---|---|
| 1. Extend Config struct | `ConfigManager.hpp` | ~10 min |
| 2. Helper function + defaults | `Timer.hpp`, `Timer.cpp` | ~15 min |
| 3. Modify RuleTimer::loop() | `RuleTimer.cpp` | ~20 min |
| 4. Modify RuleAuto::loop() | `RuleAuto.cpp` | ~10 min |
| 5. MQTT Discovery + handlers | `MqttPublisher.cpp` | ~30 min |
| 6. Build + Test | — | ~10 min |
| **Total** | | **~1.5 h** |

## 7. Future Extensions (v2)

- **Cold reduction**: Optional `tempCircMinRuntime` below a lower threshold
- **Trend-based**: Factor in whether temperature is rising or falling
- **Seasonal calendar**: Automatically adjust base parameters for summer/winter
