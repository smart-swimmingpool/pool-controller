---
title: "Auto Mode Immediate Pump Check on Mode Switch"
description: "When switching to auto mode, immediately evaluate the timer window and temperature-based runtime extension instead of waiting for the next measurement interval"
date: 2026-09-20
---

# Auto Mode Immediate Pump Check on Mode Switch

## Problem

When the operation mode is switched to `auto` (via Web UI, Home Assistant/MQTT, local buttons, or at boot), the pool pump is not immediately evaluated against the current timer window and temperature-based runtime extension. The system waits until the next measurement interval (default 300 seconds / 5 minutes) before `RuleAuto::loop()` runs and checks `checkPoolPumpTimer()`.

This causes a noticeable delay where the pump stays off even though the current time is within the configured timer window (or its temperature-extended window).

## Solution

Add an immediate rule evaluation in `OperationModeNode::setMode()` after a successful switch to `auto` mode.

### Changes

1. **`OperationModeNode::setMode()`** (src/OperationModeNode.cpp)
   - After `_mode = mode` and `saveState()`: if the new mode is `auto`, call a new private method `evaluateActiveRuleImmediately()`.

2. **New private method `evaluateActiveRuleImmediately()`** (src/OperationModeNode.cpp/.hpp)
   - Retrieves the active rule via `getRule()` (which already updates rule properties with current temperatures and timer settings).
   - Calls `rule->loop()` once immediately.
   - This reuses the existing `RuleAuto::loop()` logic which:
     - Checks the current timer window via `checkPoolPumpTimer(poolTemp)`
     - Calculates and applies temperature-based runtime extension
     - Turns on the pool pump if within window/extension
     - Applies solar logic only when pool pump is running

3. **Boot behavior** (no code change needed)
   - In `PoolController::initializeController()`, sensor node references are set on `operationModeNode` **before** `setMode(ConfigManager::getSettings().opMode.c_str(), "boot:config")` is called (lines 250-251 before line 243).
   - Therefore the immediate evaluation at boot will have valid temperature readings.

### Affected Files

- `src/OperationModeNode.hpp` — add private method declaration
- `src/OperationModeNode.cpp` — implement immediate evaluation in `setMode()` and new method

### Not Affected

- `RuleAuto`, `RuleTimer`, `RuleBoost`, `RuleManu` — no changes
- `Timer`, `TimerSetting` — no changes
- `ConfigManager`, NVS persistence — no changes
- `MqttPublisher`, HA Discovery — no changes
- `WebPortal` — no changes

## Behavior

| Scenario | Before | After |
|----------|--------|-------|
| Web UI switches to auto | Pump checked at next interval (≤5 min) | Pump checked immediately |
| HA/MQTT command to auto | Pump checked at next interval | Pump checked immediately |
| Local button cycles to auto | Pump checked at next interval | Pump checked immediately |
| Boot with saved mode = auto | Pump checked at first interval | Pump checked immediately during init |

## Edge Cases

- **Invalid temperature readings (NaN)**: `RuleAuto::loop()` already handles this — turns off solar pump, keeps pool pump decision based on timer only (via `checkPoolPumpTimer()` without temperature argument).
- **Time not synced**: `checkPoolPumpTimer()` returns `true` (pump ON for safety) when `time.tm_year == -1`.
- **Mode already auto**: `setMode()` detects no change (`!_mode.equals(mode)`) and skips the immediate evaluation — correct behavior.
- **Switching from auto to another mode**: No immediate evaluation needed; the new mode's rule will run at its next interval.

## Testing

- Unit test: `OperationModeNode::setMode("auto")` triggers immediate `rule->loop()` call.
- Integration test: Switch to auto via Web UI / HA / local button → verify pump state changes immediately if in timer window.
- Boot test: Device boots with saved mode = auto → pump runs immediately if in timer window.

## Risks

- **Low**: Reuses existing, well-tested `RuleAuto::loop()` logic. No new pump control logic introduced.
- **Concurrency**: `setMode()` can be called from multiple sources (Web, MQTT, buttons). The immediate evaluation runs synchronously in the caller's context — same as the existing `saveState()` and log calls. No new threading concerns.

## Acceptance Criteria

1. Switching to `auto` via any source (Web, HA/MQTT, local button, boot) causes immediate pump evaluation.
2. Pump turns ON immediately if current time is within timer window or temperature-extended window.
3. Temperature-based runtime extension is calculated and applied on the immediate check.
4. No duplicate pump control logic — `RuleAuto::loop()` remains the single source of truth.
5. Existing behavior for other modes (`manu`, `boost`, `timer`) unchanged.