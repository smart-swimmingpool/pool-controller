---
title: Safety Model
summary: System safety architecture of the Pool Controller — multi-layer safety approach covering hardware failsafes, firmware safeguards, software guards, and operational procedures
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "safety", "architecture", "reliability"]
menu:
  docs:
    parent: Pool Controller
    name: Safety Model
    weight: 140
---

> ⚠️ **NOTE**: This document describes the safety architecture of the
> Pool Controller. For practical electrical safety instructions when
> building the controller, see [Electrical Safety](electrical-safety.md).

## Overview

The Pool Controller is designed with a **defense-in-depth** safety model.
Safety is not a single feature but a multi-layer architecture spanning
hardware, firmware, and operational procedures.

```text
┌──────────────────────────────────────────────┐
│              Operational Layer                │
│  (User procedures, checklists, maintenance)  │
├──────────────────────────────────────────────┤
│               Software Layer                  │
│  (Watchdog, boot-loop detection, Safe Mode)  │
├──────────────────────────────────────────────┤
│              Firmware Layer                   │
│  (Config validation, type safety, NVS CRC)   │
├──────────────────────────────────────────────┤
│              Hardware Layer                   │
│  (Relay isolation, fuses, enclosure IP65)    │
└──────────────────────────────────────────────┘
```

---

## Layer 1: Hardware Safety

### Relay Isolation

- Relays provide **galvanic isolation** between the ESP32 (3.3V/5V) and
  mains voltage (230V AC)
- Minimum 4 mm creepage distance between coil and contacts on quality modules
- Relay modules with optocoupler input provide additional isolation

### Overcurrent Protection

- A **circuit breaker** must be installed on the mains Live conductor
- Rated for the pump load (see [Electrical Safety](/docs/electrical-safety/))
- Protects against short circuits and overloads

### Enclosure

- IP65-rated enclosure protects against water jets and dust
- Cable glands provide strain relief and maintain IP rating
- Physical separation of mains and low-voltage wiring inside enclosure

### Failsafe Relay Behavior

- Relays are **normally open (NO)** — if the ESP32 loses power, the relays
  disengage and pumps stop
- This prevents pumps from running uncontrollably on controller failure

---

## Layer 2: Firmware Safety

### Configuration Validation

All configuration values are validated before being applied:

| Parameter              | Validation            | Range             |
| ---------------------- | --------------------- | ----------------- |
| Temperature thresholds | Min/max bounds        | 0–60 °C           |
| Timer values           | 24h format check      | 00:00–23:59       |
| WiFi settings          | SSID length check     | 1–32 chars        |
| MQTT settings          | Hostname format check | Valid hostname/IP |
| Relay configuration    | Enum check            | `true` / `false`  |

Invalid configurations are **rejected** and the previous value is preserved.

### NVS CRC Protection

Configuration stored in ESP32 NVS includes a CRC32 checksum. If the
checksum doesn't match on boot, the configuration is reset to factory defaults.

### Type Safety

The firmware uses strongly-typed enums and structs instead of generic integers
for mode selection, reducing the risk of invalid states.

### Task Monitoring

Each FreeRTOS task monitors its own stack usage. If a task exceeds the
configured stack watermark, a warning is logged and the system can take
corrective action.

---

## Layer 3: Software Safety

### Hardware Watchdog Timer (WDT)

- ESP32 hardware WDT with 30-second timeout
- Reset if the main loop stalls for more than 30 seconds
- Automatically re-enabled after boot
- Ensures the controller cannot hang indefinitely

### Boot-Loop Detection

- NVS-based boot counter increments on each boot
- Counter resets after a successful 5-minute run
- **4 consecutive short boots** trigger Safe Mode
- In Safe Mode:
  - All relays are forced OFF
  - Web UI remains accessible
  - Serial log shows Safe Mode indicator
  - Configuration can be inspected and corrected

### Memory Monitoring

- Free heap is checked every 10 seconds
- At **critical threshold** (8 KB free heap):
  - Warning logged
  - Graceful auto-reboot initiated
- At **warning threshold** (15 KB free heap):
  - Warning logged
  - No reboot — system continues monitoring

### Sensor Auto-Recovery

- DS18B20 read failure triggers fast re-polling (5s instead of 300s)
- After 3 consecutive successful reads, back to normal interval
- Prevents unnecessary alerts from transient sensor glitches

### NTP Graceful Degradation

Three-stage fallback for time synchronization:

1. **Primary**: NTP server responds → normal operation
2. **Degraded**: NTP fails → uses last known good time
3. **Safe**: No time available → uses millis() uptime with warning

---

## Layer 4: Operational Safety

### Commissioning Procedures

- **Breadboard test** before mains connection (see [Build from Zero](/docs/build-from-zero/))
- **Relay no-load test** before connecting pumps
- **First power-on** with mains: 30-minute supervised operation

### Maintenance Schedule

- Monthly: verify temperatures, check warnings
- Annually: inspect wiring, test RCD, check enclosure seals

### Checklists

- [Production Checklist](/docs/production-checklist/) — pre-deployment
- [Security Checklist](/docs/security-checklist/) — security hardening

---

## Failure Mode Analysis

| Failure Mode      | Effect                 | Safety Layer    | Mitigation                   |
| ----------------- | ---------------------- | --------------- | ---------------------------- |
| ESP32 crash       | Relays off, pumps stop | Hardware        | Failsafe relay behavior (NO) |
| Software hang     | No relay updates       | WDT             | 30s reboot                   |
| Config corruption | Invalid settings       | Firmware        | NVS CRC → factory reset      |
| Repeated crashes  | Unsafe state           | Boot-loop       | Safe Mode → relays OFF       |
| Heap exhaustion   | Unpredictable          | Memory monitor  | Graceful reboot              |
| Sensor failure    | No temperature data    | Sensor recovery | Fast re-polling              |
| Power loss        | Controller off         | Hardware        | Relays default OFF           |
| Network loss      | No remote control      | Software        | Local web UI still works     |

---

## Related Documents

- [Electrical Safety](/docs/electrical-safety/) — Electrical safety information
- [Production Checklist](/docs/production-checklist/) — Pre-deployment checks
- [Security Checklist](/docs/security-checklist/) — Security hardening
- [Build from Zero](/docs/build-from-zero/) — Complete build guide
