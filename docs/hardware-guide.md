---
title: Hardware Guide
summary: Step-by-step hardware assembly guide for the ESP32 Pool Controller — parts list with alternatives, wiring diagram for DS18B20 sensors and relay module, power supply, soldering tips, and manufacturing best practices
date: "2026-06-07"
lastmod: "2026-06-07"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "hardware", "guide", "circuit", "assembly", "wiring"]
menu:
  docs:
    parent: Pool Controller
    name: Hardware Guide
    weight: 20
---

## Overview

This guide walks you through building the Pool Controller hardware — from
selecting parts to final assembly. Even if you've never soldered before, the
step-by-step instructions will help you get it right.

> **Target audience**: DIY electronics enthusiasts with basic soldering experience.
> Total cost: **under 100€** (excluding pumps and pool infrastructure).

## Safety First ⚠️

- The controller switches **230V AC mains voltage** to the pumps. Improper
  wiring poses risk of electric shock or fire.
- Only work on the circuit when **disconnected from power**.
- Use a **residual-current device (RCD)** for the pump circuit.
- Keep sensor wiring (low voltage) physically separate from mains wiring.
- If in doubt, consult a qualified electrician for the mains connection.

---

## Required Parts (BOM)

| # | Component | Qty | Approx. Cost | Notes |
|---|-----------|:---:|:------------:|-------|
| 1 | ESP32 Development Board (e.g. ESP32 DevKit V1, NodeMCU-32S) | 1 | 10–15€ | Ensure it has at least 4MB flash |
| 2 | DS18B20 Temperature Sensor (waterproof, stainless steel probe, 1m cable) | 2 | 8–12€ | One for pool water, one for solar collector |
| 3 | 2-Channel 5V Relay Module (with optocoupler isolation) | 1 | 5–8€ | **Must** be active-high (see notes below) |
| 4 | Resistor 4.7kΩ (¼W or ⅛W, metal film or carbon film) | 2 | < 1€ | Pull-up for the OneWire data lines |
| 5 | Breadboard + jumper wires (for prototyping) **OR** Perfboard + pin headers/screw terminals (for permanent build) | 1 | 3–8€ | |
| 6 | USB power supply 5V/≥1A (e.g. phone charger) | 1 | 5–10€ | For the ESP32 alone |
| 7 | Hookup wire, 0.14–0.5mm² (AWG 26–20), various colors | — | 3–5€ | |
| 8 | Optional: enclosure (ABS/PVC project box, IP54 or better) | 1 | 5–10€ | Protect from splashes/dust |
| 9 | Optional: screw terminals (2-pin, 5mm pitch) | 4–6 | 2–3€ | For removable sensor/power connections |
| 10 | Optional: DS3231 RTC module | 1 | 3–5€ | Backup timekeeping (not required for normal NTP operation) |
| **Total** | | | **~45–75€** | Without enclosure; well under 100€ |

### Where to Buy

All parts are widely available on Amazon, AliExpress, eBay, or at electronics
distributors like Reichelt, Pollin, Conrad (DE/AT/CH).

- **ESP32**: Search for "ESP32 DevKit V1" or "ESP32 NodeMCU-32S". Avoid
  ESP32-S2/S3/C3 variants unless you adapt the firmware — the project targets
  standard ESP32 (Xtensa dual-core).
- **DS18B20**: Look for **stainless steel, waterproof** probes with 1m cable.
  Cheaper plastic-encapsulated sensors work too but are less durable outdoors.
- **Relay Module**: **Read the module specification carefully.** The firmware
  uses **active-high logic** (GPIO HIGH → relay ON). Many cheap modules are
  active-low with a jumper to switch. Check the documentation or use a jumper
  wire to verify before wiring permanently.
- **Resistors**: Any 4.7kΩ ±5% resistor works. Buy a 100-pack for < 3€.

---

## Pin Assignment (Firmware Defaults)

The firmware uses these GPIO pins (defined in `src/Config.hpp`):

| Constant | GPIO | Purpose |
|----------|:----:|---------|
| `PIN_DS_SOLAR` | **GPIO15** | DS18B20 data — Solar collector temperature |
| `PIN_DS_POOL` | **GPIO16** | DS18B20 data — Pool water temperature |
| `PIN_RELAY_POOL` | **GPIO18** | Relay control — Pool circulation pump |
| `PIN_RELAY_SOLAR` | **GPIO19** | Relay control — Solar heating pump |

