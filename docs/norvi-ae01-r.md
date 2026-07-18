---
title: NORVI AE01-R Configuration
summary: Pool Controller pin mapping, wiring, and differences when using the industrial NORVI IIOT-AE01-R controller instead of a standard ESP32 dev board
date: "2026-06-09"
lastmod: "2026-07-13"
draft: false
toc: true
type: docs
menu:
  docs:
    parent: Pool Controller
    name: NORVI AE01-R
    weight: 60
---

## Overview

The [NORVI IIOT-AE01-R](https://norvi.io/docs/norvi-iiot-ae01-r-datasheet/) is an
industrial ESP32-WROOM32 controller with built-in relays, transistor outputs,
digital inputs, an OLED display, RS-485, and push buttons — all in a
DIN-rail-mountable enclosure (IP20). It is CE certified
(EN 61131-2, EN 61010-1) and designed for permanent installation in electrical
cabinets.

**Key differences from a standard ESP32 dev board:**

| Feature                 | Standard ESP32 Dev Board | NORVI AE01-R                          |
| ----------------------- | ------------------------ | ------------------------------------- |
| Supply voltage          | **5V** (USB)             | **24V DC**                            |
| Relay module            | External (needs 5V)      | **6 built-in relays** (5A 250V AC)    |
| Power supply for relays | Separate 5V needed       | Integrated — powered from 24V         |
| Status indication       | Built-in LED only        | **0.96" OLED display** + external LED |
| Enclosure               | Open board (breadboard)  | **DIN-rail enclosure** (IP20)         |
| Certifications          | None                     | CE (EN 61131-2, EN 61010-1)           |
| Wiring                  | Screw terminals / Dupont | **Industrial screw terminals**        |
| Digital inputs          | None                     | **8x sink/source (18–32V DC)**        |
| Additional I/O          | None                     | 2x transistor output, RS-485          |
| User interface          | None                     | **3 front-panel push buttons**        |

> **If you already have a 230V AC → 24V DC power supply**, the NORVI AE01-R can
> be powered directly from it — no separate 5V PSU needed.

---

## Pin Assignment

{{% callout warning %}}
**Important hardware limitation:** The NORVI's 8 digital inputs (GPIO18/19/21/22/23/34/35/39)
are **optocoupler-isolated input-only** — they expect 18–32V DC signals and
cannot be used for bidirectional 1-Wire (DS18B20) communication.
{{% /callout %}}

The firmware uses the `NORVI_AE01_R` preprocessor macro to select an alternate
pin mapping that avoids the occupied and input-only pins:

| Constant          |    NORVI GPIO     | Purpose                                      |            Connection            |
| ----------------- | :---------------: | -------------------------------------------- | :------------------------------: |
| `PIN_DS_SOLAR`    |    **GPIO25**     | DS18B20 — Solar collector temperature        |       Expansion Port Pin 1       |
| `PIN_DS_POOL`     |    **GPIO25**     | DS18B20 — Pool water temperature             | Shared bus on GPIO25 (expansion) |
| `PIN_RELAY_POOL`  |  **GPIO14** (R0)  | Relay — Pool circulation pump                |          Relay Output 0          |
| `PIN_RELAY_SOLAR` |  **GPIO2** (R4)   | Relay — Solar heating pump (moved: R1→R5→R4) |          Relay Output 4          |
| `PIN_LED_STATUS`  | **GPIO27** (T0.1) | Status LED (external, via transistor output) |      Transistor Output 0.1       |
| `PIN_OLED_SDA`    |    **GPIO16**     | I2C SDA — built-in 0.96" OLED (SSD1306)      |          Internal (I2C)          |
| `PIN_OLED_SCL`    |    **GPIO17**     | I2C SCL — built-in 0.96" OLED (SSD1306)      |          Internal (I2C)          |
| `PIN_BUTTON_ADC`  |    **GPIO32**     | 3 front-panel buttons (via analog ADC)       |    Internal (resistor ladder)    |

### Connecting the DS18B20 Sensors — Shared OneWire Bus

Both sensors share a **single OneWire bus on GPIO25** on the **Expansion Port**
(a 10-pin male header on the side of the NORVI enclosure, 2.54 mm pitch).
DATA, 3.3V, and GND are all available on this header — no soldering needed.

```
   NORVI AE01-R — Expansion Port (10-pin male header, 2.54 mm)
   ┌───────────────────────────────────────────────────────────┐
   │                                                           │
   │  Pin 1  ◄─────── IO25 ──── DS18B20 DATA (yellow)         │
   │  Pin 2         TXD0                                      │
   │  Pin 3         NC                                        │
   │  Pin 4         RXD0                                      │
   │  Pin 5         IO0 (BOOT)                                │
   │  Pin 6         IO32 (buttons)                             │
   │  Pin 7  ◄─────── 3.3V ──── DS18B20 VCC (red)             │
   │  Pin 8         SCL (OLED)                                 │
   │  Pin 9  ◄─────── GND  ──── DS18B20 GND (black)           │
   │  Pin 10        SDA (OLED)                                 │
   │                                                           │
   └───────────────────────────────────────────────────────────┘
                             ↑
               3× Female-Female Dupont cables
```

**Wiring table:**

| Expansion Port | Dupont (colour) | DS18B20 | Also connect to |
| -------------- | --------------- | ------- | --------------- |
| Pin 1 (IO25)   | Yellow          | DATA    | 4.7 kΩ → 3.3V   |
| Pin 7 (3.3V)   | Red             | VCC     | 4.7 kΩ → DATA   |
| Pin 9 (GND)    | Black           | GND     | —               |

Both sensors share the **same three pins** — connect both DATA lines to Pin 1,
both VCC to Pin 7, and both GND to Pin 9. A single **4.7 kΩ pull-up resistor**
between Pin 1 and Pin 7 is sufficient for both sensors.

> **Pull-up resistor color code:** Gelb – Violett – Rot – Gold (±5%)
>
> ```
>   Gelb  Violett  Rot   Gold
>   ┌──┐  ┌──┐  ┌──┐  ┌──┐
>   │  │  │  │  │  │  │  │
>   └──┘  └──┘  └──┘  └──┘
>    4     7    ×100  ±5%
>   = 47 × 100 = 4700 Ω = 4,7 kΩ
> ```

{{% callout note %}}
**Shared Bus Details:** The firmware uses the `NORVI_AE01_R` preprocessor macro
to create a single `DallasTemperature` instance shared by both sensor nodes.
The solar node (device index 0) drives `requestTemperatures()`; the pool node
(device index 1) reads from the same conversion. Both sensors require a 4.7 kΩ
pull-up resistor between the DATA line and 3.3 V — one resistor is enough when
they share the same bus.
{{% /callout %}}

{{% callout tip %}}
**Used the old soldered GPIO5?** If you already soldered your pool sensor to
the ESP32 module pin, the firmware falls back to the dedicated-bus mode when
`NORVI_AE01_R` is **not** defined. You can keep your existing wiring and just
omit the build flag, or move both sensors to the shared GPIO25 for a cleaner
setup.
{{% /callout %}}

### Sensor Identification — First Boot

Because both sensors share the same bus, the scan order determines which device
is assigned to Solar (index 0) and which to Pool (index 1). To ensure they stay
assigned correctly across reboots (e.g., when adding or replacing sensors), use
the **OLED Sensor Setup Wizard** or the serial console.

#### OLED Setup Wizard (recommended)

1. **Enter setup:** long-press (2 s) the left Button **B1** on the front panel.
2. The display shows both detected DS18B20 sensors with their ROM addresses and
   current temperatures.
3. **Identify your sensors:** warm one sensor with your finger — its temperature
   will rise on the display. Press **B2** to assign the currently selected
   sensor as **Solar**, or **B3** to assign it as **Pool**.
4. Repeat for the second sensor. The badge `SOLAR` or `POOL` appears next to
   each assigned device.
5. **Save & reboot:** once both sensors are assigned, long-press **B2** to
   write the mapping to NVS and restart. The mapping survives all future
   reboots.

> **Cancel:** long-press **B1** again at any time to exit without saving.

#### Serial Console

On first boot (or when no mapping is saved), the serial output prints:

```
• Sensor mapping: no addresses configured — using default device indices
  ℹ To configure: long-press Button 1 → Sensor Setup page → assign sensors
```

After identifying your sensors (warm one, watch the serial output for its address),
you can find the ROM addresses in the boot log and configure them via the Web UI
(coming soon).

#### How the Mapping Works

The 8-byte ROM address of each sensor is stored in NVS (Preferences namespace
`ds18b20`, keys `solar_adr` and `pool_adr`). On every subsequent boot, the
firmware scans the bus for a device matching the stored address. If found, that
device is used — regardless of its bus scan position. If the stored sensor is
not found (e.g., it was replaced), the firmware prints a warning and falls back
to the default device index.

---

## Wiring Diagram

```
 ─── POWER SUPPLY ─────────────────────────────────────────────────────

  [230V AC → 24V DC PSU]        [NORVI AE01-R]
   ┌──────────────────┐          ┌──────────────────────────┐
   │ 24V (+) ─────────┼──────────┤ 24V DC (+)              │
   │                  │          │                          │
   │ GND  ────────────┼──────────┤ GND (0V)                │
   └──────────────────┘          │                          │
                                 │  The NORVI powers itself │
                                 │  and the relays from 24V │
                                 └──────────────────────────┘

  ─── SENSORS (Shared OneWire Bus on Expansion Port) ────────────────

   ┌── DS18B20 Solar ──┐    ┌── DS18B20 Pool ───┐    NORVI Expansion Port
   │                   │    │                   │    ┌──────────────────────┐
   │ VDD (red) ────────┼────┼──── VDD (red) ────┼────┤ Pin 7 (3.3V)        │
   │                   │    │                   │    │                      │
   │ GND (blk) ────────┼────┼──── GND (blk) ────┼────┤ Pin 9 (GND)         │
   │                   │    │                   │    │                      │
   │DATA (yel) ────────┼────┼──── DATA (yel) ───┼────┤ Pin 1 (GPIO25)      │
   │                   │    │    │              │    └──────────────────────┘
   └───────────────────┘    │    │              │
                             │    │              │
                             │    └──[4.7kΩ]─────┼── 3.3V (single pull-up)
                             │                  │
                             └──────────────────┘

   One 4.7kΩ resistor between DATA (Pin 1) and 3.3V (Pin 7) is enough.
   Dupont cables: 3× Female-Female (red/yellow/black).

   ─── RELAY OUTPUTS (built-in) ─────────────────────────────────────────

   ╔══════════════════════════════════════════════════════════════════════╗
   ║  ⚠ IMPORTANT — Motor Load Consideration                            ║
   ║                                                                     ║
   ║  The NORVI built-in relays are rated for **5A resistive load**.     ║
   ║  A pump is a motor load with inrush current that can weld relay     ║
   ║  contacts after repeated switching.                                 ║
   ║                                                                     ║
   ║  **Standard configuration:** Use the NORVI relays as **control      ║
   ║  relays for external DIN-rail contactors** (see Contactor Wiring    ║
   ║  Guide below). This is zero-wear on the NORVI relays.               ║
   ║                                                                     ║
   ║  **For low-power pumps (<100W, e.g. ECM solar pumps):** Direct      ║
   ║  wiring is possible with an **RC snubber** (100nF + 100Ω,          ║
   ║  X2-rated capacitor) across the relay contacts to suppress          ║
   ║  capacitive inrush.                                                  ║
   ║                                                                     ║
   ║  See → **[Contactor Wiring Guide](contactor-guide.md)**             ║
   ╚══════════════════════════════════════════════════════════════════════╝

   NORVI Relay Output 0 (GPIO14):   Control → Pool pump (via contactor or direct <100W)
   NORVI Relay Output 4 (GPIO2):    Control → Solar pump (via contactor or direct with snubber)

   L (mains) ─── RCD ─── MCB ──┬── Relay COM0 ── NO0 ── Pump L
                                └── Relay COM4 ── NO4 ── Pump L
   N (neutral) ───────────────────────── Neutral bar ── Pump N

   ── Standard wiring (contactors) ─────────────────────────────────────

   24V DC (+) ──┬── NORVI R0 COM ── NO ──┬── Ext. Contactor Pool A1 ── A2 ──┬── GND
                 │                        │                                  │
                 ├── NORVI R4 COM ── NO ──┤── Ext. Contactor Solar A1 ── A2 ──┤
                 │                        │                                  │
                 └───── NORVI 24V IN ─────┘                                  │
                                                                             │
   GND ─────────────────────────────────────────────────────────────────────┘

   Each contactor requires a 1N4007 flyback diode across its coil
   (cathode to A1, anode to A2). The 230V load side:

   L (mains) ─── RCD ─── MCB ──┬── Contactor Solar 1-2 ── Solar Pump L
                                └── Contactor Pool 1-2 ─── Pool Pump L
   N (neutral) ───────────────────────── Neutral bar ── Both Pumps N

   ── Alternative: direct wiring (resistive/low-power loads) ───────────

   For **non-motor loads** (heating elements, valves, signal lamps) or
   **low-power ECM pumps (<100W) with an RC snubber**, the NORVI relays
   can be wired directly. They are Normally Open (SPST), rated 5A/250V AC.

   L (mains) ─── RCD ─── MCB ──┬── NORVI COM0 ── NO0 ── Load
                                └── NORVI COM4 ── NO4 ── Load
   N (neutral) ───────────────────────── Neutral bar ── Load N

   > **For pumps >100W or without RC snubber:** Use external contactors
   > as shown above. Direct wiring of motor loads will eventually weld
   > the relay contacts.

   ── Field report ─────────────────────────────────────────────────────

   In one field installation, a solar pump was connected to Relay Output 1
   (GPIO12, R1). After a few hundred switching cycles, R1 developed a
   welded contact — the relay clicked audibly but the NO contact never
   opened. Subsequent rewiring to R5 (GPIO33) failed identically. R0
   (GPIO14, pool pump) continued working (different motor characteristic).
   The pump is now configured for R4 (GPIO2). An RC snubber (100nF + 100Ω,
   X2-rated capacitor) is recommended across relay contacts for ECM
   capacitive inrush protection.

   ⚡ Relay polarity: Unlike standard external relay modules (which are
   typically active-LOW: LOW = ON, HIGH = OFF), the NORVI AE01-R built-in
   relays are **active-HIGH**: writing `HIGH` to the GPIO pin energizes
   the relay coil (contact closes), and `LOW` de-energizes it (contact opens).

   The firmware handles this automatically via the `RelayModuleNode` constructor's
   `activeLow` parameter. On NORVI builds the relays are instantiated with
   `activeLow = false` (see `PoolController.cpp`). When porting to a different
   board with built-in relays, verify the relay driver polarity.

 ─── STATUS LED (external) ────────────────────────────────────────────

   NORVI Transistor Output 0.1 (GPIO27):
     GPIO27 ─── 330Ω ─── LED (anode) ─── LED (cathode) ─── GND

   The transistor output is open-collector (100mA max). When the pin
   goes HIGH, the internal transistor switches to GND, completing the
   circuit. Use a standard 5mm LED with a 330Ω series resistor.
```

---

## Built-in OLED Display

The NORVI AE01-R includes a **0.96" SSD1306 OLED display** (128×64, I2C
address 0x3C). The firmware uses it to show four information pages:

