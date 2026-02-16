# Edge Case Analysis Report

## Pool Controller - Potential Edge Cases & Reliability Issues

**Date**: 2026-02-16  
**Version**: 3.1.0  
**Analysis Scope**: Complete codebase review for edge cases and failure modes

**Status Update**: This analysis was performed and many issues were addressed
in version 3.1.0. Issues marked with ✅ **FIXED in v3.1.0** have been
resolved. Issues without this marker remain as recommendations for future
improvement.

---

## Executive Summary

This document identifies potential edge cases and failure scenarios in the
pool controller system that could lead to errors or unexpected behavior. Each
issue is rated by severity and includes recommendations for mitigation.

**Severity Levels**:

- 🔴 **Critical**: Could cause system failure or safety issues
- 🟡 **High**: Could cause incorrect operation or data loss
- 🟠 **Medium**: Could cause temporary malfunction or degraded performance
- 🔵 **Low**: Minor issues or edge cases with minimal impact

**Fix Status**:

- ✅ **FIXED in v3.1.0**: Issue has been addressed in this release
- ⚠️ **OPEN**: Issue remains as a recommendation for future work

---

## 1. Temperature Sensor Edge Cases

### 1.1 Sensor Disconnection During Operation 🔴 Critical ✅ **FIXED in v3.1.0**

**Location**: `src/DallasTemperatureNode.cpp`, `src/RuleAuto.cpp`

**Pre-v3.1.0 Issue**: When a Dallas temperature sensor returned
`DEVICE_DISCONNECTED_C` (-127°C), the error was logged but the old
`_temperature` value remained in memory.

**Pre-v3.1.0 Impact**:

- Rules continued using stale temperature data
- Auto mode could make incorrect heating decisions based on old readings
- No automatic recovery mechanism triggered relay safety shutdown

**Scenario** (pre-v3.1.0):

```text
1. Pool temperature sensor reads 25°C
2. Sensor wire becomes disconnected
3. System detects error and logs it
4. Auto rule still sees 25°C and continues operating pumps
5. Actual pool temperature could be 35°C, causing overheating
```

**v3.1.0 Solution**:

1. ✅ Temperature set to NaN on `DEVICE_DISCONNECTED_C` error
2. ✅ Auto rule validates temperatures with `isnan()` before decisions
3. ✅ Solar pump automatically disabled on invalid sensor readings
4. ✅ Clear warning messages when sensors disconnected

### 1.2 No Sensors Found at Startup 🟡 High ✅ **FIXED in v3.1.0**

**Location**: `src/DallasTemperatureNode.cpp`

**Pre-v3.1.0 Issue**: If no sensors were detected at startup, the node set
error state but continued polling indefinitely with `numberOfDevices = 0`.

**v3.1.0 Solution**:

1. ✅ `_temperature` initialized to NaN sentinel value
2. ✅ Enhanced warning messages for missing sensors
3. ✅ Auto mode validates temperature before use
4. Implement exponential backoff for sensor re-detection
5. Add visual/MQTT alert for missing sensors

### 1.3 Temperature Sensor Array Iteration 🟠 Medium

**Location**: `src/DallasTemperatureNode.cpp:94-123`

**Issue**: The loop iterates through `numberOfDevices` but only uses the last
sensor's reading. If multiple sensors are on one pin, only the last one's
value is stored.

**Current behavior**:

```cpp
for (uint8_t i = 0; i < numberOfDevices; i++) {
  _temperature = sensor.getTempC(tempDeviceAddress);  // Overwrites!
}
```

**Impact**:

- Multiple sensors on one pin are not supported correctly
- Only last sensor reading is used
- Could cause confusion in multi-sensor setups

**Recommendations**:

1. Document that only one sensor per pin is supported
2. Or: Store array of temperatures and average them
3. Or: Use first valid sensor reading and skip others

---

## 2. Time and Timer Edge Cases

### 2.1 NTP Time Sync Failure 🔴 Critical ⚠️ **OPEN**

