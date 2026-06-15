---
title: Electrical Safety
summary: Electrical safety information for the Pool Controller — 230V AC mains wiring, relay isolation, fuse protection, enclosure requirements, and safe commissioning procedures
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "safety", "electrical", "hardware", "commissioning"]
menu:
  docs:
    parent: Pool Controller
    name: Electrical Safety
    weight: 130
---

> ⚠️ **WARNING: This document describes 230V AC mains voltage installation.**
> Improper wiring poses risk of electric shock, fire, or equipment damage.
>
> - If you are not experienced with mains-voltage wiring, hire a qualified
>   electrician
> - This guide is for informational purposes — local electrical codes may
>   require certified installation
> - This is a DIY project — not CE/UL certified for commercial use

## Overview

The Pool Controller switches **230V AC mains voltage** to pool and solar
pumps.
Improper wiring or installation poses risk of electric shock, fire, or equipment
damage. Read this document carefully before connecting mains power.

> **If you are not experienced with mains-voltage wiring, hire a qualified
> electrician.** This guide is for informational purposes — local electrical
> codes may require certified installation.

---

## Hazard Assessment

| Hazard           | Risk Level | Description                                          |
| ---------------- | ---------- | ---------------------------------------------------- |
| Electric shock   | High       | Direct contact with 230V AC can be fatal             |
| Fire             | Medium     | Overloaded circuits or loose connections can ignite  |
| Equipment damage | Medium     | Incorrect wiring can destroy the controller or pumps |
| Water ingress    | High       | Pool environment has high humidity and splash risk   |

---

## General Safety Rules

1. **Always disconnect power** before working on the controller or pumps
2. **Use a residual-current device (RCD/FI)** on the controller circuit
3. **Never work wet** — ensure dry hands and dry environment
4. **Use the correct fuse rating** for your pump load
5. **Keep mains and low-voltage wiring separated** inside the enclosure
6. **Secure all connections** with strain relief
7. **Label all cables** clearly

---

## Mains Wiring

### Relay Output Wiring

The relay module switches the **Live (phase)** conductor only. The Neutral
and Protective Earth connect directly to the pump.

```text
AC Mains        Relay Module         Pump
┌────────┐     ┌──────────────┐     ┌─────┐
│ Live   ──────┤ COM    NO    ──────┤ L   │
│ (230V) │     │              │     │     │
│ Neutr. ───────────────────────────┤ N   │
│        │     │              │     │     │
│ Earth  ───────────────────────────┤ PE  │
└────────┘     └──────────────┘     └─────┘
```

### Wire Sizing

| Pump Power  | Min. Wire Cross-Section | Fuse |
| ----------- | ----------------------- | ---- |
| ≤ 500 W     | 1.5 mm² (AWG 15)        | 6 A  |
| 500–1000 W  | 1.5 mm² (AWG 15)        | 10 A |
| 1000–2000 W | 2.5 mm² (AWG 13)        | 16 A |

### Fuse Requirements

- Install a circuit breaker appropriate for your pump load
- The fuse should be on the **Live** conductor before the controller
- Use a DIN-rail or panel-mount fuse holder

---

## Low-Voltage Wiring

### ESP32 and Sensor Wiring

- ESP32 operates at 3.3V / 5V (USB power)
- DS18B20 sensors use 3.3V — max cable length: 10 m
- Keep sensor cables away from mains cables (min. 5 cm separation)
- Use shielded twisted-pair cable for long sensor runs

### Power Supply

- Use a **regulated** 5V power supply (not a bare wall adapter)
- Minimum 1A for ESP32 + relay module
- For 12V pool systems, use a 12V→5V DC-DC converter

---

## Enclosure Requirements

### IP Rating

| Location                 | Minimum IP Rating | Requirements                       |
| ------------------------ | ----------------- | ---------------------------------- |
| Indoor (dry)             | IP54              | Dust protection, splash resistance |
| Outdoor (sheltered)      | IP65              | Water jets, dust-tight             |
| Outdoor (direct weather) | IP66+             | Powerful water jets, weatherproof  |

### Enclosure Best Practices

- Use **cable glands** (PG9 or PG11) for all cable entries
- Install a **drip loop** on external cables before entering the enclosure
- Keep mains and low-voltage cables on **opposite sides** of the enclosure
- Use **strain relief** on all cables
- Ensure the enclosure can be **locked** or secured against tampering

---

## Grounding

### Protective Earth (PE)

- Connect the enclosure to protective earth if it is metal
- Connect pump motor protective earth
- Use green/yellow wire (min. 2.5 mm²) for ground connections
- Verify ground continuity with a multimeter before commissioning

### Ground Loop Prevention

- Run all ground wires to a **single ground point** (star topology)
- Do NOT daisy-chain ground connections
- Use insulated crimp ring terminals on ground connections

---

## Commissioning

### Before Connecting Mains

1. Complete the [Build from Zero](/docs/build-from-zero/) breadboard test
2. Verify sensors return plausible readings
3. Test relay operation with a multimeter (no-load)
4. Confirm the enclosure is properly sealed

### First Power-On with Mains

1. **Do not leave the controller unattended** for the first 30 minutes
2. Listen for unusual sounds (buzzing, arcing)
3. Check for warmth on relay terminals and power supply
4. Verify pump starts and stops correctly
5. Verify relay disengages when toggled OFF

### Annual Inspection

- [ ] Check all screw terminals for tightness
- [ ] Inspect cables for damage or wear
- [ ] Test RCD/FI switch
- [ ] Clean enclosure ventilation openings
- [ ] Verify ground continuity

---

## Emergency Procedures

### If You Smell Burning

1. **Immediately disconnect power** at the circuit breaker
2. Do not touch the controller
3. Use a CO₂ or powder fire extinguisher (NOT water)
4. Have the installation inspected before re-use

### If Someone Receives an Electric Shock

1. **Do not touch the person** if they are still in contact with power
2. Disconnect power at the circuit breaker
3. Call emergency services immediately
4. Begin first aid (CPR if trained) once power is disconnected

---

## Related Documents

- [Build from Zero](/docs/build-from-zero/) — Complete build guide
- [Hardware Guide](/docs/hardware-guide/) — Hardware assembly details
- [Safety Model](/docs/safety-model/) — System safety architecture
- [Production Checklist](/docs/production-checklist/) — Pre-deployment checks
- [Security Checklist](/docs/security-checklist/) — Security hardening