| Page  | Content                                                                                                            |
| :---: | ------------------------------------------------------------------------------------------------------------------ |
| **1** | Pool & solar temperature, pump status, operation mode, and button hints (▲ next page, ▼ toggle pump, ■ cycle mode) |
| **2** | WiFi SSID, IP address, MQTT connection status, AP mode hint                                                        |
| **3** | System uptime, free heap, low-watermark heap, firmware version                                                     |
| **4** | QR code linking to the web dashboard at `http://<device-ip>/`                                                      |

**Shared footer:** All pages show a separator line at the bottom followed by
the current operation mode (page 1 only), local time (HH:MM), firmware version
(`vX.Y.Z`), and page number. The time updates with every display refresh and
uses the configured timezone with automatic daylight saving time (DST).

**Auto WiFi page:** When no WiFi is configured (AP mode) or the connection is
lost, the display automatically switches to the **Network Status** page (page 2)
at startup so the user can see the AP name and the setup URL immediately.

**QR code page:** The last page (page 4) generates a QR code for the web
dashboard URL (`http://<ip>/`). Scan it with your phone's camera to open the
dashboard quickly without typing the IP address. The QR code is generated
on-device using the `ricmoo/QRCode` library.

The display updates every 2 seconds and is controlled by the front-panel
buttons (see below).

