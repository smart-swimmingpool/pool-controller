---
title: Contactor Wiring Guide
summary: Why built-in relays fail on motor loads and how to wire external contactors for reliable pump switching — bill of materials, wiring diagram, step-by-step instructions
date: "2026-07-13"
draft: false
toc: true
type: docs
menu:
  docs:
    parent: Pool Controller
    name: Contactor Guide
    weight: 65
---

> **⚠️ Note:** ECM pumps (electronically commutated motors <100W) have
> large input capacitors that cause a brief high charging peak (10–20A
> for microseconds) each time the relay closes. Over many cycles this
> capacitive inrush can micro-weld relay contacts. An **RC snubber**
> (100nF + 100Ω) across the relay contact suppresses this effect and is
> sufficient for loads <100W — no contactor required.
>
> The contactor solution below is intended for larger pumps (>300W) or
> inductive loads where running current exceeds 2A.

## Overview

This document explains why the built-in relays of the NORVI AE01-R (and other
standard relay modules) can fail when switching motor loads (pumps), and how to
build a robust, long-term solution using **external DIN-rail contactors**.

**Applies to:** NORVI AE01-R (R0–R5) and external 5V relay modules (standard
ESP32 builds).

---

## Problem: Relay Contacts Weld Under Motor Load

### The NORVI Relays

The NORVI AE01-R has **6 built-in electromechanical relays** (SPST-NO,
5A/250V AC). These are rated for **resistive loads** — incandescent lamps,
heating elements, or low-inrush electronics.

A **pump is an inductive load** (motor with coils):

| Characteristic | Resistive Load | Motor Load (Inductive) |
|---------------|:--------------:|:----------------------:|
| Continuous current | As rated | As rated |
| Inrush current | 1–1.5× rated current | **5–10× rated current** |
| Switching arc | Minor | **Severe** (back-EMF) |
| Contact wear | Minimal | **High** |

**In practice:** A NORVI relay rated "5A" driving a 300–600W pump
(1.3–2.6A running, 6.5–13A inrush peak) can develop **welded contacts**
after a few hundred switching cycles. The relay coil still clicks audibly,
but the NO contact stays closed — the pump runs **continuously** even with
the controller showing OFF.

### Field Report

In one field installation this failure occurred on two different relay outputs
(R1, then R5) of the same NORVI AE01-R, both switching the same solar pump.
The pump turned out to be a **6–28W ECM type** (Lowara Artiga ECO AUTO+) —
well within the relay rating, yet contacts welded. The cause is capacitive
inrush from the ECM's input capacitors (~10–20A peak for microseconds each
switch-on). The identical R0 channel (pool pump, Speck BADU Magic II/6 450W
asynchronous motor) showed no failure.

---

## Solution: External DIN-Rail Contactor

A **contactor** is a heavy-duty relay built for motor loads. The NORVI relay
switches only the **contactor's coil** (a few mA at 24V DC) — the contactor's
main contacts handle the 230V AC load.

```
 NORVI Relay            Contactor (external)           Pump
 ┌──────────┐          ┌────────────────────┐      ┌────────┐
 │          │  24V DC  │  A1 (+)            │      │        │
 │ COM ─────┼──────────┤  (coil)            │      │        │
 │          │          │          A2 (-) ───┼── GND│        │
 │ NO  ─────┼──────┐   │                    │      │        │
 └──────────┘      │   │ 1 (L-input) ───────┤─── 2 │        │
                   │   └────────────────────┘      │  L-N  │
 230V L ───────────┘                               └────────┘
 230V N ─────────────────────────────────────────────┘
```

### Benefits

| Benefit | Description |
|---------|-------------|
| **Contact life** | Contactors rated for 1–5 million operations under motor load |
| **Serviceable** | Replace a defective contactor in 2 minutes on DIN rail — no soldering |
| **NORVI unloaded** | NORVI relays switch mA → no contact wear at all |
| **Cost-effective** | ~15–25€ per contactor |

---

## Recommended Contactors

### Finder 22 Series (recommended)

Compact DIN-rail contactors with built-in surge suppression.

| Type | Coil | Contact | Motor Load | Width | Price |
|------|:----:|:-------:|:----------:|:----:|:-----:|
| **22.21.0.024.4000** | 24V DC | 1× NO, 20A | 1.5 kW | 17.6mm | ~20–25€ |
| **22.32.0.024.4000** | 24V DC | 2× NO, 16A | 1.5 kW | 17.6mm | ~25–30€ |

**Note:** Finder 22 includes a built-in suppressor diode (varistor), but
adding an external 1N4007 flyback diode across the coil is still recommended.

### Eaton / Moeller DILM

Robust contactors, often more affordable.

| Type | Coil | Contact | Motor Load | Width | Price |
|------|:----:|:-------:|:----------:|:----:|:-----:|
| **DILM7-01 (24V DC)** | 24V DC | 3× NO, 7A | 3 kW | 45mm | ~12–15€ |

### Interface Relays (compact)

For maximum space saving:

