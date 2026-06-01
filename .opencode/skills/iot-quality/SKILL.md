---
name: iot-quality
description: "IoT quality assurance for the ESP32 pool-controller — static analysis, anti-pattern detection, code review rules, hardening for 24/7 operation, and systematic quality gates. Use when asked to review code quality, detect embedded-specific bugs, enforce IoT best practices, or verify production readiness. 🇩🇪 Deutsche Trigger: Qualitätssicherung, Statische Analyse, Anti-Patterns, Code-Review, Embedded Bugs, IoT Best Practices, Production Readiness, Quality Gates, Speicherleck-Erkennung."
keywords:
  - qualitätssicherung
  - quality assurance
  - statische analyse
  - static analysis
  - anti-patterns
  - code review
  - embedded bugs
  - iot best practices
  - production readiness
  - produktionsreife
  - quality gates
  - speicherleck
  - memory leak
  - codestandards
  - coding standards
  - pre-merge checkliste
---

# IoT Quality — Pool Controller

Quality assurance specific to embedded/IoT firmware for the pool-controller. This goes beyond general C++ quality — it targets patterns that cause failures in always-on resource-constrained devices.

> **🔍 Code Search**: Use `semble search "delay("` to find blocking patterns, `semble search "String"` for heap allocations. `semble find-related` helps trace anti-pattern instances across modules. See `Agents.md` §7 for full `semble` usage.

## Quality Gates (Required Before Merge)

```
┌─────────────────────────────────────────────┐
│              PRE-MERGE CHECKLIST             │
├─────────────────────────────────────────────┤
│ □ 1. `make build` — compilation passes       │
│ □ 2. `make lint-fix && make lint` — no new   │
│       warnings/errors                        │
│ □ 3. No new `String` in loop/hot-paths       │
│ □ 4. No blocking patterns (delay >100ms)     │
│ □ 5. `feedWatchdog()` reachable in all paths  │
│ □ 6. Heap/stack analysis: no new allocations  │
│        in critical paths                      │
│ □ 7. ESP-IDF 5.x API compatibility            │
│ □ 8. No new global `static` that could be     │
│        function-local                          │
│ □ 9. MQTT publish payload fits buffer         │
│ □ 10. NVS prefs.end() called in all paths     │
└─────────────────────────────────────────────┘
```

## 1. Anti-Pattern Detection (from `Agents.md` §19)

These patterns are **forbidden** in this project. Scan every PR for them:

| Anti-Pattern | Why It's Banned | File to Check |
|-------------|----------------|---------------|
| Unbounded `delay()` in loop | Blocks watchdog, starvation | All `loop()` methods |
| Busy-wait loops | CPU waste, watchdog miss | Any `while(!condition)` |
| Blocking network calls in loop | Starvation of other tasks | `NetworkManager` |
| Heap allocation in hot-paths | Fragmentation, OOM | MQTT/Sensor loops |
| `String` objects in cycle code | Fragmentation | `MqttPublisher`, `WebPortal` |
| Global state without sync | Race conditions (FreeRTOS) | Static class members |
| Large buffers (>512B) as file-scope `static` | RAM waste if used once | Setup-only code |
| Uninitialized pointer members | UB → hard crash | All constructors |
| Static `String` with `.concat()` no reset | Content doubling | `WpsProvisioner.cpp:34` (atomic ✓) |

### Detailed Check: String Usage

The `Agents.md` §19 says: "String-Objekte in Zyklus-Code" are verboten.

**Check these high-risk files**:
- `MqttPublisher.cpp` — HA discovery JSON construction
- `WebPortal.cpp` — HTTP response building
- `ConfigManager.cpp` — config serialization (currently uses `result += buf` pattern at line 32-33)
- `NetworkManager.cpp:144` — `String clientId = "pool-controller-" + String(...)` (setup only, acceptable)

