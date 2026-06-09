---
title: State Persistence and System Monitoring
summary: 24/7 reliability features for the ESP32 Pool Controller — NVS state persistence, memory monitoring, watchdog timer, health API, and troubleshooting
date: "2026-06-07"
lastmod: "2026-06-07"
draft: false
toc: true
type: docs
tags: ["docs", "persistence", "monitoring", "watchdog", "reliability"]
menu:
  docs:
    parent: Pool Controller
    name: State Persistence
    weight: 60
---

# State Persistence and System Monitoring

## Overview

The Pool Controller includes comprehensive state persistence and system
health monitoring to ensure reliable 24/7 operation.

## State Persistence

### What Gets Persisted

All controller states are automatically saved to non-volatile storage (NVS)
and restored after reboots or power failures:

#### Operation Settings

- **Operation mode**: auto, manual, boost, timer
- **Pool maximum temperature**: Target pool temperature
- **Solar minimum temperature**: Minimum solar temperature for activation
- **Temperature hysteresis**: Temperature difference for control
- **Timer settings**: Start and end times for timer mode

#### Relay States

- **Pool pump**: On/Off state
- **Solar pump**: On/Off state

### How It Works

Uses the Preferences library for persistent storage in NVS
(Non-Volatile Storage). Each value is stored with a type-specific key.

Relay states are persisted individually per relay via their own namespace
in NVS, storing the on/off state under the key `"switch"`.

### Automatic Restoration

When the controller reboots:

1. **State Manager** loads all persisted values
2. **Configuration settings** from LittleFS config.json can override persisted state
3. **Last known state** is used if no config override exists

This ensures that:

- After a power failure, pumps return to their previous state
- User-configured temperatures and timers are preserved
- Operation mode is maintained across reboots

### Example Scenario

```text
User sets:
  - Operation mode: auto
  - Pool max temp: 28.5°C
  - Timer: 10:30 - 17:30

Power failure occurs at 14:00

Controller reboots:
  - Loads saved state
  - Restores operation mode: auto
  - Restores temperatures and timers
  - Continues operation seamlessly
```

## System Health Monitoring

### Memory Monitoring

The system continuously monitors free heap memory to prevent crashes from
memory exhaustion.

#### Thresholds

**ESP32**:

- **Low Memory Warning**: < 16 KB (16,384 bytes)
- **Critical Memory**: < 8 KB (8,192 bytes) → Auto-reboot

#### Behavior

1. **Every 10 seconds**: Memory check performed
2. **Low memory**: Warning logged to serial and MQTT
3. **Critical memory**: Controller automatically reboots to recover
4. **Minimum tracking**: Tracks lowest memory point since boot

### Watchdog Timer

Prevents system hangs and ensures recovery from software failures.

#### ESP32

- **Hardware watchdog**: 30-second timeout
- **Automatic panic**: Reboots if watchdog not fed
- **Fed in main loop**: Every cycle

### Health Status API

The SystemMonitor provides methods to check system health:

```cpp
// Get current free heap
uint32_t heap = SystemMonitor::getFreeHeap();

// Get minimum heap since boot
uint32_t minHeap = SystemMonitor::getMinFreeHeap();

// Check if system is healthy
bool healthy = SystemMonitor::isHealthy();

// Get uptime in seconds
uint32_t uptime = SystemMonitor::getUptimeSeconds();

uint8_t fragmentation = SystemMonitor::getHeapFragmentation();
```

## Configuration

### Enabling Features

Both state persistence and system monitoring are **automatically enabled**. No configuration required.

### Customizing Thresholds

To customize memory thresholds, modify `src/SystemMonitor.hpp`:

```cpp
// Low memory threshold (warning only)
static constexpr uint32_t LOW_MEMORY_THRESHOLD = 8192;  // 8 KB

// Critical memory threshold (auto-reboot)
static constexpr uint32_t CRITICAL_MEMORY_THRESHOLD = 4096;  // 4 KB
```

### Disabling Auto-Reboot

If you prefer to handle low memory manually (not recommended for 24/7
operation):

Comment out the auto-reboot section in `src/SystemMonitor.hpp`:

```cpp
// Critical memory - reboot immediately
if (freeHeap < criticalThreshold) {
    Serial.printf("CRITICAL: Free heap %d bytes < %d bytes. Rebooting...\n",
                freeHeap, criticalThreshold);
    // Serial.flush();
    // delay(1000);
    // ESP.restart();  // Comment this to disable auto-reboot
}
```

## Monitoring and Logs

### Serial Output

**Normal operation**:

```text
✓ State loaded from persistent storage
State persistence and system monitoring initialized
Free heap: 28,456 bytes
```

**Low memory warning**:

```text
WARNING: Low memory detected. Free heap: 7,892 bytes (min: 7,456)
```

**Critical memory** (before reboot):

```text
CRITICAL: Free heap 3,842 bytes < 4,096 bytes. Rebooting...
```

### MQTT Logs

System status is published via the LoggerNode to MQTT topic:

```text
pool-controller/log
```

Example messages:

- `"State persistence and system monitoring initialized"`
- `"WARNING: Low memory detected. Free heap: 7892 bytes (min: 7456)"`

## Benefits for 24/7 Operation

### Reliability

- ✅ Survives power failures
- ✅ Recovers from memory issues automatically
- ✅ Detects and recovers from system hangs
- ✅ No manual intervention needed

### User Experience

- ✅ Settings preserved across reboots
- ✅ No reconfiguration after power loss
- ✅ Seamless operation continuity
- ✅ Predictable behavior

### Maintenance

- ✅ Memory leak detection
- ✅ Automatic problem recovery
- ✅ Health status monitoring
- ✅ Diagnostic information available

## Troubleshooting

### States Not Persisting

1. Check serial output for "State loaded from persistent storage"
2. Verify NVS partition is available
3. Check for Preferences errors in logs

### Frequent Reboots

If the controller reboots frequently:

1. **Check memory usage**: Review logs for low memory warnings
2. **Identify memory leak**: Look for pattern in when reboots occur
3. **Reduce memory usage**:

    - Increase measurement intervals
    - Reduce MQTT message frequency
    - Disable features if possible
4. **Lower threshold**: Temporarily lower critical threshold to prevent reboots

    (the controller will still reboot, but you may extend uptime while debugging)

### Watchdog Timeouts

If watchdog triggers (ESP32):

1. **Long-blocking operations**: Check for delays or long operations in code
2. **Increase timeout**: Modify timeout in `SystemMonitor::begin()`
3. **Feed more frequently**: Add `SystemMonitor::feedWatchdog()` in long
  operations

## Technical Details

### Storage Usage

**ESP32 NVS**:

- Operation mode: ~10 bytes
- Float values (3): 12 bytes
- Integer values (4): 16 bytes
- Total: ~40 bytes

### Performance Impact

- **State save**: < 10ms (occurs only on changes)
- **State load**: < 20ms (once at boot)
- **Memory check**: < 1ms (every 10 seconds)
- **Watchdog feed**: < 0.1ms (every loop)

**Total impact**: Negligible (< 0.1% CPU usage)

## Future Enhancements

- 🔜 **Configurable thresholds**: MQTT-based threshold configuration
- 🔜 **Health dashboard**: Web UI for health monitoring

---

**Version**: 3.3.0
**Status**: Production Ready
**Platform**: ESP32
