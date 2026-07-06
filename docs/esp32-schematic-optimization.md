---
title: ESP32 Schematic Optimization
summary: ESP32 schematic optimization for sensors (DS18B20, DHT22) and relay control — GPIO analysis, pull-up resistors, noise mitigation, and power supply
date: "2026-06-11"
lastmod: "2026-06-11"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "esp32", "hardware", "sensors", "relays"]
menu:
  docs:
    parent: Pool Controller
    name: ESP32 Schematic Optimization
    weight: 25
---

This document analyzes the existing setup (Fritzing source + existing hardware
documentation) and describes an optimized ESP32 proposal for sensors and relays.

## 1) Current Analysis (Relevant Signals)

Current firmware pin assignment for ESP32 (`src/Config.hpp`):

- `PIN_DS_SOLAR` = GPIO32
- `PIN_DS_POOL` = GPIO33
- `PIN_RELAY_POOL` = GPIO25
- `PIN_RELAY_SOLAR` = GPIO26

Hardware used per documentation:

- 2x DS18B20 Temperature Sensors
- 2-Channel Relay Module (5V)

## 2) Main Issues in the Previous Setup (Before Optimization)

1. **DS18B20 was on GPIO15 (Strapping Pin)**

   GPIO15 is a boot-strapping pin on the ESP32.
   A OneWire bus with pull-up on this pin can negatively affect
   boot behavior.

2. **Relay/Power Supply Coupling**

   5V relay modules can cause switching interference (spikes/noise).
   Without clean separation of logic and load supply, the risk of
   resets and measurement noise increases.

3. **Missing Explicit Fail-Safe Definition in Schematic**

   On startup or reset, no pump should turn on unintentionally.

## 3) Optimized ESP32 Proposal

### 3.1 Recommended Pin Assignment

| Function      | Before (ESP32) | Now (ESP32) | Reason                           |
| ------------- | -------------- | ----------- | -------------------------------- |
| DS18B20 Solar | GPIO15         | GPIO32      | Avoids strapping pins            |
| DS18B20 Pool  | GPIO16         | GPIO33      | Clean separation of sensor lines |
| Relay Pool    | GPIO18         | GPIO25      | Robust output, low conflict risk |
| Relay Solar   | GPIO19         | GPIO26      | Robust output, low conflict risk |

Note: For existing installations with old wiring, adjustment of the
firmware pins and possibly rewiring is required.

### 3.1.1 Decision Documentation (Clear Decision)

**Decision:** For ESP32, the signals are permanently assigned to
`GPIO32/33` (DS18B20) and `GPIO25/26` (Relays).

**Why this specific assignment:**

1. **Boot robustness:** Sensors are no longer on strapping pins
   (in particular no OneWire on GPIO15).
2. **Low noise:** Sensor GPIOs are logically separated from relay GPIOs,
   reducing mutual interference during switching operations.
3. **Operational safety:** Relays on well-usable output pins with clear
   fail-safe behavior on startup/reset.
4. **Maintainability:** Consistent, documented standard assignment in
   firmware and hardware documentation reduces wiring errors and
   support effort.

**Not chosen:** The earlier ESP32 assignment (`15/16/18/19`) because it
carries higher boot/noise risk.

### 3.2 Sensor Optimization (DS18B20)

- Provide a clean **4.7k pull-up to 3.3V** for each sensor line.
- Route sensor cables spatially separate from relay/230V lines.
- For longer cables: use twisted pair and a shared clean GND.
- Provide 100nF decoupling near the ESP32 power supply.

### 3.3 Relay Optimization

- Use a **relay module with optocoupler** and clearly documented logic level
  direction (active-low/active-high).
- If the module supports it: **separate JD-VCC (relay coil) from VCC (logic)**
  and use star-point grounding.
- Plan appropriate mains protection measures on the pump side
  (e.g., appropriate fusing/protection circuit for the installation).
- Design relay inputs with additional pull-up/pull-down fail-safe to
  prevent unintended switching during boot.

### 3.4 Wiring (Visualization)

```text
                          +---------------------+
                          |        ESP32        |
                          |                     |
DS18B20 Solar DATA  <---->| GPIO32              |
DS18B20 Pool  DATA  <---->| GPIO33              |
Relay IN1 (Pool)    <---->| GPIO25              |
Relay IN2 (Solar)   <---->| GPIO26              |
                          |                     |
3V3 --------------------->| 3V3                 |
GND --------------------->| GND                 |
                          +----------+----------+
                                    |
                                    | common ground
                                    v
                    +----------------+----------------+
                    | 2-Channel Relay Module (5V, opto) |
                    | VCC (Logic)  <- 3V3/5V*         |
                    | JD-VCC (Coil)<- 5V              |
                    | GND           <- GND            |
                    | IN1           <- GPIO25         |
                    | IN2           <- GPIO26         |
                    +----------------+----------------+

DS18B20 Solar: VDD -> 3V3, GND -> GND, DATA -> GPIO32, Pull-up 4.7k to 3V3
DS18B20 Pool : VDD -> 3V3, GND -> GND, DATA -> GPIO33, Pull-up 4.7k to 3V3

* depends on the relay module used (check logic level)
```

## 4) Firmware Reference for Rewiring

The recommended pins are implemented in `src/Config.hpp`. Relevant pins:

- `PIN_DS_SOLAR`
- `PIN_DS_POOL`
- `PIN_RELAY_POOL`
- `PIN_RELAY_SOLAR`

## 5) Verification Checklist After Conversion

- Does the ESP32 boot reproducibly (cold/warm) without errors?
- Do relays **not** switch unintentionally during boot?
- Are temperature measurements stable, even during relay switching?
- No resets during parallel WiFi + relay operation?

## 6) Operation and Safety

For productive 24/7 operation and safety aspects, please also refer to
the project's security and reliability documentation.

## 7) See Also

- Detailed assembly guide with step-by-step instructions and
  manufacturing tips: [Hardware Guide](hardware-guide.md)
- Complete wiring diagram with RTC module:
  [ESP32 Complete Wiring Schematic](esp32-complete-wiring-schematic.md)
