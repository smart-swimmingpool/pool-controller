---
name: esp32-reliability
description: "24/7 reliability features for the pool-controller — watchdog, memory monitoring, boot-loop detection, degradation management, and sensor recovery. Use when asked to debug, test, or improve the always-on reliability systems. 🇩🇪 Deutsche Trigger: 24/7 Betrieb, Watchdog, Speicherüberwachung, Boot-Loop Erkennung, Degradationsmanagement, Sensor-Wiederherstellung, Ausfallsicherheit, Selbstheilung."
keywords:
  - 24/7 betrieb
  - zuverlässigkeit
  - reliability
  - watchdog
  - speicherüberwachung
  - memory monitoring
  - boot-loop erkennung
  - boot loop detection
  - degradation
  - degradationsmanagement
  - sensor wiederherstellung
  - sensor recovery
  - ausfallsicherheit
  - selbstheilung
  - self healing
  - safe mode
  - sicherer modus
---

# ESP32 Reliability — Pool Controller

24/7 operation reliability systems for the pool-controller. These features ensure the system runs autonomously without human intervention.

> **🔍 Code Search**: Use `semble search "feedWatchdog"` or `semble search "detectBootLoop"` to find reliability-related code. `semble find-related` helps trace degradation paths across components. See `Agents.md` §7 for full `semble` usage.

## Reliability Architecture

```
┌─────────────────────────────────────────────────────┐
│                PoolController::loop()                │
├─────────────────────────────────────────────────────┤
│ 1. SystemMonitor::feedWatchdog()    ← TWDT reset     │
│ 2. SystemMonitor::checkMemory()     ← every 10s      │
│ 3. DegradationManager::evaluate()   ← every 5s       │
│ 4. Boot-loop counter check          ← after 5min      │
│ 5. Normal: driver loops + MQTT                        │
└─────────────────────────────────────────────────────┘
```

## 1. Hardware Watchdog (TWDT)

**Location**: `SystemMonitor.hpp:43-59`

- **Timeout**: 30 seconds
- **Panic on timeout**: true (triggers ESP32 reboot)
- **ESP-IDF 5.x note**: Arduino framework pre-initializes TWDT, so use `esp_task_wdt_reconfigure()` not `esp_task_wdt_init()`

**Must call** `SystemMonitor::feedWatchdog()` in every `loop()` iteration (already at `PoolController.cpp:183`).

**Testing**: To verify watchdog triggers, add a blocking `delay(35000)` — serial output will stop and ESP32 should reboot after 30s.

## 2. Memory Monitoring

**Location**: `SystemMonitor.hpp:72-105`

| Threshold | Value | Action |
|-----------|-------|--------|
| Warning | <16384 bytes (16KB) | Log warning once |
| Critical | <8192 bytes (8KB) | Immediate `ESP.restart()` |

**Check interval**: Every 10 seconds in `loop()`.

**Tracking**: `minFreeHeap` tracks the all-time-low since boot, available via `SystemMonitor::getMinFreeHeap()`.

## 3. Boot-Loop Detection (P8)

**Location**: `SystemMonitor.hpp:129-178`, `PoolController.cpp:138-200`

**Mechanism**:
1. `SystemMonitor::detectBootLoop()` called at start of `setup()` (before anything initializes)
2. Increments NVS-stored `bootCount` on each boot
3. If `bootCount >= 3` → returns true (boot-loop detected)
4. Safe mode: **all relays forced OFF**, relay Preferences cleared
5. After 5 minutes stable operation → `clearBootLoopCounter()` resets counter

**NVS namespace**: `sysmon`, key: `bootCount`

**Testing**: To simulate a boot-loop, set the counter:
```cpp
Preferences prefs;
prefs.begin("sysmon", false);
prefs.putInt("bootCount", 3);
prefs.end();
```
Then reboot — serial should show:
```
✖ BOOT-LOOP DETECTED (3 consecutive boots)
✖ SAFE MODE ACTIVE — all relays forced OFF
```

## 4. Degradation Manager

**Location**: `DegradationManager.hpp/cpp`

**Degradation Levels** (enum class `DegradationLevel`):

| Level | Value | Meaning |
|-------|-------|---------|
| `NORMAL` | 0 | Everything nominal |
| `NO_WIFI` | 1 | WiFi/MQTT lost, local operation still works |
| `NO_TIME` | 2 | NTP sync lost, timer scheduling degraded |
| `NO_SENSOR` | 3 | Temperature sensor failure, cautious defaults |
| `CRITICAL` | 4 | Multiple failures or critically low memory → safe mode |

**Evaluation**: `DegradationManager::evaluate()` runs every 5 seconds (`EVALUATION_INTERVAL_MS`).

**Integration points**:
- `DallasTemperatureNode` → `reportSensorStatus(nodeId, valid)` on each read
- `RuleAuto` / `RuleTimer` → `getLevel()` for time degradation decisions
- `PoolController::loop()` → `DegradationManager::evaluate()` every loop

**Safe mode**: `DegradationManager::isSafe()` returns true when level ≥ CRITICAL.
- Can be forced via `forceSafeMode()` (used by boot-loop detection)
- Released via `unforceSafeMode()` (when boot-loop is cleared)

## 5. Fast Sensor Recovery (P7)

**Location**: `DallasTemperatureNode.cpp`

When sensor reads `NaN` (Not a Number):
- Re-poll interval drops from 300s to **5s** for faster recovery
- Once sensor returns valid reading → back to normal interval

## 6. NTP Graceful Degradation (3-Stage)

**Location**: `TimeClientHelper.hpp/cpp`

From `PoolController.cpp:92-94` — configurable via `ConfigManager`:
- **Green hours** (`time-loss-green-hours`): Time considered reliable after NTP loss
- **Red hours** (`time-loss-red-hours`): After this, time is considered unreliable

Timer rules check degradation state — when `NO_TIME`, pool timer defaults to ON (circulation for hygiene).

## 7. State Persistence

**Location**: `StateManager.hpp/cpp`

Uses ESP32 NVS (Preferences). Persists:
- Operation mode and settings
- Temperature thresholds (pool max, solar min, hysteresis)
- Timer configurations
- Relay states

Auto-restored after power failure or reboot.

## Reliability Checklist

When modifying reliability code, verify:
- [ ] `SystemMonitor::feedWatchdog()` is called every loop iteration
- [ ] New blocking code doesn't exceed 30s (watchdog timeout)
- [ ] NVS `prefs.end()` is called after every open
- [ ] No `static` String or buffer in hot-paths (heap fragmentation)
- [ ] Boot-loop detection path sets safe mode before any relay can turn on
- [ ] Degradation level transitions are logged
- [ ] Sensor failure doesn't cascade to other systems
- [ ] ESP-IDF 5.x API compatibility (esp_task_wdt_reconfigure)