---

## Front-Panel Buttons

The three push buttons on the front panel are read via analog ADC on GPIO32.
Each button produces a distinct voltage level through a built-in resistor
ladder:

|     Button     | Action                                                    |
| :------------: | --------------------------------------------------------- |
|  **1** (left)  | Cycle OLED display page forward (1 → 2 → 3 → 4 → 1)       |
| **2** (middle) | In **manual** mode: toggle pool pump ON/OFF               |
| **3** (right)  | Cycle operation mode (auto → manu → boost → timer → auto) |

The ADC thresholds are configured in `NorviButtonHandler.hpp`. Raw values
are logged via serial on startup for calibration.

---

## Power Supply

The NORVI AE01-R runs on **24V DC** (±10%). The built-in relays and the
ESP32 are both powered from this single supply.

| Parameter           | Value                       |
| ------------------- | --------------------------- |
| Rated voltage       | **24V DC**                  |
| Current consumption | **400 mA** (all relays off) |
| Recommended PSU     | **1A 24V DC** (or more)     |

> **Note:** Unlike the standard ESP32 dev board setup, the NORVI does **not**
> need a separate 5V USB supply or a 5V relay module PSU. Everything runs
> from the single 24V DC input.

---

## What's Different — Comparison with Standard Setup

| Aspect           | Standard ESP32 Dev Board  | NORVI AE01-R                                      |
| ---------------- | ------------------------- | ------------------------------------------------- |
| **Power**        | 5V USB or 5V PSU          | 24V DC (single supply)                            |
| **Relays**       | External 2-channel module        | 6 built-in (we use 2)                             |
| **Relay power**  | 5V to relay module             | Integrated (24V → relay coils)                    |
| **Relay polarity** | Active-LOW (LOW = ON)        | **Active-HIGH** (HIGH = ON)                       |
| **Sensor pins**  | GPIO32, GPIO33            | GPIO25 (Exp Port), GPIO5 (solder)                 |
| **Relay pins**   | GPIO25, GPIO26            | GPIO14 (R0), GPIO2 (R4)                           |
| **Status LED**   | Built-in (GPIO2)          | External via GPIO27 (T0.1)                        |
| **OLED display** | None                      | Built-in 0.96" (SSD1306) — 4 info pages + QR code |
| **Push buttons** | BOOT button (GPIO0)       | 3 front buttons — cycle pages & modes             |
| **Mounting**     | Breadboard / enclosure    | **DIN-rail** (standard electrical cabinet)        |