**Location**: `src/TimeClientHelper.cpp:94-99`

**Issue**: If NTP update fails, `getUtcTime()` returns `0`, which represents
Unix epoch (1970-01-01 00:00:00).

**Impact**:

- Timer mode uses epoch time for scheduling decisions
- Pool pump might run at wrong times or not at all
- State persistence timestamps become invalid
- `mktime()` calculations in `RuleAuto.cpp:80` could overflow or produce
  incorrect results

**Scenario**:

```text
1. WiFi connection drops during operation
2. NTP client can't update time
3. getUtcTime() returns 0
4. Timer calculations use 1970 as "current time"
5. Pool pump never activates (timer thinks it's 1970, before start time)
```

**Recommendations** (for future):

1. Cache last valid time and use RTC or millis() to maintain approximate time
2. Detect time sync failures and disable timer mode automatically
3. Add time validity check: reject times before 2020-01-01
4. Alert user via MQTT when time sync fails
5. Consider ESP32 RTC or external RTC module for backup

### 2.2 Timer Midnight Crossing Edge Case 🟡 High ✅ **FIXED in v3.1.0**

**Location**: `src/RuleAuto.cpp`

**Pre-v3.1.0 Issue**: Timer logic in `checkPoolPumpTimer()` didn't handle the
case where the timer crosses midnight (e.g., start 22:00, end 02:00).

**Pre-v3.1.0 logic**:

```cpp
if (difftime(mktime(&time), mktime(&startTime)) >= 0 &&
    difftime(mktime(&time), mktime(&endTime)) <= 0)
```

**Problem**: If `startTime > endTime` (crossing midnight), the condition
`time >= start AND time <= end` is never true.

**Impact**:

- Overnight timer settings don't work
- Pool pump never runs for timers crossing midnight
- Silent failure - no error reported

**Example**:

```text
Timer: 22:00 - 02:00
Current: 23:00
Check: 23:00 >= 22:00 ✓ AND 23:00 <= 02:00 ✗ = FALSE
Expected: TRUE (pump should be running)
```

**Recommendations**:

1. Implement midnight-aware timer logic:

```cpp
bool isInRange = (startTime <= endTime)
    ? (time >= startTime && time <= endTime)  // Normal case
    : (time >= startTime || time <= endTime); // Crosses midnight
```

2. Add validation to prevent user from setting invalid timer ranges
3. Add unit tests for midnight crossing scenarios

### 2.3 Timezone Index Out of Bounds 🟠 Medium

**Location**: `src/OperationModeNode.cpp:258-268`,
`src/TimeClientHelper.cpp:102-108`

**Issue**: While there is bounds checking in `setTimezoneIndex()`, the
validation happens after the user sets the value via MQTT. The
`_selectedTimezoneIndex` could theoretically be corrupted.

**Impact**:

- Array out-of-bounds access if index is corrupted in memory
- Potential crash or undefined behavior
- TimeZone pointer could be invalid

**Recommendations**:

1. Add defensive bounds checking in `getTimeFor()` and `getTimeInfoFor()`
2. Validate loaded timezone index from persistent storage
3. Add range check: `index = constrain(index, 0, getTzCount()-1)`

---

## 3. Memory Management Edge Cases

### 3.1 SystemMonitor Overflow Protection 🟡 High

**Location**: `src/SystemMonitor.hpp:72-78`

**Issue**: The memory check uses unsigned arithmetic that could overflow:

```cpp
if (now - lastMemoryCheck < 10000) {
  return;
}
```

**Problem**: If `millis()` wraps around at ~49.7 days and `now <
lastMemoryCheck`, the subtraction underflows, producing a very large number,
causing memory check to be skipped for potentially hours.

**Impact**:

- Memory monitoring disabled during overflow period
- Critical memory situation could go undetected
- System might crash before next valid check

**Actual Risk**: **LOW** - The `Utils::shouldMeasure()` function handles
this correctly, but `SystemMonitor` doesn't use it.

**Recommendations**:

