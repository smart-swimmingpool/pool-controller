---
title: ESP32 Complete Wiring Schematic
summary: Complete ESP32 wiring diagram for the Pool Controller — DS18B20 temperature sensors, relay modules, optional RTC DS3231, resistor calculation, and power supply
date: "2026-06-11"
lastmod: "2026-06-11"
draft: false
toc: true
type: docs
featured: false
tags: ["docs", "esp32", "hardware", "schematic", "wiring"]
menu:
  docs:
    parent: Pool Controller
    name: ESP32 Complete Wiring
    weight: 26
---

This document provides an additional, complete wiring diagram for the
ESP32 setup with sensors, relays, resistors, and clock module (RTC).

The standard pins used in the firmware are:

- `PIN_DS_SOLAR` = GPIO32
- `PIN_DS_POOL` = GPIO33
- `PIN_RELAY_POOL` = GPIO25
- `PIN_RELAY_SOLAR` = GPIO26

## 1) Components (incl. clock and resistors)

- 1x ESP32 Dev Board
- 2x DS18B20 Temperature Sensor (Solar, Pool)
- 2x Resistor 4.7kΩ (Pull-up for OneWire data lines)
- 1x 2-Channel Relay Module 5V (preferably with optocoupler)
- 1x RTC Module DS3231 (clock, optional)
- 1x Power Supply 5V DC (sufficiently rated)
- Wiring cables, terminals, optionally fuse/protection circuit on load side

## 2) Complete Schematic

### Sensors (DS18B20, 4.7kΩ pull-up between DATA and 3V3)

Each sensor has its own wire for DATA, VDD and GND. The 4.7kΩ resistor is the only link between DATA and 3V3.

```svgbob
 .---------------------.             .--------------------.
 |        ESP32        |             |   DS18B20 Solar    |
 |                     |             |                    |
 |           GPIO32 o--+----*--------+--o DATA            |
 |                     |    |        |                    |
 |                     | .--+--.     |                    |
 |                     | |4.7k |     |                    |
 |                     | '--+--'     |                    |
 |                     |    |        |                    |
 |              3V3 o--+----*--------+--o VDD             |
 |                     |             |                    |
 |              GND o--+-------------+--o GND             |
 '---------------------'             '--------------------'
```

```svgbob
 .---------------------.             .--------------------.
 |        ESP32        |             |    DS18B20 Pool    |
 |                     |             |                    |
 |           GPIO33 o--+----*--------+--o DATA            |
 |                     |    |        |                    |
 |                     | .--+--.     |                    |
 |                     | |4.7k |     |                    |
 |                     | '--+--'     |                    |
 |                     |    |        |                    |
 |              3V3 o--+----*--------+--o VDD             |
 |                     |             |                    |
 |              GND o--+-------------+--o GND             |
 '---------------------'             '--------------------'
```

### Relay module

The relay contacts switch the pump circuit (230V AC).
It is not part of this low-voltage drawing; see the [Electrical Safety](/docs/electrical-safety/) guide.

```svgbob
 .---------------------.        .--------------------.
 |        ESP32        |        |    Relay module    |
 |                     |        |   2 channels, 5V   |
 |           GPIO25 o--+--------+--o IN1             |
 |           GPIO26 o--+--------+--o IN2             |
 |               5V o--+--------+--o VCC             |
 |              GND o--+--------+--o GND             |
 '---------------------'        '--------------------'
```

### RTC DS3231 (optional)

Supply the RTC with 3V3 or 5V, depending on the module type.

```svgbob
 .---------------------.        .--------------------.
 |        ESP32        |        |     RTC DS3231     |
 |                     |        |                    |
 |           GPIO21 o--+--------+--o SDA             |
 |           GPIO22 o--+--------+--o SCL             |
 |              3V3 o--+--------+--o VCC             |
 |              GND o--+--------+--o GND             |
 '---------------------'        '--------------------'
```

## 3) Wiring Table

| Component             | Signal | ESP32 Pin | Additional Component            |
| --------------------- | ------ | --------- | ------------------------------- |
| DS18B20 Solar         | DATA   | GPIO32    | 4.7kΩ pull-up to 3V3            |
| DS18B20 Pool          | DATA   | GPIO33    | 4.7kΩ pull-up to 3V3            |
| Relay Channel 1       | IN1    | GPIO25    | optional pull-down (fail-safe)  |
| Relay Channel 2       | IN2    | GPIO26    | optional pull-down (fail-safe)  |
| RTC DS3231 (optional) | SDA    | GPIO21    | I2C bus                         |
| RTC DS3231 (optional) | SCL    | GPIO22    | I2C bus                         |
| All components        | GND    | GND       | common ground routing           |
| DS18B20 / RTC         | Power  | 3V3       | clean decoupling                |
| Relay module          | Power  | 5V        | separate load/logic recommended |

## 4) Important Notes

1. 230V/mains side of pumps must be wired according to applicable safety regulations.
2. Route sensor cables separately from mains and relay cables.
3. Check active logic of the relay module (active-low or active-high).
4. For low-noise operation near the ESP32, provide additional decoupling
   (e.g., 100nF).
5. The firmware uses NTP by default; the RTC module serves as an additional
   hardware clock for extensions/offline scenarios.

## 5) References

- Detailed assembly guide (also for beginners):
  [Hardware Guide](hardware-guide.md) — step-by-step,
  manufacturing tips, troubleshooting
- Pin background and optimization:
  [ESP32 Schematic Optimization](esp32-schematic-optimization.md)
- [KiCad Schematic: ESP32 Dev Board](kicad/esp32-dev-board/esp32-dev-board-schematic.pdf) — KiCad 9.0 schematic PDF export
