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
| `effective-runtime` | `/sensor/pool-controller/effective-runtime/…` | s |

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

## 7. User Guide

### 7.1 Quick Start

1. **Set a timer** — temperature-based circulation only works in **Timer** or **Auto** mode with a configured time window. Set your desired base runtime first (e.g. 10:00–18:00).
2. **Set a threshold** — start with `24 °C`. Below this temperature, the pump runs the normal timer schedule.
3. **Set a factor** — start with `30 min/°C`. Every degree above threshold adds half an hour.
4. **Set a max runtime** — start with `720 min (12 h)`. Prevents excessive runtime on very hot days.

That's it — the pump will now run longer when the water is warm.

> **Rule of thumb:** Once configured, the feature works automatically. No manual intervention needed. You can watch the **Circ. Runtime** value on the dashboard to see the calculated extension at a glance.

### 7.2 Recommended Settings by Pool Type

Find your setup and use these as a starting point:

| Pool Type | Threshold | Factor | Max Runtime | Why |
|---|---|---|---|---|
| ☀️ **Small above-ground** (≤15 m³) | 22 °C | 20 min/°C | 480 min (8 h) | Smaller volume needs less margin |
| 🏊 **Medium family pool** (25–50 m³) | 24 °C | 30 min/°C | 720 min (12 h) | Good balance of water quality and cost |
| 🌴 **Large pool / high bather load** (50+ m³) | 22 °C | 40 min/°C | 960 min (16 h) | More circulation needed for hygiene |
| 🌡️ **Heated pool** | 26 °C | 15 min/°C | 600 min (10 h) | Already filtered more via heating pump |

If you have solar heating, keep the threshold at or above your solar minimum temperature to avoid overlap.

### 7.3 Configuration via Web UI

#### Pool Tab

Open the settings menu (gear icon) → **Pool** → **🌡️ Temperature-Based Circulation** section.

| Field | Description |
|---|---|
| **Circ. Temp Threshold** | Pool temperature must exceed this value for extension to apply. |
| **Circ. Temp Factor** | How many extra minutes per °C above threshold. Higher = more aggressive extension. |
| **Circ. Max Runtime** | The absolute maximum pump runtime per day, including any extension. |

Click **Save Pool Settings** at the bottom of the tab.

#### Dashboard Telemetry

On the main dashboard, the bottom strip shows **Circ. Runtime** — the currently calculated effective runtime in minutes. This number updates live as the pool temperature changes and the `only-extend` rule tracks the highest computed value.

### 7.4 Configuration via Home Assistant

Once connected via MQTT, three Number entities appear in Home Assistant:

| Entity | Purpose |
|---|---|
| `number.pool_controller_temp_circ_threshold` | Set the temperature threshold |
| `number.pool_controller_temp_circ_factor` | Adjust the extension factor |
| `number.pool_controller_temp_circ_max_runtime` | Set the upper runtime limit |
| `sensor.pool_controller_effective_runtime` | Read-only — shows computed runtime |

You can change values from the Home Assistant UI, automations, or dashboards. Changes take effect immediately — no reboot required.

### 7.5 Example: Automating with Home Assistant

**Scenario:** Adjust factor based on weather forecast.

```yaml
automation:
  - alias: "Aggressive circulation on hot days"
    trigger:
      platform: numeric_state
      entity_id: weather.home
      attribute: temperature
      above: 32
    action:
      service: number.set_value
      target:
        entity_id: number.pool_controller_temp_circ_factor
      data:
        value: 45
```

**Scenario:** Notify when circulation ran near max.

```yaml
automation:
  - alias: "High circulation alert"
    trigger:
      platform: numeric_state
      entity_id: sensor.pool_controller_effective_runtime
      above: 700
    action:
      service: notify.mobile_app_phone
      data:
        message: "Pool circulation running near max ({{ states('sensor.pool_controller_effective_runtime') }} min)"
```

### 7.6 FAQ / Troubleshooting

> **Q: The pump runs much longer than expected — why?**

Check your pool temperature. If it's above the threshold and the factor is high, long runtimes are normal. Look at the **Circ. Runtime** value to see the calculated extension. Lower the **factor** or raise the **threshold** if needed.

> **Q: The pump stopped earlier than I expected — why?**

The extension only applies while the pump is running. If the pump was already on, the end time may have been extended earlier but then cooled down — the `only-extend` rule preserves the highest end time. Check that:
1. The pool temperature is actually above the threshold
2. The timer window is configured (temperature extension doesn't activate the pump by itself)

> **Q: Can the pump start earlier than the timer start?**

No. The feature only **extends** the end time. The start time remains exactly as configured in the timer. (See `only-extend` rule in §2.)

> **Q: Does the max runtime reset each day?**

Yes. At the start of each timer cycle, the effective end time resets to the base timer end. The extension is recalculated fresh.

> **Q: I changed values in the Web UI but nothing happened — why?**

Check the **Mode** on the dashboard. Temperature-based circulation works in **Timer** and **Auto** modes. In **Manual** or **Boost** mode, the feature is inactive.

> **Q: Will this wear out my pump faster?**

The `tempCircMaxRuntime` parameter is specifically designed to prevent excessive runtime. Set it to a value your pump can handle — 12 hours/day is safe for most pool pumps. Consult your pump's duty cycle specification.

## 8. Future Extensions (v2)

- **Cold reduction**: Optional `tempCircMinRuntime` below a lower threshold
- **Trend-based**: Factor in whether temperature is rising or falling
- **Seasonal calendar**: Automatically adjust base parameters for summer/winter