1. Replace with `Utils::shouldMeasure(lastMemoryCheck, 10)` for overflow
   safety
2. Or use the same pattern: `(now - lastMemoryCheck) >= interval` which
   works correctly with unsigned overflow

### 3.2 NTPClient Memory Leak on Reconfiguration 🟠 Medium

**Location**: `src/TimeClientHelper.cpp:75-80`

**Issue**: When reconfiguring NTP server, old `timeClient` is deleted and a
new one created. However, if `timeClientSetup()` is called multiple times,
this could fragment heap memory.

**Impact**:

- Repeated calls cause heap fragmentation
- Over time, could contribute to memory exhaustion
- Not critical but sub-optimal

**Recommendations**:

1. Only call `timeClientSetup()` once during initialization
2. Or: Add flag to prevent multiple calls
3. Or: Implement update method instead of delete/recreate

### 3.3 String Allocation in address2String() 🔵 Low

**Location**: `src/DallasTemperatureNode.cpp:145-158`

**Issue**: The function creates multiple String objects in a loop,
potentially causing heap fragmentation.

```cpp
for (uint8_t i = 0; i < 8; i++) {
  if (deviceAddress[i] < 16) {
    adr = adr + F("0");  // String concatenation
  }
  adr = adr + String(deviceAddress[i], HEX);  // More allocation
}
```

**Impact**:

- Multiple temporary String objects created
- Contributes to heap fragmentation
- Called during setup, so not a runtime issue

**Recommendations**:

1. Use char buffer and sprintf for zero-allocation implementation
2. Pre-reserve String capacity before loop
3. Low priority - only called during setup

---

## 4. Rule Execution Edge Cases

### 4.1 Null Rule Pointer 🟡 High ✅ **FIXED in v3.1.0**

**Location**: `src/OperationModeNode.cpp`

**Pre-v3.1.0 Issue**: If no rule matched the current mode, `getRule()`
returned `nullptr`, which was checked before calling `rule->loop()`. However,
there was no error handling or fallback.

**v3.1.0 Solution**:

1. ✅ System switches to manual mode when no rule matches
2. ✅ Error state published via MQTT for user notification
3. ✅ State persisted to prevent repeated failures

### 4.2 Temperature Comparison with Invalid Values 🔴 Critical ✅ **FIXED in v3.1.0**

**Location**: `src/RuleAuto.cpp`

**Pre-v3.1.0 Issue**: Auto rule compared temperatures without validating they
were valid readings.

```cpp
if (getSolarTemperature() < (getSolarMinTemperature() - hyst)) {
  // This could be comparing -127°C (disconnected sensor) with valid value
}
```

**Impact**:

- Disconnected sensor (-127°C) always triggers "too cold" condition
- Pool pumps shut down even when heating is needed
- Incorrect decisions based on invalid data

**Recommendations**:

1. Add temperature validity check before rule execution:

```cpp
bool isValidTemp(float temp) {
  return temp > -50.0 && temp < 150.0;
}
```

2. Disable auto mode if any critical sensor is invalid
3. Use last known good value with age check
4. Enter safe manual mode on invalid sensor data

### 4.3 Hysteresis Value Edge Cases 🟠 Medium

**Location**: `src/RuleAuto.cpp:20-34`

**Issue**: No validation of hysteresis value. User could set negative or
extremely large values via MQTT.

**Impact**:

- Negative hysteresis inverts logic
- Large hysteresis (e.g., 100K) prevents switching
- Zero hysteresis causes rapid on/off cycling

**Recommendations**:

1. Add validation in `applyProperty()`:

```cpp
if (property.equalsIgnoreCase(cHysteresis)) {
  float newHyst = value.toFloat();
  if (newHyst >= 0.1 && newHyst <= 10.0) {
    _hysteresis = newHyst;
  } else {
    // Reject invalid value
  }
}
```

2. Enforce limits: 0.1K to 10K reasonable range

---

## 5. Relay Control Edge Cases

