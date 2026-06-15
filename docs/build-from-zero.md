---
title: Build from Zero
summary: Complete step-by-step guide to build the Pool Controller from scratch — hardware variants, bill of materials, flashing, first commissioning, MQTT setup, sensor testing, relay test, Home Assistant integration, electrical commissioning, and maintenance
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "build", "hardware", "guide", "commissioning", "tutorial"]
menu:
  docs:
    parent: Pool Controller
    name: Build from Zero
    weight: 10
---

> ⚠️ **WARNING: This guide involves 230V AC mains voltage.**
> Improper wiring poses risk of electric shock, fire, or equipment damage.
>
> - Only proceed if you have experience with mains-voltage installations
> - **Always disconnect power** before working on the circuit
> - Use a residual-current device (RCD/FI) on the controller circuit
> - If unsure, hire a qualified electrician
> - This is a DIY project — not CE/UL certified for commercial use

## Introduction

This guide walks you through building the Smart Swimming Pool Controller from
scratch — no prior knowledge of this project required. You will go from an empty
desk to a fully operational pool controller integrated with Home Assistant.

> **Total cost**: under 100€ (excluding pumps and pool infrastructure)\
> **Skill level**: beginner — basic soldering and ability to follow instructions\
> **Time**: 2–4 hours

---

## Hardware Variants

The controller supports ESP32 DevKit V1 with the following relay configurations:

| Variant          | Relay Module                           | Max. Load        | Use Case               |
| ---------------- | -------------------------------------- | ---------------- | ---------------------- |
| **Basic**        | 2-channel 5V relay module (active-low) | 10A per channel  | Pool pump + solar pump |
| **NORVI AE01-R** | Integrated ESP32 + relays              | 10A              | All-in-one industrial  |
| **Custom**       | Any 3.3V/5V relay via GPIO             | Depends on relay | Advanced setups        |

This guide focuses on the **Basic** variant. See the [Hardware Guide](/docs/hardware-guide/) for
NORVI and custom configurations.

---

## Bill of Materials (BOM)

### Required Components

| Qty | Component                                 | Approx. Cost | Notes                                |
| --- | ----------------------------------------- | ------------ | ------------------------------------ |
| 1   | ESP32 DevKit V1 (ESP32-WROOM-32)          | 8–12 €       | Any ESP32 dev board works            |
| 1   | 2-channel 5V relay module (active-low)    | 4–6 €        | HL-52S, SRD-05VDC-SL-C or equivalent |
| 1   | DS18B20 temperature sensor (pool)         | 3–5 €        | Stainless steel probe, 3m cable      |
| 1   | DS18B20 temperature sensor (solar)        | 3–5 €        | Stainless steel probe, 3m cable      |
| 1   | 4.7 kΩ resistor                           | 0,10 €       | Pull-up for DS18B20 data line        |
| 1   | Breadboard + jumper wires                 | 5 €          | For prototyping                      |
| 1   | Micro-USB cable + 5V power supply         | 5 €          | At least 1A                          |
| 1   | 230V AC relay module (if not using above) | 10 €         | Only if relays don't switch mains    |
| 1   | Project enclosure (IP65)                  | 10–15 €      | Weatherproof box                     |

### Optional Components

| Qty | Component                         | Purpose                        |
| --- | --------------------------------- | ------------------------------ |
| 1   | DHT22 temperature/humidity sensor | Ambient temperature monitoring |
| 1   | 5V buzzer                         | Audible alerts                 |
| 1   | LED + 220Ω resistor               | Visual status indicator        |
| 1   | 12V → 5V DC-DC converter          | Power from 12V pool system     |

### Tools Required

- Soldering iron + solder
- Wire stripper / cutter
- Multimeter
- Small screwdriver (for relay terminals)
- USB cable (data + power)

---

## Step 1: Prepare the Development Environment

### Install PlatformIO

PlatformIO is the build system used for the Pool Controller firmware.

```bash
# Option A: Install PlatformIO Core (recommended)
python3 -m pip install platformio

# Option B: Use VS Code with PlatformIO extension
# Install VS Code, then install PlatformIO extension from marketplace
```

Verify the installation:

```bash
pio --version
# Should print: PlatformIO Core, version 6.x or higher
```

### Install Git

```bash
# Linux (Debian/Ubuntu)
sudo apt install git

# macOS
brew install git

# Windows
# Download from https://git-scm.com/
```

### Clone the Repository

```bash
git clone https://github.com/smart-swimmingpool/pool-controller.git
cd pool-controller
```

