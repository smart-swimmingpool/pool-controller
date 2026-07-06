---
name: doxygen
description: Inline documentation standard for the pool-controller C++ codebase using Doxygen. Use when asked to add, review, or fix inline code documentation. 🇩🇪 Deutsche Trigger: Inline-Dokumentation, Doxygen-Kommentare, Code-Dokumentation ergänzen, Quellcode dokumentieren.
---

# Doxygen Documentation Standard

This project uses **Doxygen** (`/** ... */`) for all inline code documentation.
PlatformIO/Arduino ecosystem standard — also used by ESP-IDF and most embedded
C++ libraries.

## Scope

- Every **header file** (`*.hpp`) must have:
  - A **file comment** with `@file` and `@brief`
  - A **class/struct comment** with `@brief`
  - **Public method comments** with `@brief`, `@param`, `@return` where applicable
- Every **source file** (`*.cpp`) must have a **file comment** with `@file`
  and `@brief`
- **Private methods** may have lighter comments; use a single-line `/** ... */`
  for trivial getters/setters
- **Constants, macros, enums** get a `/** @brief ... */` comment

## Style

### File Header

```cpp
/**
 * @file Config.hpp
 * @brief Pin assignments and compile-time constants for the Pool Controller.
 *
 * All GPIO pin assignments and tunable constants are centralized here.
 * Changing these values requires rebuilding the firmware.
 */
```

### Class/Struct

```cpp
/**
 * @brief Manages the pool pump relay state.
 *
 * Wraps an Arduino digital output pin as a relay switch.
 * Active-high logic: GPIO HIGH → relay ON.
 */
class RelayModuleNode final {
```

### Public Method (full)

```cpp
/**
 * @brief Attempts to parse a time string into hours and minutes.
 * @param timeStr  Input string in "HH:MM" format.
 * @param outHours Output parameter for parsed hours (0–23).
 * @param outMins  Output parameter for parsed minutes (0–59).
 * @return true if parsing succeeded, false on invalid format.
 * @note The input string must be at least 5 characters ("HH:MM").
 */
auto parseTime(const char* timeStr, uint8_t& outHours, uint8_t& outMins) -> bool;
```

### Simple Getter/Setter (single line)

```cpp
/** @brief Get current free heap in bytes. */
static auto getFreeHeap() -> uint32_t;
```

### Free Function (namespace scope)

```cpp
/**
 * @brief Check if enough time has elapsed since the last measurement.
 * @param lastMeasurement  Timestamp (ms) of the last measurement.
 * @param intervalSeconds  Desired interval in seconds.
 * @return true if intervalSeconds have elapsed since lastMeasurement.
 */
auto isMeasurementDue(uint32_t lastMeasurement, uint32_t intervalSeconds) -> bool;
```

### Constants / Macros

```cpp
/** @brief Interval for temperature sensor updates (seconds). */
static constexpr uint32_t TEMP_READ_INTERVAL{30};

/** @brief Default MQTT broker port. */
static constexpr uint16_t MQTT_DEFAULT_PORT{1883};
```

## Tags Used

| Tag        | When to use                                                |
| ---------- | ---------------------------------------------------------- |
| `@brief`   | **Required** on every documented symbol — one-line summary |
| `@param`   | Every function parameter, unless trivially obvious         |
| `@return`  | Every non-void return, unless trivially obvious            |
| `@note`    | Important caveats, edge cases, threadsafety                |
| `@see`     | Cross-reference to related functions/files                 |
| `@warning` | Dangerous behavior, side effects, 230V safety              |
| `@file`    | File header only — auto-populated from `\file`             |
| `@author`  | Optional — do NOT add; copyright already in file header    |

## PlatformIO Integration

Generate HTML docs locally:

```bash
# Install doxygen
sudo apt install doxygen graphviz  # Debian/Ubuntu

# Generate (uses Doxyfile if present, otherwise default settings)
doxygen

# Output: docs/html/index.html
```

For CI, a `Doxyfile` can be added at the project root. Currently the docs
are maintained for human readability in-IDE only (no CI Doxygen step).

## Verification Checklist

- [ ] Every `*.hpp` has a `@file` comment at the top
- [ ] Every `*.cpp` has a `@file` comment at the top
- [ ] Every class/struct has `@brief`
- [ ] Every public method with parameters has `@param` for each param
- [ ] Every non-void public method has `@return`
- [ ] Single-line getters use `/** @brief ... */` format
- [ ] Trivial destructors, deleted constructors do NOT need doc comments