**Fix pattern**:
```cpp
// BEFORE (bad — reallocation in loop)
String result = "";
for (int i = 0; i < 32; i++) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", hash[i]);
    result += buf;  // Each += can reallocate!
}

// AFTER (good — fixed buffer)
char result[65];
for (int i = 0; i < 32; i++) {
    snprintf(result + (i * 2), 3, "%02x", hash[i]);
}
result[64] = '\0';
```

### Detailed Check: Blocking Patterns

The main loop at `PoolController.cpp:180-232` must never block for more than a few ms.

**Check these**:
- `delay(5000)` at `PoolController.cpp:77` — only in `initializeController()` on fatal error (acceptable — it's followed by `ESP.restart()`)
- `delay(1000)` at `SystemMonitor.hpp:92` — before reboot on critical memory (acceptable)
- Any `while(!condition)` without yield/watchdog feed — **banned**

### Detailed Check: ESP-IDF 5.x Compatibility

The project uses `espressif32 @ 6.9.0` which includes ESP-IDF 5.x.

**Known API breaks** (from `Agents.md` §20):

| API | ESP-IDF 4.x | ESP-IDF 5.x |
|-----|-------------|-------------|
| Task Watchdog Init | `esp_task_wdt_init(uint32_t timeout, bool panic)` | `esp_task_wdt_init(const esp_task_wdt_config_t *)` |
| Task Watchdog Reconfig | N/A | `esp_task_wdt_reconfigure(const esp_task_wdt_config_t *)` |
| WPS Start | `esp_wifi_wps_start(int)` | `esp_wifi_wps_start()` |

**Already handled in this project**:
- `SystemMonitor.hpp:48-54` — uses `#if ESP_IDF_VERSION` for WDT API ✓
- `WpsProvisioner.cpp:68-72` — uses `#if ESP_IDF_VERSION >= ...` for WPS API ✓

**Check new code** for use of these APIs without version guards.

## 2. 24/7 Operation Hardening

### Watchdog Coverage

Every `loop()` function must ensure `SystemMonitor::feedWatchdog()` is callable:
- Check: all code paths through `PoolController::loop()` reach the watchdog call at line 183
- Check: no component's `loop()` method blocks longer than the TWDT timeout (30s)
- Check: long operations use incremental processing or `vTaskDelay`

### NVS Write Endurance

ESP32 NVS (Preferences) has ~100K write cycles per key.

**High-write-risk areas**:
- `OperationModeNode::saveState()` — called on every setting change
- `StateManager::save*()` — called by multiple setters

**Mitigations**:
- Already uses `_suppressPersist` flag during bulk setup (`PoolController.cpp:167`)
- Consider debouncing rapid setting changes
- Consider wear-leveling or writing only if value changed

### Boot-Loop Recovery

**Already implemented** (P8 in `SystemMonitor.hpp:129-178`):
- 3 consecutive short boots (<5 min) → Safe Mode
- Safe Mode: all relays OFF, relay state cleared from NVS
- After 5 min stable → counter cleared, normal operation resumes

**Verify on changes**: Any new initialization code must not interfere with boot-loop detection (which runs at the very start of `setup()`).

### Memory Leak Detection

**Static analysis**: `pio check` catches some leaks.

**Runtime checks** — add temporarily:
```cpp
// In PoolController::loop(), monitor leak trends:
static uint32_t lastHeapCheck = 0;
if (millis() - lastHeapCheck > 60000) {
    lastHeapCheck = millis();
    Serial.printf("HEAP_STATS: free=%u min=%u largest=%u\n",
        ESP.getFreeHeap(),
        SystemMonitor::getMinFreeHeap(),
        heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
}
```

## 3. Code Review Rules (Project-Specific)

### File Organization

**Current structure** (`src/` is flat — all modules mixed):
```
src/
├── main.cpp            ← Entry point
├── PoolController.*    ← Orchestrator
├── *Node.*             ← Temperature/Relay nodes
├── Rule*.*             ← Operation mode rules
├── *Manager.*          ← System managers (Network, Config, State, System, Degradation)
├── *Helper.*           ← Utilities (TimeClient)
├── Config.*            ← Pin definitions
├── Utils.*             ← Helpers
└── Nodes/              ← Logger sub-node
```

**Review rule**: New modules should follow the layered pattern from `Agents.md` §3:
- `app/` — business logic
- `drivers/` — hardware abstractions
- `services/` — infrastructure
- `platform/` — board adapters

### Singleton Pattern

Most managers use static singletons:
```cpp
static void Manager::begin();
static void Manager::loop();
```

**Review rule**: New singletons must:
1. Have a clear lifecycle (begin/loop/end)
2. Not depend on other singletons' internal state
3. Handle `begin()` being called multiple times (idempotent)

### Error Handling

**Must do**:
- Check return values of all ESP-IDF calls (`esp_err_t`)
- Check ArduinoJson deserialization results
- Validate sensor readings (NaN, out-of-range)
- Log errors with enough context to diagnose remotely

**Must not do**:
- Silently ignore errors
- `assert()` in production code paths (crashes the device)
- Infinite retry without backoff

### Serial Logging Discipline

- Log level appropriate: `Serial.printf` for info/warning, `Serial.println` for errors
- No credentials in logs (WiFi password, MQTT password)
- Rate-limit repetitive logs (especially in reconnect loops)
- Use structured format: `KEY=VALUE` for machine-parseable logs

## 4. Automated Quality Checks

### In CI (GitHub Actions):
- **Super-Linter**: cpplint, EditorConfig, Markdown, YAML, JSON, GHA, Gitleaks, Bash
- **PlatformIO Check**: static analysis on source
- **PlatformIO Build**: compilation for esp32dev

### Missing CI (future work):
- `platformio test` — unit tests (none exist yet; no `test/` directory)
- Memory analysis (heap/stack usage report)
- Code coverage
- Compiler warnings as errors

### Notable: Stale Dependencies
- `DHT sensor library` listed in `platformio.ini` lib_deps but **not used** in any source file — candidate for cleanup

### Local Make Targets:
```bash
make lint      # Super-Linter (Docker)
make lint-fix  # clang-format + prettier auto-fix
make build     # pio run -e esp32dev
make clean     # Clean build artifacts
```

## 5. Production Readiness Checklist

Before any release/tag:

- [ ] `make build` passes for esp32dev
- [ ] `make lint` passes with zero warnings
- [ ] All serial debug output reviewed (no secrets leaked)
- [ ] Static analysis: `pio check` has no new findings
- [ ] Heap: `ESP.getFreeHeap()` at startup is stable (no drift after 24h)
- [ ] Watchdog: TWDT configured and fed in all paths
- [ ] Boot-loop: counter resets after 5 min stable operation
- [ ] MQTT: Discovery payloads fit in buffers (< buffer margin)
- [ ] NVS: All prefs.begin/end pairs are properly closed
- [ ] No `String` allocations in loop() code paths
- [ ] No unbounded `delay()` or busy-wait added
- [ ] Version bump in `platformio.ini` (FW_VERSION)
- [ ] CHANGELOG.md updated
- [ ] Release tag matches `release-please` config

## 6. Common CR Comments for This Project

| Issue | Comment Template |
|-------|-----------------|
| String in loop | "This `String` concatenation in the loop path will fragment the heap. Use a pre-allocated `char[]` buffer and `snprintf` instead." |
| Missing watchdog feed | "This code path can block for more than 30s without feeding the watchdog. Restructure with incremental processing or add `SystemMonitor::feedWatchdog()` calls." |
| No ESP-IDF guard | "This API changed between ESP-IDF 4.x and 5.x. Wrap it with `#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)` for compatibility." |
| File-scope static buffer | "This large buffer (>512B) is only used in setup(). Make it function-local to free RAM after initialization." |
| NVS prefs not closed | "Preferences::end() must be called to avoid leaking NVS handles." |
| setInsecure() | "setInsecure() skips TLS certificate validation. For production, use setCACert() with the broker's CA certificate." |