---

## Step 2: Flash the Firmware

### Build the Firmware

```bash
pio run
```

This compiles the firmware. On first run, PlatformIO downloads all required
libraries automatically.

### Connect the ESP32

1. Connect the ESP32 DevKit to your computer via USB
2. Determine the serial port:
   - Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
   - macOS: `/dev/cu.usbserial-*`
   - Windows: `COM3` or similar

### Upload Firmware

```bash
# Upload firmware to connected ESP32
pio run --target upload --upload-port /dev/ttyUSB0

# Or let PlatformIO auto-detect:
pio run --target upload
```

### Verify Upload

```bash
# Monitor serial output
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

You should see boot messages. The controller starts in **AP mode** (access point)
if no WiFi is configured.

---

## Step 3: Breadboard Assembly (Test Setup)

Before soldering or installing permanently, build the circuit on a breadboard.

### Wiring Diagram

```text
ESP32 DevKit V1          DS18B20 Sensors
┌─────────────┐
│ GPIO15 ─────┼────┬──── Data (solar)
│ GPIO16 ─────┼────┬──── Data (pool)
│ 3.3V  ─────┼────┼──── VCC (both sensors)
│ GND   ─────┼────┼──── GND (both sensors)
│             │    │
│             │   4.7kΩ
│             │    │
│             └────┴──────────────── 3.3V
│
│ GPIO18 ───── IN1  (relay module)
│ GPIO19 ───── IN2  (relay module)
│ 5V    ───── VCC  (relay module)
│ GND   ───── GND  (relay module)
└─────────────┘
```

> **Important**: The DS18B20 data line requires a **4.7 kΩ pull-up resistor**
> between DATA and VCC. Without it, temperature reads -127 °C.

### Connect the Relay Module

Active-low relay modules trigger when the GPIO goes LOW. The firmware
defaults to **active-low** (use `relay-invert = false` in configuration if
your module is active-low).

If your relay module is **active-high**, set `relay-invert = true` in the
configuration (see Step 5).

---

## Step 4: First Startup — WiFi Configuration

### Connect to AP Mode

1. Power the ESP32 via USB
2. Wait 10 seconds for boot
3. Look for WiFi network **`Pool-Controller-Setup`**
4. Connect to it (no password)

### Configure WiFi

1. Open a browser and go to **`http://192.168.4.1`**
2. Go to the **WiFi Setup** tab
3. Select your home WiFi network (or use the scan button)
4. Enter the WiFi password
5. Click **Save & Reconnect**

The controller reboots and connects to your home network.

### Find the Device IP

Check your router's DHCP client list or use a network scanner. The device
is named `pool-controller` (mDNS: `pool-controller.local`).

Once connected, access the web interface at `http://<device-ip>`.

---

## Step 5: MQTT Configuration

### Prerequisites

- A running MQTT broker (Mosquitto, Home Assistant add-on, etc.)
- MQTT credentials (if authentication is enabled)

### Configure via Web UI

1. Open the web interface (`http://<device-ip>`)
2. Go to the **MQTT Settings** tab
3. Enter:
   - **Broker**: IP or hostname of your MQTT broker
   - **Port**: 1883 (default) or 8883 (TLS)
   - **Username**: (optional, if authentication is enabled)
   - **Password**: (optional)
   - **Protocol**: `homeassistant` (recommended)
4. Click **Save & Reconnect**

The controller reboots and connects to MQTT.

### Verify MQTT Connection

Use MQTT Explorer or a command-line tool:

```bash
# Subscribe to all pool-controller topics
mosquitto_sub -h <broker-ip> -t "pool-controller/#" -v
```

You should see temperature readings and status messages.

---

## Step 6: Sensor Verification

### Check Temperature Readings

In the web interface **Dashboard** tab, verify:

- **Pool Temperature** should show the ambient/water temperature
- **Solar Temperature** should show the collector temperature

If a sensor reads **-127 °C**, the sensor is not detected:

1. Check the 4.7 kΩ pull-up resistor
2. Verify wiring (VCC, GND, DATA)
3. Check for cold solder joints

### DHT22 (Optional)

If you connected a DHT22 sensor, verify the humidity reading on the dashboard.

---

## Step 7: Relay Test (No-Load)

**IMPORTANT**: Do NOT connect 230V AC loads during this test. Verify relay
operation with a multimeter or a 5V LED + resistor.

### Manual Relay Test