### 5.1 ESP8266 State Persistence 🟡 High ✅ **FIXED in v3.1.0**

**Location**: `src/StateManager.hpp`, `src/RelayModuleNode.cpp`

**Pre-v3.1.0 Issue**: Relay state persistence was only implemented for ESP32.
ESP8266 lost relay state on reboot.

**v3.1.0 Solution**:

1. ✅ EEPROM-based persistence implemented for ESP8266
2. ✅ Hash-based key mapping with DJB2 algorithm
3. ✅ Lazy initialization ensures EEPROM ready before use
4. ✅ Data region cleared on first boot to prevent garbage reads
5. ✅ EEPROM wear reduced - only writes on actual state changes
6. ✅ Relay states persist across reboots on both platforms

### 5.2 Relay Pointer Initialization 🟠 Medium

**Location**: `src/RelayModuleNode.hpp:58`,
`src/RelayModuleNode.cpp:120`

**Issue**: `relay` pointer is initialized to `NULL` and only set in
`setup()`. If `setSwitch()` or `getSwitch()` is called before `setup()`, it
will crash.

**Impact**:

- Null pointer dereference if methods called before setup
- System crash

**Actual Risk**: **LOW** - Homie framework ensures setup() called first

**Recommendations**:

1. Add defensive null checks:

```cpp
void RelayModuleNode::setSwitch(const boolean state) {
  if (relay == nullptr) {
    return;  // Or log error
  }
  // ... rest of code
}
```

2. Or: Initialize relay in constructor instead of setup()

### 5.3 Duplicate setRunLoopDisconnected() Call 🔵 Low

**Location**: `src/RelayModuleNode.cpp:19-21`,
`src/OperationModeNode.cpp:18-20`

**Issue**: Both files call `setRunLoopDisconnected(true)` twice in
constructor.

```cpp
setRunLoopDisconnected(true);
setRunLoopDisconnected(true);  // Duplicate line
```

**Impact**: None - harmless but indicates possible copy-paste error

**Recommendations**: Remove duplicate lines (cleanup/code quality)

---

## 6. WiFi and MQTT Edge Cases

### 6.1 MQTT Operations While Disconnected 🟠 Medium

**Location**: Multiple files - all nodes check `Homie.isConnected()` before
publishing

**Issue**: While there are connection checks, if connection drops during
message publication, the publish could fail silently.

**Impact**:

- State updates lost
- User sees stale data in Home Assistant/openHAB
- No retry mechanism

**Recommendations**:

1. Check return value of `setProperty().send()`
2. Implement message queue for critical state updates
3. Add "last successful publish" timestamp
4. Re-publish on reconnection

### 6.2 WiFi Connection Loss During NTP Sync 🟠 Medium

**Location**: `src/TimeClientHelper.cpp:94-99`

**Issue**: If WiFi drops during NTP update, function returns 0 without
retry.

**Impact**: Time becomes invalid until next sync (1 hour later)

**Recommendations**:

1. Reduce sync interval when time is invalid (e.g., retry every 5 minutes)
2. Keep track of last successful sync time
3. Alert user if time hasn't synced in 24 hours

---

## 7. Numeric Conversion Edge Cases

### 7.1 Float to String Buffer Overflow Protection 🔵 Low

**Location**: `src/Utils.hpp:41-48`

**Issue**: The `floatToString()` function has buffer size check but doesn't
validate the converted string length.

**Current protection**:

```cpp
if (bufferSize < 8) {
  buffer[0] = '\0';
  return;
}
dtostrf(value, 0, decimals, buffer);  // Could still overflow!
```

**Problem**: `dtostrf()` could write beyond buffer if value is extremely
large (e.g., 1e38 with 2 decimals).

**Impact**:

- Buffer overflow possible with extreme values
- Stack corruption risk

**Actual Risk**: **VERY LOW** - temperature values are constrained

**Recommendations**:

1. Use `snprintf()` instead of `dtostrf()` for guaranteed bounds
2. Or: Add value range check before conversion
3. Document buffer size requirements more explicitly

