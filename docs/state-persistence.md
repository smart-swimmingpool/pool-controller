# State Persistence and System Monitoring

## Overview

The Pool Controller now includes comprehensive state persistence and system
health monitoring to ensure reliable 24/7 operation.

## State Persistence

### What Gets Persisted

All controller states are automatically saved to non-volatile storage and
restored after reboots or power failures:

#### Operation Settings

- **Operation mode**: auto, manual, boost, timer
- **Pool maximum temperature**: Target pool temperature
- **Solar minimum temperature**: Minimum solar temperature for activation
- **Temperature hysteresis**: Temperature difference for control
- **Timer settings**: Start and end times for timer mode

#### Relay States (ESP32)

- **Pool pump**: On/Off state
- **Solar pump**: On/Off state

### How It Works

**ESP32**: Uses the Preferences library for persistent storage in NVS
(Non-Volatile Storage).

**ESP8266**: Currently basic support (to be enhanced in future updates).

### Automatic Restoration

When the controller reboots:

1. **State Manager** loads all persisted values
2. **Configuration settings** from Homie config can override persisted state
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

**ESP8266**:

- **Low Memory Warning**: < 8 KB (8,192 bytes)
- **Critical Memory**: < 4 KB (4,096 bytes) → Auto-reboot

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

#### ESP8266

- **Software watchdog**: Built-in
- **yield() called**: Regular feeding in main loop

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

// ESP8266 only: Get heap fragmentation percentage
uint8_t fragmentation = SystemMonitor::getHeapFragmentation();
```

## Configuration

### Enabling Features

Both state persistence and system monitoring are **automatically enabled** in
version 3.1.0+. No configuration required.

### Customizing Thresholds

To customize memory thresholds, modify `src/SystemMonitor.hpp`:

```cpp
// Low memory threshold (warning only)
static constexpr uint32_t LOW_MEMORY_THRESHOLD = 8192;  // ESP8266

// Critical memory threshold (auto-reboot)
static constexpr uint32_t CRITICAL_MEMORY_THRESHOLD = 4096;  // ESP8266
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
homie/pool-controller/log
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

**ESP32**:

1. Check serial output for "State loaded from persistent storage"
2. Verify NVS partition is available
3. Check for Preferences errors in logs

**ESP8266**:

1. Currently limited support
2. Will be enhanced in future updates
3. Relay states not persisted on ESP8266

### Frequent Reboots

If the controller reboots frequently:

1. **Check memory usage**: Review logs for low memory warnings
2. **Identify memory leak**: Look for pattern in when reboots occur
3. **Reduce memory usage**:
   - Increase measurement intervals
   - Reduce MQTT message frequency
   - Disable features if possible
4. **Lower threshold**: Temporarily lower critical threshold to prevent reboots
   while debugging

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

**ESP8266 EEPROM**:

- Reserved: 512 bytes (for future expansion)
- Currently minimal usage

### Performance Impact

- **State save**: < 10ms (occurs only on changes)
- **State load**: < 20ms (once at boot)
- **Memory check**: < 1ms (every 10 seconds)
- **Watchdog feed**: < 0.1ms (every loop)

**Total impact**: Negligible (< 0.1% CPU usage)

## Future Enhancements

Planned improvements:

1. **ESP8266 full persistence**: Complete EEPROM implementation
2. **Configurable thresholds**: MQTT-based threshold configuration
3. **Memory stats**: Historical memory usage tracking
4. **Remote reboot**: MQTT command to trigger reboot
5. **Health dashboard**: Web UI for health monitoring
6. **Smart recovery**: Different strategies based on failure type

---

**Version**: 3.1.0+
**Status**: Production Ready
**Platforms**: ESP32 (full support), ESP8266 (partial support)