### What's no longer needed

- **External relay module** — eliminated (built-in)
- **Separate 5V PSU for relays** — eliminated (24V integrated)
- **5V USB power supply** — eliminated (24V single supply)
- **Breadboard / perfboard** — eliminated (screw terminals)

### What's still the same

- DS18B20 sensors with 4.7kΩ pull-up resistors
- 230V AC wiring (RCD/MCB protection, pump connections)
- MQTT / Home Assistant / Homie Convention — all software is identical

---

## Building the Firmware

The NORVI variant is selected at compile time via the `NORVI_AE01_R` preprocessor
macro. Use the dedicated PlatformIO environment:

```bash
pio run -e norvi_ae01_r -t upload
```

This environment:

- Defines `-D NORVI_AE01_R` to select the alternate pin mapping (shared OneWire bus)
- Includes `Adafruit SSD1306` and `Adafruit GFX Library` for the OLED
- Initializes I2C on GPIO16/GPIO17 and the button ADC on GPIO32
- Instantiates relay nodes with `activeLow = false` for the built-in active-HIGH relays

To also upload the LittleFS filesystem (web UI):

```bash
pio run -e norvi_ae01_r -t uploadfs
```

> **Note:** The NORVI's USB port is used for programming but shares the UART
> with RS-485. `Serial.print()` may interfere with RS-485 communication —
> use the OLED display for debug output when RS-485 is active.