| Type | Coil | Contact | Width | Price |
|------|:----:|:-------:|:----:|:-----:|
| **Finder 40.52.9.024.0040** | 24V DC | 1× SPDT 16A | 6.2mm | ~10€ |
| + **Socket 94.02** | — | — | — | ~3€ |

> **Note:** Interface relays are smaller than full-size contactors, but still
> significantly more robust than NORVI's on-board relays. Adequate for pumps
> up to 600W.

---

## Wiring Diagram

### 24V Control Circuit

```
 24V DC (+) ──┬── NORVI R0 COM ── NO ──┬── Contactor Pool A1 ── A2 ──┬── GND
              │                        │                            │
              ├── NORVI R4 COM ── NO ──┤── Contactor Solar A1 ── A2 ──┤
              │                        │                            │
              └───── NORVI 24V IN ─────┘                            │
                                                                    │
 GND ───────────────────────────────────────────────────────────────┘
```

**Flyback diodes 1N4007** across each contactor coil:
- Cathode (banded end) to **A1 (+)**
- Anode to **A2 (GND)**

These diodes absorb the back-EMF spike when the contactor coil switches off.
Without them, the voltage spike can damage the NORVI relay driver.

### 230V Power Circuit

```
230V L (Live) ─── RCD ─── MCB ──┬── Contactor Solar 1-2 ── Solar Pump L
                                 │
                                 └── Contactor Pool 1-2 ─── Pool Pump L

230V N (Neutral) ──────────────────────── Both Pump N
```

Always protect the 230V side with an RCD (30mA) and MCB (B10A or appropriate).

---

## Bill of Materials (Both Pumps)

| Qty | Item | Est. Price |
|:---:|------|:----------:|
| 2 | Finder 22.21.0.024.4000 (20A, 24V DC, 1× NO) | ~45€ |
| 2 | 1N4007 diode (flyback across contactor coil) | ~0.50€ |
| 5m | 3×1.5mm² cable (NYM-J or H07RN-F) | ~8€ |
| 1 | Ferrule crimp set 0.75mm² + 1.5mm² | ~5€ |
| 2 | Wago 221-412 (junction terminals) | ~4€ |
| — | Cable ties, heatshrink, labelling | ~5€ |
| **Total** | | **~65–70€** |

---

## Installation Steps

### 1. Preparation

1. **Disconnect power** — turn off 230V breaker, disconnect 24V supply
2. **Check DIN rail** — contactors are 17.6mm (Finder 22) or 45mm (Eaton).
   Plan space next to the NORVI.
3. **Prepare cables** — 1.5mm² for 230V, 0.75mm² for 24V. Crimp ferrules.

### 2. Wiring

 **Step 1: 24V Control Circuit**

1. **24V DC (+) to NORVI relay COM** — bridge from NORVI 24V terminal to
   COM-R0 and COM-R4
2. **NORVI relay NO to contactor A1** — R0-NO → Pool contactor A1,
   R4-NO → Solar contactor A1
3. **Contactor A2 to GND** — both A2 to GND bus
4. **Flyback diode** — 1N4007 across A1/A2 of each contactor
   (cathode to A1, anode to A2)

**Step 2: 230V Power Circuit**

1. **230V L from RCD/MCB** — to contactor terminal 1 (both contactors
   paralleled)
2. **Contactor terminal 2** — to respective pump L
3. **230V N** — directly to pump N (not through contactor)
4. **Earth (PE)** — if pump has a housing, connect separately

**Step 3: Verification**

1. **Continuity check** — 24V side: ~100–400Ω between A1 and A2 (coil
   resistance). Diode test: measure in reverse direction
2. **Short circuit check** — 230V side: > 1MΩ between L and N
3. **Isolation check** — > 10MΩ between 24V and 230V circuits
   (galvanic isolation)

### 3. Functional Test

1. Apply 24V → controller boots
2. Controller shows pumps OFF → **contactors must be open** (no continuity
   between terminal 1 and 2)
3. Turn pumps ON (Web UI, MQTT, or OLED) → **contactor must audibly click**
4. Pump starts running → **success**

---

## Troubleshooting

| Symptom | Cause | Solution |
|---------|-------|----------|
| Contactor silent | NORVI relay not switching | Check GPIO pin in controller, verify relay ID |
| Contactor clicks, pump dead | Contactor 1-2 not conducting | Check 230V supply, measure across terminals 1-2 |
| Pump runs despite OFF | Welded contactor **or** stuck NORVI relay | Replace contactor; NORVI diagnosis: measure GPIO level |
| Contactor buzzing | Coil voltage too low | Check 24V supply (min 21.6V DC) |
| Controller resets on switching | Missing flyback diode | Add 1N4007 diode |

---

## References

- [SVG Wiring Diagram: Contactor Circuit](wiring-contactor.svg)
- [NORVI AE01-R Configuration](norvi-ae01-r.md)
- [Hardware Guide](hardware-guide.md)
- [Finder 22 Datasheet](https://www.findernet.com/en/series/22-series-industrial-relays/)
- [Eaton DILM Datasheet](https://www.eaton.com)
