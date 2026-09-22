# Auto Mode Immediate Pump Check Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** When switching to `auto` mode (via Web UI, Home Assistant/MQTT, local buttons, or at boot), immediately evaluate the timer window and temperature-based runtime extension instead of waiting for the next measurement interval.

**Architecture:** Add immediate rule evaluation in `OperationModeNode::setMode()` after successful switch to `auto`. Reuse existing `RuleAuto::loop()` logic which already handles timer window check, temperature extension calculation, and pump control. No new pump control logic introduced.

**Tech Stack:** ESP32 Arduino framework, C++17, PlatformIO, existing codebase patterns.

## Global Constraints

- Follow existing code style (clang-format, EditorConfig)
- No heap allocations in hot paths — use stack/pre-allocated objects
- No `String` in loops — use `const char*` or `StringView`
- All new code must compile with `-Wall -Wextra -Wpedantic`
- Preserve existing NVS persistence behavior
- No changes to `RuleAuto`, `RuleTimer`, `RuleBoost`, `RuleManu`, `Timer`, `ConfigManager`, `MqttPublisher`, `WebPortal`

---

### Task 1: Add private method declaration to OperationModeNode.hpp

**Files:**
- Modify: `src/OperationModeNode.hpp` (add private method declaration after line 87)

**Interfaces:**
- Produces: `void evaluateActiveRuleImmediately()` — private method called from `setMode()`

- [ ] **Step 1: Add method declaration**

```cpp
// In OperationModeNode.hpp, private section after line 87 (after saveState())
void evaluateActiveRuleImmediately();
```

- [ ] **Step 2: Verify compiles**

Run: `pio check` (or build)
Expected: Compiles without errors

- [ ] **Step 3: Commit**

```bash
git add src/OperationModeNode.hpp
git commit -m "feat: add evaluateActiveRuleImmediately declaration"
```

---

### Task 2: Implement evaluateActiveRuleImmediately() in OperationModeNode.cpp

**Files:**
- Modify: `src/OperationModeNode.cpp` (add implementation after `saveState()` method, before `loadState()`)

**Interfaces:**
- Consumes: `getRule()` — returns active `Rule*` with updated properties (temperatures, timer settings)
- Produces: Immediate call to `rule->loop()` which evaluates timer window and controls pump

- [ ] **Step 1: Write implementation**

```cpp
// In OperationModeNode.cpp, after saveState() method (around line 319)
void OperationModeNode::evaluateActiveRuleImmediately() {
  Rule *rule = getRule();
  if (rule != nullptr) {
    rule->loop();
  } else {
    LOG_ERROR("  ✖ No active rule for immediate evaluation (mode=%s)\n", _mode.c_str());
  }
}
```

- [ ] **Step 2: Verify compiles**

Run: `pio check`
Expected: Compiles without errors

- [ ] **Step 3: Commit**

```bash
git add src/OperationModeNode.cpp
git commit -m "feat: implement evaluateActiveRuleImmediately"
```

---

### Task 3: Call evaluateActiveRuleImmediately() in setMode() when switching to auto

**Files:**
- Modify: `src/OperationModeNode.cpp` (in `setMode()` method, after `_mode = mode` and `saveState()`)

**Interfaces:**
- Consumes: `evaluateActiveRuleImmediately()` from Task 2
- Triggered when: `mode.equals(STATUS_AUTO)` AND `!_mode.equals(mode)` (actual mode change)

- [ ] **Step 1: Modify setMode()**

```cpp
// In OperationModeNode::setMode(), after line 131 (_mode = mode) and line 134 (saveState())
// Add after the if (!_suppressPersist) saveState(); block:

if (mode.equals(STATUS_AUTO)) {
  evaluateActiveRuleImmediately();
}
```

Full context around lines 124-138:
```cpp
if (!_mode.equals(mode)) {
  // Reset temperature-based runtime extension on mode change
  for (auto &rule : _ruleVec) {
    rule->resetTemperatureExtension();
  }
  PoolController::LogCapture::logEvent("MODE_CHANGED", "Mode changed %s -> %s (source=%s, persist=%s)", _mode.c_str(),
    mode.c_str(), source, _suppressPersist ? "no" : "yes");
  _mode = mode;
  LOG_DEBUG("set mode: %s (source=%s)\n", _mode.c_str(), source);
  if (!_suppressPersist)
    saveState();

  // NEW: Immediate evaluation when switching to auto
  if (mode.equals(STATUS_AUTO)) {
    evaluateActiveRuleImmediately();
  }
} else {
  LOG_INFO("Mode set requested: %s -> %s (source=%s, changed=no, persist=no)\n", _mode.c_str(), mode.c_str(), source);
}
```

- [ ] **Step 2: Verify compiles**

Run: `pio check`
Expected: Compiles without errors

- [ ] **Step 3: Commit**

```bash
git add src/OperationModeNode.cpp
git commit -m "feat: call immediate rule evaluation on switch to auto mode"
```

---

### Task 4: Build and run native tests

**Files:**
- Test: `test/native/` (existing test suite)

**Interfaces:**
- Verifies: No regressions in existing mode switching, rule evaluation, timer logic

- [ ] **Step 1: Run native tests**

Run: `pio test -e native`
Expected: All tests pass

- [ ] **Step 2: If any test fails, investigate and fix**

- [ ] **Step 3: Commit any test fixes**

```bash
git add -A
git commit -m "fix: adjust tests for immediate auto evaluation"
```

---

### Task 5: Build firmware for target hardware

**Files:**
- Build: All firmware targets

**Interfaces:**
- Verifies: Firmware compiles for all supported boards (ESP32 variants)

- [ ] **Step 1: Build for default environment**

Run: `pio run`
Expected: Build succeeds

- [ ] **Step 2: Build for all environments**

Run: `pio run -e norvi_ae01_r` (and other environments if defined)
Expected: All builds succeed

- [ ] **Step 3: Commit if any build fixes needed**

```bash
git add -A
git commit -m "fix: build adjustments for immediate auto evaluation"
```

---

### Task 6: Manual verification (integration test)

**Files:**
- Hardware: ESP32 device with pool controller firmware

**Interfaces:**
- Verifies: Immediate pump activation when switching to auto within timer window

- [ ] **Step 1: Flash firmware to device**

Run: `pio run -t upload` (or OTA)

- [ ] **Step 2: Test Web UI switch to auto**
  - Set timer window to include current time
  - Switch mode to `auto` via Web UI
  - Verify pool pump turns ON immediately (not after 5 min)

- [ ] **Step 3: Test HA/MQTT command to auto**
  - Send `auto` command via MQTT to `pool-controller/operation-mode/set`
  - Verify immediate pump response

- [ ] **Step 4: Test local button cycle to auto**
  - Press mode button to cycle to `auto`
  - Verify immediate pump response

- [ ] **Step 5: Test boot with saved mode = auto**
  - Save mode = `auto` in NVS
  - Reboot device
  - Verify pump runs immediately if in timer window

- [ ] **Step 6: Verify temperature extension applied**
  - Set pool temp > threshold (e.g., 28°C with 24°C threshold)
  - Switch to auto
  - Verify pump stays ON past base timer end time (extension active)

- [ ] **Step 7: Verify no regression for other modes**
  - Switch to `manu`, `boost`, `timer` — verify normal behavior

---

## Execution Handoff

**Plan complete and saved to `docs/superpowers/plans/2026-09-20-auto-mode-immediate-pump-check-plan.md`. Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**