1. Open the web interface Dashboard
2. Toggle **Pool Pump** switch ON
3. Listen for the relay click — you should hear it engage
4. Measure continuity across relay COM/NO terminals with a multimeter
5. Toggle OFF — relay should disengage with a click
6. Repeat for **Solar Pump**

### Expected Behavior

| Relay State    | COM–NO | COM–NC |
| -------------- | ------ | ------ |
| OFF (inactive) | Open   | Closed |
| ON (active)    | Closed | Open   |

If the relay behavior is **inverted** (ON = COM–NC closed), set
`relay-invert = true` in Configuration → Advanced.

---

## Step 8: Home Assistant Integration

The controller uses **MQTT Discovery** to register all entities automatically.

### Prerequisites

- MQTT configured and working (Step 5)
- Home Assistant with MQTT integration configured
- `mqtt-protocol = "homeassistant"` in controller settings

### Auto-Discovery

After the controller connects to MQTT:

1. Open Home Assistant
2. Go to **Settings → Devices & Services**
3. Click **MQTT** → the pool controller should appear automatically
4. You will find entities for:
   - Pool and solar temperature sensors
   - Operation mode selector
   - Pool pump and solar pump switches
   - Timer configuration
   - Temperature thresholds

### Lovelace Dashboard

A pre-built dashboard is available in `docs/home-assistant/dashboard.yaml`.

See [Home Assistant Integration](/docs/home-assistant/) for detailed setup.

---

## Step 9: Electrical Commissioning

### Safety Checklist

Before connecting 230V AC:

- [ ] DS18B20 sensors return plausible temperatures (not -127 °C)
- [ ] Both relays click when toggled from web UI
- [ ] Relay wiring matches: COM → mains, NO → pump
- [ ] Enclosure is closed and sealed
- [ ] Power supply is rated for the load
- [ ] All connections are strain-relieved

### Connect Mains Voltage

1. **DISCONNECT POWER** at the circuit breaker
2. Wire the relay output:
   - **COM** → Live (230V AC)
   - **NO** → Pump Live wire
   - **Neutral** → directly to pump (not through relay)
   - **Ground** → protective earth
3. Secure all connections in a junction box
4. Restore power

### Verify Pump Operation

1. Open web interface Dashboard
2. Toggle pool pump ON
3. Verify pump runs
4. Toggle pool pump OFF
5. Verify pump stops
6. Repeat for solar pump

> **First commissioning tip**: Stay near the controller for the first 30 minutes
> of operation. Monitor temperatures and relay states.

---

## Step 10: Production Setup

### Enclosure Assembly

1. Mount the ESP32 + relay board inside the IP65 enclosure
2. Drill cable glands for sensor and power cables
3. Use strain relief on all external cables
4. Keep high-voltage (230V) and low-voltage (5V/3.3V) wiring separated

### Wiring Best Practices

- Use ferrules on stranded wires for screw terminals
- Label all cables
- Keep DS18B20 cable runs under 10m
- Use shielded cable for sensor runs in noisy environments

### Final Verification

See the [Production Checklist](/docs/production-checklist/) for a complete
commissioning checklist.

---

## Maintenance

### Regular Checks (Monthly)

- [ ] Verify temperature readings are plausible
- [ ] Check web interface for warnings
- [ ] Verify MQTT connection is active
- [ ] Check ESP32 free heap (should be > 20 KB)

### Firmware Updates

```bash
# Via web interface
# 1. Download latest firmware from GitHub Releases
# 2. Web UI → Security & Update → OTA Update
# 3. Select firmware binary and upload

# Via PlatformIO (USB)
pio run --target upload
```

See [OTA Updates](/docs/ota-updates/) for detailed instructions.

### Sensor Replacement

If a DS18B20 fails:

1. Power down the controller
2. Replace the sensor
3. Power up
4. Verify the reading returns to normal

### When Things Go Wrong

See [Troubleshooting](/docs/troubleshooting/) for common issues and solutions.

---

## Related Documents

- [Hardware Guide](/docs/hardware-guide/) — Detailed hardware assembly
- [Software Guide](/docs/software-guide/) — Development environment + API
- [MQTT Configuration](/docs/mqtt-configuration/) — MQTT setup details
- [Electrical Safety](/docs/electrical-safety/) — Electrical safety information
- [Safety Model](/docs/safety-model/) — System safety architecture
- [Production Checklist](/docs/production-checklist/) — Pre-deployment checklist
- [Security Checklist](/docs/security-checklist/) — Security hardening
- [Troubleshooting](/docs/troubleshooting/) — Common issues and fixes
