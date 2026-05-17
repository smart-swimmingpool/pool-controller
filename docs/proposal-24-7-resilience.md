# 24/7 Operation: Resilience Concept for the Pool Controller

> **Status**: Phase 1+2 implemented, Phase 3 in progress, ESP8266 removal complete  
> **Date**: 2026-05-17  
> **Branch**: `proposal/24-7-resilience`  
> **Base**: Version 3.1.0 → 3.2.0  

---

## 1. Introduction

The Pool Controller must run **24/7 autonomously** without manual intervention,
even under:

- **Power failure**: Controller reboots with sensible state when power returns
- **WiFi outage**: Continues controlling pumps autonomously (partially implemented)
- **MQTT broker outage**: Runs standalone, buffers state changes
- **Temperature sensor failure**: Degrades gracefully instead of crashing

### Current Maturity

Already implemented in v3.1.0:

| Mechanism | Status |
|---|---|
| State Persistence (ESP32 Preferences, ~~ESP8266 EEPROM~~) | ✅ |
| Relay state restoration after reboot | ✅ |
| `setRunLoopDisconnected(true)` for offline operation | ✅ |
| SystemMonitor with watchdog (ESP32 HW) | ✅ |
| Memory monitoring with auto-reboot on critical | ✅ |
| NTP time caching with millis() fallback | ✅ |
| NaN validation for sensor values in Rules | ✅ |
| Pin conflict detection at startup | ✅ |
| Timer midnight-crossing logic | ✅ |
| **State-Load independent of MQTT (P1)** | **✅ Since 840f3a8** |
| **~~ESP8266 EEPROM collision fix (P3)~~** | **✅ Since 840f3a8 (obsolete)** |
| **Watchdog feeding in long ops (P6)** | **✅ Since 840f3a8** |
| **DegradationManager (P5)** | **✅ Since 50be817** |
| **NTP three-stage degradation (P2)** | **✅ Since c969663** |
| **MQTT state refresh on reconnect (P4)** | **✅ Since 6eedebf** |
| **ESP8266 support removed** | **✅ 3.2.0** |

This proposal addresses the **remaining gaps** for truly resilient 24/7 operation.

---

## 2. Remaining Weakness Analysis

### 🔴 Critical: State Load Depends on MQTT Connection

**Problem**: `operationModeNode.loadState()` runs inside `setupHandler()`,
which only fires when Homie establishes an MQTT connection.

Affected code:
```cpp
// PoolController.cpp:250
auto PoolControllerContext::setupHandler() -> void {
  StateManager::begin();
  SystemMonitor::begin();
  operationModeNode.loadState();  // ← NOT called without MQTT!
}

// PoolController.cpp:343
auto PoolControllerContext::setup() -> void {
  Homie.setup();
  initializeController();  // Creates rules, but does NOT load states
}
```

**Scenario**: Power failure + WiFi outage → Controller boots → Homie can't
connect MQTT → `setupHandler()` never fires → Mode defaults to HomieSetting
default ("auto"), not the previously saved mode. Temperature thresholds,
timer settings also lost.

**Impact**: Combined power+WiFi failure loses all user settings until WiFi
and MQTT are restored.

### 🟡 High: Pump Logic During Extended NTP Failure

**Problem**: `checkPoolPumpTimer()` immediately disables the pump when
`time.tm_year == -1` (no valid NTP). After 24h without sync, the time
estimate becomes "invalid".

```cpp
// RuleAuto.cpp:89
if (time.tm_year == -1) {
  return false;  // Pump OFF
}
```

**Impact**: Extended WiFi outage (>24h) with Timer mode stops the pump
completely, even if the estimated time is still plausible. Auto mode
only affects the solar pump in this case, but this is undocumented.

### 🟡 High: ESP8266 EEPROM Hash Collisions

**Problem**: The DJB2 hash across only 15 slots of 32 bytes is
collision-prone. With 8 stored keys, collision probability is ~85%.

```cpp
// StateManager.cpp:28
return EEPROM_DATA_START + (hash % EEPROM_SLOT_COUNT) * EEPROM_SLOT_SIZE;
// 15 slots for 8+ keys → ~85% collision probability
```

**Impact**: Settings overwrite each other → unpredictable behavior after
ESP8266 reboot.

### 🟡 High: Startup Order Between initializeController and Homie Node Setup

**Problem**: `PoolControllerContext::setup()` calls `Homie.setup()` followed
by `initializeController()`. The `OperationModeNode::setup()` (Homie node
setup) runs inside `Homie.setup()`. But `initializeController()` also reads
HomieSettings and configures nodes.

The order is:
1. Global constructors: all static nodes
2. `PoolControllerContext::setup()` → `Homie.setup()` (node setups execute)
3. `initializeController()` → creates Rules via `new`, sets intervals