### 7.2 String.toFloat() and String.toInt() Error Handling 🟠 Medium

**Location**: `src/OperationModeNode.cpp:216, 221, 226`, multiple locations

**Issue**: When parsing user input from MQTT, no validation that conversion
succeeded.

```cpp
_hysteresis = value.toFloat();  // What if value is "abc"?
```

**Impact**:

- Invalid input results in 0.0 or 0
- User might accidentally set critical values to zero
- No error feedback

**Recommendations**:

1. Validate input format before conversion
2. Check for zero and reject if unexpected
3. Add min/max validation for all user inputs
4. Return false and log error for invalid conversions

---

## 8. Timer-Specific Edge Cases

### 8.1 Timer Setting Persistence Race Condition 🔵 Low

**Location**: `src/OperationModeNode.cpp:229-255`

**Issue**: Timer settings are read, modified, and written back in separate
operations. Not atomic.

```cpp
TimerSetting timerSetting = getTimerSetting();  // Read
timerSetting.timerStartHour = value.toInt();    // Modify
setTimerSetting(timerSetting);                  // Write
```

**Impact**:

- If multiple MQTT messages arrive rapidly, some updates could be lost
- Non-critical: unlikely in real-world use

**Recommendations**:

1. Add mutex/lock if implementing concurrent MQTT handling
2. Or: Batch timer updates and persist once
3. Low priority - current approach acceptable for single-threaded operation

---

## 9. State Persistence Edge Cases

### 9.1 Preferences Begin/End Balance 🔵 Low

**Location**: `src/StateManager.hpp`, multiple functions

**Issue**: Each state operation opens and closes preferences separately.
This is inefficient but safe.

**Impact**:

- Multiple flash writes during state save
- Slower than batch operations
- Increases wear on flash memory

**Recommendations**:

1. Implement batch state save/load:

```cpp
static void beginSave() { prefs.begin("pool-controller", false); }
static void endSave() { prefs.end(); }
```

2. Call once for multiple saves
3. Low priority - flash wear is not critical concern

### 9.2 State Load Failure Handling 🟠 Medium

**Location**: `src/OperationModeNode.cpp:287-306`

**Issue**: No error handling if state load fails. Uses defaults silently.

**Impact**:

- User might not know settings were reset
- Could cause unexpected behavior after flash corruption

**Recommendations**:

1. Add checksum or validation to detect corrupted state
2. Log warning when using defaults vs loaded state
3. Publish MQTT message indicating state was reset

---

## 10. Platform-Specific Edge Cases

### 10.1 ESP8266 Watchdog Timer Limitations 🟠 Medium

**Location**: `src/SystemMonitor.hpp:50-54`

**Issue**: ESP8266 uses software watchdog via `yield()`, which only works if
code calls it regularly. Long-running operations can still cause WDT reset.

**Impact**:

- Less reliable than ESP32 hardware watchdog
- Could still experience unexpected reboots
- No timeout configuration

**Recommendations**:

1. Document ESP8266 watchdog limitations
2. Ensure `yield()` called in all loops
3. Add `yield()` in long MQTT publishing operations
4. Consider recommending ESP32 for production use

### 10.2 ESP32-Specific Features Not Available on ESP8266 🔵 Low

**Location**: Multiple - state persistence, hardware watchdog, etc.

**Issue**: Feature parity between platforms is incomplete. ESP8266 users
have degraded experience.

**Impact**: User confusion, platform-specific bugs

**Recommendations**:

1. Document feature matrix clearly
2. Add compile-time warnings for missing features
3. Consider dropping ESP8266 support or implementing missing features

---

## 11. Concurrency and Threading Edge Cases

### 11.1 No Thread Safety for Shared State 🟠 Medium

**Location**: All shared variables (temperatures, relay states, etc.)

**Issue**: ESP32 supports multithreading, but no mutex protection on shared
state.

**Impact**:

- If future code uses multiple tasks, race conditions possible
- Currently single-threaded, so not a problem yet