---

## NORVI AE01-R Pin Reference

|  GPIO  | NORVI Function                         |   Used by Pool Controller?   |
| :----: | -------------------------------------- | :--------------------------: |
|   0    | NRST (outputs PWM at boot)             |              ❌              |
|   1    | RS-485 TX (shared with USB)            |              ❌              |
| **2**  | **Relay Output 4 / Built-in LED**        |   ✅ **Solar pump (R4)**     |
|   3    | RS-485 RX (shared with USB)            |              ❌              |
|   4    | RS-485 Flow Control                    |              ❌              |
| **5**  | **— (free GPIO, no NORVI peripheral)** | ✅ **DS18B20 Pool (solder)** |
|   12   | Relay Output 1 (welded — use R4 instead) |              ❌              |
|   13   | Relay Output 2                         |              ❌              |
|   14   | **Relay Output 0**                     |         ✅ Pool pump         |
|   15   | Relay Output 3                         |              ❌              |
|   16   | **I2C SDA** (OLED display)             |       ✅ OLED display        |
|   17   | **I2C SCL** (OLED display)             |       ✅ OLED display        |
|   18   | Digital Input 0 (input only)           |              ❌              |
|   19   | Digital Input 4 (input only)           |              ❌              |
|   20   | —                                      |              ❌              |
|   21   | Digital Input 5 (input only)           |              ❌              |
|   22   | Digital Input 6 (input only)           |              ❌              |
|   23   | Digital Input 7 (input only)           |              ❌              |
| **25** | **Expansion Port Pin 1**               |     ✅ **DS18B20 Solar**     |
|   26   | Transistor Output 0.0                  |          ❌ (free)           |
|   27   | **Transistor Output 0.1**              |   ✅ Status LED (external)   |
| **32** | **Analog Input / Buttons**             |   ✅ 3 front-panel buttons   |
|   33   | Relay Output 5                         |              ❌              |
|   34   | Digital Input 2 (input only)           |              ❌              |
|   35   | Digital Input 3 (input only)           |              ❌              |
|   38   | —                                      |              ❌              |
|   39   | Digital Input 1 (input only)           |              ❌              |

> **Note:** GPIOs marked "input only" (GPIO34, 35, 39) lack internal pull-ups
> and are connected to optocoupler circuits. They cannot be used for 1-Wire
> or any bidirectional protocol.

---

## References

- [NORVI IIOT-AE01-R Datasheet](https://norvi.io/docs/norvi-iiot-ae01-r-datasheet/)
- [NORVI IIOT-AE01-R User Guide](https://norvi.io/docs/norvi-iiot-ae01-r-user-guide/)
- [KiCad Schematic: NORVI AE01-R](kicad/norvi-ae01-r/norvi-ae01-r-schematic.pdf) — KiCad 9.0 schematic PDF export
- [Main Hardware Guide](hardware-guide.md)
- [Contactor Wiring Guide](contactor-guide.md) — External contactors for reliable pump switching
- [SVG Wiring Diagram: Contactor Circuit](wiring-contactor.svg)
- [Config.hpp pin source](https://github.com/smart-swimmingpool/pool-controller/blob/main/src/Config.hpp)