**Problem**: Rules created with raw `new` in `initializeController()` are
owned by the Vector in `OperationModeNode`. The destructor deletes them,
which could cause use-after-free if Homie internally recreates nodes.

### 🟠 Medium: Silent MQTT Publish Failures

**Problem**: Many `MqttInterface` methods check `Homie.isConnected()` but
don't check publish return values. If the connection drops during publishing,
the failure is silent.

```cpp
// MqttInterface.hpp:68
inline void publishSwitchState(...) {
  if (isHomeAssistant()) {
    HomeAssistant::DiscoveryPublisher::publishSwitchState(...);
    // Return value ignored!
  } else {
    node.setProperty(homieProperty).send(state ? "true" : "false");
    // Result ignored!
  }
}
```

**Impact**: State updates are lost. No retransmission on reconnect →
Home Assistant may show stale states.

### 🟠 Medium: No Watchdog Feeding in Long Operations

**Problem**: The watchdog (30s on ESP32) is only fed in the main loop.
Long operations could trigger it.

```cpp
// PoolController.cpp:391
auto PoolControllerContext::loop() -> void {
  SystemMonitor::feedWatchdog();
  SystemMonitor::checkMemory();
  Homie.loop();
}
```

**Recommendation**: Feed watchdog inside potentially long operations.

### 🟠 Medium: No Retry/Backoff for Sensor Recovery

**Problem**: When a Dallas sensor disconnects, `_temperature` stays NaN
without accelerated re-polling.

**Impact**: After a sensor fault, it can take minutes to recover because
no shortened polling interval is active.

### 🔵 Low: millis() Overflow After ~49 Days

**Problem**: `millis()` overflows after ~49 days on ESP8266/ESP32.
`Utils::shouldMeasure()` handles this correctly via unsigned arithmetic,
but there's no monitoring or testing for this.

---

## 3. Proposed Solutions

### P1: Decouple State Load from MQTT 🔴 Critical

**Goal**: Persisted states always load, regardless of WiFi/MQTT status.

**Solution A (recommended) — Load in `initializeController()`**:

```cpp
auto PoolControllerContext::setup() -> void {
  Homie.setup();

  // StateManager and SystemMonitor always initialize,
  // not only on MQTT connect
  StateManager::begin();
  SystemMonitor::begin();

  initializeController();
  operationModeNode.loadState();  // Always runs
}
```

HomieSettings then serve only as **factory defaults**, overridden by
persisted values.

**Solution B — Fallback chain**: `initializeController()` checks
`Homie.isConnected()` and calls `loadState()`. On first MQTT connect,
`loadState()` is called again (overwriting potentially newer values).

**Effort**: 1–2 days  
**Risk**: Low

### P2: Graceful Degradation for NTP Failure 🟡 High

**Goal**: Don't shut everything down when time is uncertain; degrade
intelligently.

**Approach — Three-Level Model**:

| Level | Condition | Behavior |
|---|---|---|
| **Green** | last NTP sync < 1h | Normal operation |
| **Yellow** | 1h–24h since last sync | Timer runs with millis() estimate, warning via MQTT/Serial |
| **Red** | >24h since last sync **or** time jump | Timer mode falls back to Auto mode, pump runs on temperature logic |

```cpp
// Proposed RuleAuto::checkPoolPumpTimer()
bool RuleAuto::checkPoolPumpTimer() {
  tm time = getCurrentDateTime();

  if (time.tm_year == -1) {
    auto degradation = getTimeDegradationLevel();

    if (degradation == TimeDegradation::YELLOW) {
      return checkPumpTimerWithEstimate();
    }

    // RED: fallback to temperature control or last known on/off time
    return getFallbackPumpState();
  }
  return checkPumpTimerExact(time);
}
```

**Effort**: 2–3 days  
**Risk**: Medium

### P3: Fix ESP8266 EEPROM Collisions 🟡 High

**Solution A (recommended) — Structured slot layout**:

```cpp
enum EEPROMSlot : uint16_t {
  SLOT_MAGIC      = 0,     // 4 bytes Magic Number
  SLOT_OPMODE     = 4,     // 32 bytes: Mode String
  SLOT_POOL_TEMP  = 36,    // 4 bytes: float
  SLOT_SOLAR_TEMP = 40,    // 4 bytes: float
  SLOT_HYSTERESIS = 44,    // 4 bytes: float
  SLOT_TIMER_START_H = 48, // 4 bytes: int
  SLOT_TIMER_START_M = 52, // 4 bytes: int
  SLOT_TIMER_END_H   = 56, // 4 bytes: int
  SLOT_TIMER_END_M   = 60, // 4 bytes: int
  SLOT_RELAY_POOL    = 64, // 1 byte: bool
  SLOT_RELAY_SOLAR   = 65, // 1 byte: bool
  SLOT_CHECKSUM      = 66, // 2 bytes: CRC16 over slots 4-65
  SLOT_END           = 68   // Fits in 512 bytes EEPROM
};
```