> **Note on GPIO15**: This pin is a **Strapping pin** on the ESP32 — it affects
> the boot process if pulled HIGH or LOW at startup. The firmware works reliably
> with it here, but if you experience boot issues, consider moving the DS18B20
> sensors to **GPIO32/33** and updating `src/Config.hpp` accordingly (see the
> [Wiring Optimization](#optional-wiring-optimization) section).

---

## Wiring Diagram

```
                         +-------------------------+
                         |         ESP32           |
                         |                         |
  DS18B20 SOLAR DATA --->| GPIO15   (PIN_DS_SOLAR) |
  DS18B20 POOL  DATA --->| GPIO16   (PIN_DS_POOL)  |
  Relay IN1 (Pool)  ---->| GPIO18   (PIN_RELAY_POOL)|
  Relay IN2 (Solar) ---->| GPIO19   (PIN_RELAY_SOLAR)|
                         |                         |
  3.3V ------------------>| 3V3                     |
  GND ------------------->| GND                     |
                         +-----------+-------------+
                                     |
                                     | common GND
                                     v
         +--------+--------+--------+--------+
         |        |        |        |        |
    +----+--+  +--+----+  |   +----+--+  +--+----+
    |Solar  |  | Pool  |  |   | RLY1  |  | RLY2  |
    |DS18B20|  |DS18B20|  |   |(Pool) |  |(Solar)|
    |       |  |       |  |   |       |  |       |
    | VDD --+--+ VDD --+--+---> 3.3V  |  |       |
    | GND --+--+ GND --+-------> GND  |  |       |
    |DATA --->| GPIO15|  |   |       |  |       |
    |       |  |DATA -->|  |   | VCC  |  | VCC  |
    +-------+  +-------+  |   | -- 5V |  | -- 5V|
                           |   | GND  |  | GND  |
                           |   |      |  |      |
                    4.7kΩ  |   | IN1  |  | IN2  |
                     |     |   | <-18 |  | <-19 |
                     v     |   +------+  +------+
                3.3V ----+-+--- 3.3V
```

### Wire Connections (Step by Step)

#### 1. Temperature Sensors (DS18B20)

The DS18B20 has three wires (on waterproof probes: typically **red = VDD**,
**yellow/white = DATA**, **black = GND** — but **verify with your sensor's
datasheet**):

| DS18B20 Wire | Connect to |
|:------------:|------------|
| Red (VDD) | ESP32 **3.3V** |
| Black (GND) | ESP32 **GND** |
| Yellow/White (DATA) | **GPIO15** (solar) or **GPIO16** (pool) |

**Critical — add the pull-up resistor:**

Connect a **4.7kΩ resistor** between the DATA wire and the **3.3V** line.
One resistor per sensor, as close to the sensor wire connection as possible.

```
    ESP32 3.3V ──┬── 4.7kΩ ──── DS18B20 DATA
                  │
                DS18B20 VDD
```

Without this resistor, the sensor will not be detected — this is the #1
cause of "no sensor found" errors.

#### 2. Relay Module

| Relay Module | Connect to |
|:------------:|------------|
| VCC (or VDD) | **5V** (from the ESP32 board's VIN/5V pin, or external 5V supply) |
| GND | **GND** (common with ESP32) |
| IN1 | **GPIO18** (pool pump) |
| IN2 | **GPIO19** (solar pump) |

**Relay Logic**: The firmware sets the GPIO pin HIGH (3.3V) to activate the
relay. If your module switches with LOW (active-low), flip the jumper or adapt
the firmware (see `src/RelayModuleNode.cpp`).

**Load wiring (230V side):**

```
    L ──┤ RCD ├──┤ MCB ├──┬──┤ Relay-COM1 ├── Pool Pump ── N
                           └──┤ Relay-COM2 ├── Solar Pump ── N
```

- Connect the pump's live wire to the relay's **COM** (common) terminal.
- Connect the relay's **NO** (normally open) terminal to the pump.
- The other pump wire goes to **neutral (N)**.
- Always wire the 230V circuit through an **RCD and appropriately rated MCB**.

#### 3. Power Supply

| Component | Supply | Source |
|-----------|:------:|--------|
| ESP32 board | **5V USB** (regulated) | Phone charger, USB port, or dedicated 5V PSU |
| Relay module (coils) | **5V** | From ESP32 VIN pin OR external 5V supply |
| DS18B20 sensors | **3.3V** | From ESP32 3.3V output pin |
| (Optional) RTC DS3231 | **3.3V** | From ESP32 3.3V output pin |

> **Important**: The ESP32's on-board 3.3V regulator can supply ~600mA. The
> DS18B20 sensors draw < 5mA total — well within limits. If you add many
> additional 3.3V components, consider an external 3.3V regulator.

---

## Step-by-Step Assembly

### Prototyping on a Breadboard

1. **Place the ESP32** on the breadboard straddling the center gap
2. **Connect power rails**: 3.3V and GND rails on both sides
3. **Insert the 4.7kΩ resistors**: Between the DATA row and 3.3V rail
4. **Connect DS18B20 sensors**: Use jumper wires for VDD (3.3V), GND, and DATA
5. **Connect the relay module**: Jumper wires for VCC (5V), GND, IN1, IN2
6. **Power via USB**: The ESP32's VIN/USB pin provides 5V for the relay module
7. **Double-check all connections** before applying power
8. **Verify**: See [First Power-On](#first-power-on-and-testing) below

{{< figure
  library="true"
  src="../pool-controller_breadboard.png"
  title="Pool Controller breadboard prototype with ESP32, two DS18B20 temperature sensors, 4.7kΩ pull-up resistors, and 2-channel relay module"
  lightbox="true" >}}

> 💡 **Tip**: Use different colored jumper wires for clarity — e.g., red for
> power (3.3V/5V), black for GND, yellow for sensor data, blue for relay
> control.

### Permanent Assembly on Perfboard

Once you've verified the circuit on a breadboard, build the permanent version:

1. **Plan the layout**: Arrange components on the perfboard before soldering.
   Keep the 230V relay terminals at one edge, sensors at the opposite edge.
2. **Solder in this order**:
   - Pin headers / screw terminals for the ESP32 (socket it, don't solder directly)
   - Resistors (4.7kΩ)
   - Pin headers for the relay module (socket it too)
   - Screw terminals for sensor connections
3. **Wire routing**: Use solid core wire for connections. Keep data lines short.
4. **Inspect for solder bridges**: Check each joint with a magnifier or
   multimeter (continuity test).
5. **Mount in enclosure**: Use standoffs or double-sided foam tape. Drill holes
   for sensor and relay cable entries. Use cable glands (PG7/PG9) for a
   water-resistant seal.

---

## Manufacturing Tips

### Soldering

- **Use leaded solder** (Sn60Pb40 or Sn63Pb37) for easiest handling —
  it flows better than lead-free, especially for beginners.
- **Flux**: Use rosin-core solder; add liquid flux for stubborn joints.
- **Temperature**: Set iron to 320–350°C for leaded, 370–400°C for lead-free.
- **Iron tip**: Clean with a wet sponge or brass wool **before every joint**.
- **Good joint**: Shiny, concave fillet that flows around the component lead.
  A dull, cracked, or ball-shaped joint is **bad** — reheat and add fresh solder.

### Enclosure

- Choose an **ABS or PVC project box** with at least IP54 rating for outdoor
  installation (splash protection).
- Drill ventilation holes if the relay module gets warm, but angle them
  downwards to reduce water ingress.
- Mount the ESP32 on **M2.5 or M3 nylon standoffs** to avoid short circuits.
- Label external connectors (pool sensor, solar sensor, pump 1, pump 2, USB power).
- Use **cable glands** (PG7 for thin sensor cables, PG9 for thicker power cables)
  where wires enter the enclosure.

### Cable Management

- **Label both ends** of each wire with heat-shrink wrap or small adhesive
  labels — future-you will thank past-you.
- **Keep sensor and relay wires separate** inside the enclosure to minimize
  electrical noise coupling.
- **Strain relief**: Tie internal wires to mounting posts or use cable ties so
  that pulling on external cables doesn't stress solder joints.
- **Service loop**: Leave enough slack inside the enclosure so you can open it
  and work without disconnecting everything.

### Outdoor Sensor Installation

- **DS18B20 probes** are waterproof but the cable entry at the sensor end is
  not always fully sealed. Apply **heat-shrink tubing** over the cable joint
  or use **self-amalgamating silicone tape** for outdoor installations.
- Route sensor cables in **conduit** (PVC or flexible) where they are exposed
  to mechanical stress (lawn, pavement).
- **Max cable length**: DS18B20 works reliably up to ~30m with a 4.7kΩ pull-up
  and twisted-pair cable. For longer runs, reduce the pull-up to 2.2kΩ or use
  a dedicated OneWire driver.
- Position the pool sensor in the **pump circuit** (after the filter, before
  the pump return) for accurate average pool temperature.
- Position the solar sensor at the **hottest point of the solar collector**
  (usually the top outlet pipe).

---

## Power Supply Options

| Option | Pros | Cons |
|--------|------|------|
| **USB phone charger** (5V/1A+) | Cheap, readily available, safe | Limited current for additional peripherals |
| **DIN-rail PSU** (Mean Well HDR-15-5 or similar) | Professional, reliable, fits in electrical cabinet | Slightly more expensive (~15€) |
| **ESP32 VIN from USB** + relay from same 5V | Simple wiring | Total current must stay under ESP32 board's limit |

**Recommendation**: If you're installing in a permanent location near your pump
control panel, use a **DIN-rail 5V PSU** (e.g., Mean Well HDR-15-5). It's
clean, reliable, and can power both the ESP32 and the relay module without
issue.

---

## First Power-On and Testing

### 1. Visual Inspection

Before connecting power:
- Check for **solder bridges** between adjacent pins
- Verify **polarity** of all components (DS18B20 VDD/GND, relay VCC/GND)
- Ensure **no stray wire strands** are shorting neighboring pins
- Measure resistance between **3.3V and GND** — should be > 10kΩ (not shorted)

### 2. Power On

1. Connect USB power (or 5V PSU)
2. The ESP32's **built-in LED** should flash quickly (boot sequence)
3. After ~3 seconds, the LED blinks **slowly** — waiting for WiFi

### 3. Verify Sensors

Open the serial monitor (9600 baud):
```
Pool Controller v3.3.0
Starting up...
Initialized pins: GPIO15, GPIO16, GPIO18, GPIO19
Solar Temp: GPIO15
Pool Temp:  GPIO16
Pool Relay: GPIO18
Solar Relay: GPIO19
```

If sensors are connected and working:
```
Solar temperature: 25.3°C
Pool temperature:  22.1°C
```

If you see `Sensor error` or `-127°C`, check:
- [ ] 4.7kΩ pull-up resistor present on each DATA line?
- [ ] DS18B20 VDD connected to 3.3V (not 5V)?
- [ ] DS18B20 GND connected to common ground?
- [ ] DATA pin matches the firmware configuration?

### 4. Test Relays

From the Web UI (Configuration tab) or via serial command:
```
Mode: manual
Pool pump: ON  → relay should click, pump starts
Solar pump: ON → relay should click, pump starts
```

If relay doesn't click:
- [ ] Is the relay module powered (5V between VCC and GND)?
- [ ] Is the logic level correct (active-high vs active-low)?
- [ ] Does the LED on the relay module light up when GPIO goes HIGH?

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| "Sensor error" or `-127°C` | Missing pull-up resistor | Add 4.7kΩ between DATA and 3.3V |
| "Sensor error" | Wrong GPIO pin | Check `PIN_DS_SOLAR`/`PIN_DS_POOL` in `src/Config.hpp` |
| Intermittent sensor readings | Loose connection or noise | Check solder joints, separate data from relay wires |
| Relay doesn't activate | Wrong logic level | Check active-high vs active-low; add jumper or change firmware |
| Relay clicks but pump doesn't run | 230V wiring issue | Check COM/NO terminals, verify pump connection |
| ESP32 won't boot (brownout) | Insufficient power | Use 5V/≥1A power supply; add 100µF capacitor near VIN |
| ESP32 resets when relay switches | Voltage spike on relay coil | Add flyback diode across relay coil, or use module with built-in protection |
| Sensor readings jump when relay switches | Electrical noise | Route sensor wires away from relay/power wires |

---

## Optional: Wiring Optimization

The default pins (GPIO15/16/18/19) work reliably for most users. If you
experience boot instability or want the most robust configuration, use these
**optimized pins** instead:

| Function | Default Pin | Optimized Pin | Reason |
|----------|:-----------:|:-------------:|--------|
| DS18B20 Solar | GPIO15 | **GPIO32** | GPIO15 is a Strapping pin — removing OneWire from it eliminates any boot risk |
| DS18B20 Pool | GPIO16 | **GPIO33** | Clean separation from remaining Strapping pin GPIO0 |
| Relay Pool | GPIO18 | **GPIO25** | ADC2 pads (GPIO18/19) are avoided; GPIO25 is a clean digital output |
| Relay Solar | GPIO19 | **GPIO26** | Same as above |

To use optimized pins, edit `src/Config.hpp`:

```cpp
constexpr uint8_t PIN_DS_SOLAR{32};     // was 15
constexpr uint8_t PIN_DS_POOL{33};      // was 16
constexpr uint8_t PIN_RELAY_POOL{25};   // was 18
constexpr uint8_t PIN_RELAY_SOLAR{26};  // was 19
```

This pinout is already analyzed and recommended in the
[ESP32 Schematic Optimization](esp32-schematic-optimization-de.md) document
(German).

---

## References

- Fritzing source file: [pool-controller.fzz](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/pool-controller.fzz)
- [ESP32 Schematic Optimization (DE)](esp32-schematic-optimization-de.md)
- [ESP32 Complete Wiring Schematic (DE)](esp32-complete-wiring-schematic-de.md)
- [DS18B20 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf)
- [ESP32 Pin Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [Config.hpp pin source](https://github.com/smart-swimmingpool/pool-controller/blob/main/src/Config.hpp)