**Recommendations**:

1. Document that code is single-threaded only
2. Add mutex protection if multi-threading is added
3. Use atomic operations for critical flags

---

## 12. Configuration and Validation Edge Cases

### 12.1 Measurement Interval Validation 🔵 Low

**Location**: Multiple nodes, e.g., `src/DallasTemperatureNode.cpp:29`

**Issue**: Measurement interval has minimum check but no maximum:

```cpp
_measurementInterval = (measurementInterval > MIN_INTERVAL)
    ? measurementInterval
    : MIN_INTERVAL;
```

**Impact**:

- User could set extremely large interval (e.g., 1000000 seconds)
- Temperature updates would be very rare
- System appears to hang

**Recommendations**:

1. Add maximum interval check (e.g., 3600 seconds)
2. Validate during initialization
3. Document acceptable range

### 12.2 Pin Configuration Conflicts 🟡 High ✅ **FIXED in v3.1.0**

**Location**: `src/PoolController.cpp`

**Pre-v3.1.0 Issue**: No validation that pin numbers didn't conflict between
nodes.

**v3.1.0 Solution**:

1. ✅ Pin conflict detection implemented at startup
2. ✅ System halts with clear error message on conflicts
3. ✅ Pin usage map displayed on successful validation

---

## Summary and Priority Recommendations

### ✅ Fixed in v3.1.0

1. **Sensor disconnection handling** (1.1) - Temperature set to NaN,
   validation added
2. **No sensors found** (1.2) - Better initialization and warnings
3. **Midnight crossing timers** (2.2) - Midnight-aware logic implemented
4. **Invalid temperature comparisons** (4.2) - Validation with isnan() added
5. **Null rule pointer** (4.1) - Fallback to manual mode implemented
6. **ESP8266 state persistence** (5.1) - EEPROM persistence with improved hash
7. **Pin configuration conflicts** (12.2) - Validation at startup added

### 🔴 Critical Priority (Remaining)

1. **NTP time sync failure** (2.1) - Cache time and detect sync failures

### 🟡 High Priority (Remaining)

### 🟠 Medium Priority (Plan for Future)

1. **Timezone bounds** (2.3) - Add defensive checks
2. **Memory check overflow** (3.1) - Use consistent overflow-safe pattern
3. **Input validation** (7.2) - Validate all MQTT inputs
4. **State load failures** (9.2) - Detect and report

### 🔵 Low Priority (Nice to Have)

1. **Code cleanup** - Remove duplicate lines
2. **Documentation** - Document limitations and edge cases
3. **Optimization** - Reduce String allocations, batch state operations

---

## Testing Recommendations

To validate fixes for these edge cases, implement tests for:

1. **Sensor failure scenarios**:

   - Disconnect sensor during operation
   - No sensors at startup
   - Intermittent sensor connection

2. **Time and timer scenarios**:

   - WiFi loss during operation
   - NTP sync failures
   - Midnight crossing timers
   - DST transitions

3. **Memory stress tests**:

   - Run for >50 days (millis overflow)
   - Low memory conditions
   - Rapid MQTT message floods

4. **Invalid input tests**:

   - Out-of-range values
   - Invalid string formats
   - Malformed MQTT messages

5. **Platform-specific tests**:
   - Test on both ESP32 and ESP8266
   - Verify state persistence
   - Power failure recovery

---

## Conclusion

The pool controller is well-designed with many reliability features already
in place (memory monitoring, watchdog, overflow protection). However, several
edge cases remain that could cause issues in production environments.

**Key Strengths**:

- Millis overflow handled correctly in `Utils.hpp`
- Memory monitoring and auto-reboot
- Hardware watchdog on ESP32
- State persistence on ESP32

**Key Weaknesses**:

- Insufficient sensor error handling
- Time sync failure not handled
- Timer midnight crossing broken
- ESP8266 feature parity incomplete
- Input validation missing

Addressing the **Critical** and **High** priority items will significantly
improve system reliability and user experience.