**Benefits**:
- No collisions
- CRC protects against corruption
- Easily extensible
- Faster than hashing

**Effort**: 1–2 days  
**Risk**: Medium (breaks compatibility with existing EEPROM content)

### P4: MQTT Publish Retry & Reconnect Refresh 🟠 Medium

**Goal**: No lost state updates during temporary MQTT outages.

**Approach — Dirty-Flag Queue**:

```cpp
class MqttPublishQueue {
  // ...
public:
  bool publish(const char* topic, const char* payload, bool retained);
  void onReconnect();  // Sends all queued messages
};

// In Homie onReadyToOperate callback:
void PoolControllerContext::setupHandler() {
  MqttPublishQueue::onReconnect();  // Re-send missed updates
}
```

**Effort**: 2–3 days  
**Risk**: Low

### P5: Explicit Degradation Strategy 🟠 Medium

**Goal**: Clearly defined operational states with documented behavior.

```cpp
enum class DegradationLevel {
  NORMAL,        // Everything OK
  NO_WIFI,       // WiFi down, running, no MQTT updates
  NO_TIME,       // Time uncertain, timer fallback active
  NO_SENSOR,     // Sensor faulty, pump with cautious values
  CRITICAL,      // Multiple failures → Safe Mode
};
```

| Level | Pool Pump | Solar Pump | Logging |
|---|---|---|---|
| NORMAL | Rule-based | Rule-based | MQTT + Serial |
| NO_WIFI | Rule-based | Rule-based | Serial |
| NO_TIME | Auto mode | Rule-based | Serial + Warning |
| NO_SENSOR | Last good value (max 1h) | OFF | Serial + Alarm |
| CRITICAL | Manual (last state) | OFF | Serial + Persistent Warning |

**Effort**: 2–3 days  
**Risk**: Medium

### P6: Watchdog Feeding in Long Operations 🟠 Medium

```cpp
// DallasTemperatureNode.cpp
void DallasTemperatureNode::loop() {
  SystemMonitor::feedWatchdog();

  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    sensor.requestTemperatures();
    SystemMonitor::feedWatchdog();
    _temperature = sensor.getTempCByIndex(0);
  }
}
```

**Effort**: 0.5 days  
**Risk**: None

### P7: Accelerated Sensor Recovery 🟠 Medium

```cpp
void DallasTemperatureNode::loop() {
  uint32_t effectiveInterval = isnan(_temperature)
    ? RECOVERY_INTERVAL  // e.g. 5 seconds instead of 60
    : _measurementInterval;

  if (Utils::shouldMeasure(_lastMeasurement, effectiveInterval)) {
    // ... read sensor
    if (isnan(_temperature)) {
      _lastMeasurement = millis();  // Next try in RECOVERY_INTERVAL
    }
  }
}
```

**Effort**: 1 day  
**Risk**: Low

### P8: Boot-Loop Detection via Boot Counter 🟠 Medium

**Goal**: Detect repeated power failures or startup crashes.

```cpp
void detectBootLoop() {
  int bootCount = StateManager::loadInt("bootCount", 0);
  uint32_t lastUptime = StateManager::loadInt("lastBootUptime", 0);

  if (lastUptime < 300 && bootCount > 2) {
    enterSafeMode();  // All relays OFF, base functions only
  }

  StateManager::saveInt("bootCount", bootCount + 1);
  StateManager::saveInt("lastBootUptime", SystemMonitor::getUptimeSeconds());
}
```

**Effort**: 1 day  
**Risk**: Low

### P9: Configurable Fallback Behavior 🟠 Medium

**Goal**: User-configurable behavior during time loss.

```cpp
HomieSetting<const char*> fallbackModeSetting_{"fallback-mode",
  "Operation mode on time loss (auto/manu/off)"};
HomieSetting<bool> keepPumpOnTimeLossSetting_{"keep-pump-on-time-loss",
  "Keep pump running on time loss (true/false)"};
HomieSetting<long> timeLossMaxHoursSetting_{"time-loss-max-hours",
  "Max hours without time sync before fallback activates (1-72)"};
```

**Effort**: 1 day  
**Risk**: Low

---

## 4. Prioritized Roadmap

### Phase 1: Critical Stability (Immediate)

| Priority | Proposal | Effort |
|---|---|---|
| 🔴 P1 | Decouple State Load from MQTT | 1–2 days |
| 🟡 P3 | Fix ESP8266 EEPROM collisions | 1–2 days |
| 🟠 P6 | Watchdog feeding in long ops | 0.5 days |

### Phase 2: Graceful Degradation (Next 2 weeks)

| Priority | Proposal | Effort |
|---|---|---|
| 🟡 P2 | Graceful degradation for NTP failure | 2–3 days |
| 🟠 P4 | MQTT publish retry & reconnect refresh | 2–3 days |
| 🟠 P5 | Explicit degradation strategy | 2–3 days |

### Phase 3: Proactive Resilience 🔄 In Progress

| Priority | Proposal | Effort |
|---|---|---|
| 🟠 P7 | Accelerated sensor recovery | 1 day |
| 🟠 P8 | Boot-loop detection + Safe Mode | 1 day |
| 🟠 P9 | Configurable fallback behavior | 1 day |

---

## 5. Testing & Verification

| Proposal | Test Scenario |
|---|---|
| P1 | Boot without WiFi → states loaded ✓. Boot without WiFi after mode change → new mode active |
| P2 | NTP blocked (1h, 6h, 24h, 48h) → correct degradation. Timer falls back to Auto |
| P3 | All 9 keys write → read back → correct values. CRC corruption → defaults used |
| P4 | MQTT outage 5 min → 10 state changes → after reconnect all 10 correctly published |
| P5 | Simulated sensor failure → degradation to NO_SENSOR → correct pump behavior |
| P6 | 500ms blocking call → watchdog not triggered |
| P7 | Sensor disconnect → recovery in <10s instead of >30s |
| P8 | 5 short boots in sequence → Safe Mode active. One long boot → counter reset |
| P9 | Fallback mode set via MQTT to "manu" → pump runs on last command during time loss |

---

## 6. Architecture Impact

### New Files

```
src/
├── DegradationManager.hpp   (P5)
├── DegradationManager.cpp   (P5)
├── MqttPublishQueue.hpp     (P4)
├── MqttPublishQueue.cpp     (P4)
└── BootGuard.hpp            (P8)
```

### Changed Files

| File | Changes |
|---|---|
| `PoolController.cpp` | State load in `setup()`, DegradationManager loop |
| `StateManager.cpp` | ESP8266 EEPROM new layout (P3) |
| `DallasTemperatureNode.cpp` | Recovery interval on NaN (P7), watchdog feed (P6) |
| `OperationModeNode.cpp` | Fallback logic on time loss (P2) |
| `RuleAuto.cpp` | Three-level timer (P2) |
| `RuleTimer.cpp` | Three-level timer (P2) |
| `SystemMonitor.hpp/cpp` | Degradation integration (P5) |
| `MqttInterface.hpp` | Publish queue integration (P4) |
| `HomeAssistantMQTT.hpp/cpp` | Queue-capable publishes (P4) |
| `Config.hpp` | New HomieSettings for fallback (P9) |

### Backward Compatibility

- P3 (EEPROM layout) breaks compatibility with existing ESP8266 installations.
  Migration: first boot after update reinitializes EEPROM.
- All other proposals are additive changes with no breakage.

---

## 7. Summary

The system is **fundamentally well-positioned** for 24/7 operation (see
existing features), but has several critical gaps:

1. **🔴 P1** is the most urgent: without it, combined power+WiFi failure
   loses all settings.
2. **🟡 P2 + P3** address the two most common extended-failure scenarios
   (WiFi outage >24h, ESP8266 storage corruption).
3. **🟠 P4–P9** move the system from "mostly works" to "runs reliably
   under all conditions".

> **Recommendation**: Implement P1, P3, and P6 in a first sprint, then
> P2+P5 as a second step, remaining proposals as needed.

---

## 8. Appendix: Current Architecture (Reference)

```
ESP boot
  → setup()
    → Homie.setup()
      → (internal) Node setups: RelayModuleNode, OperationModeNode
      → [if MQTT connects] setupHandler()
        → StateManager::begin()
        → SystemMonitor::begin()
        → operationModeNode.loadState()     ← CRITICAL: only here!
    → initializeController()
      → Create Rules
      → Set intervals
  → loop()
    → SystemMonitor::feedWatchdog()
    → SystemMonitor::checkMemory()
    → Homie.loop()
      → OperationModeNode::loop()
        → Rule::loop() (temperature-based pump control)
```

```
Data flow during WiFi outage:

Sensor reads → Temperature updated → Rule evaluates →
Relay setSwitch() → GPIO toggles → State persisted to NVS/EEPROM

→ MQTT publish skipped (Homie.isConnected() == false)
→ No retry on reconnect
```

---

*End of proposal. For questions or change requests please open an issue.*
