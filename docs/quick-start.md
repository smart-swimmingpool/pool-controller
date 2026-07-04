---
title: Quick Start Guide
summary: Step-by-step guide to set up your Smart Swimming Pool Controller from scratch. Perfect for beginners with basic electronics knowledge.
type: docs
weight: 10
---

**Target audience:** DIY enthusiasts with **basic electronics knowledge** (soldering, GPIO, MQTT).
**Time required:** ~4\u20136 hours (including parts sourcing).
**Cost:** ~45\u201375\u20ac (excluding pumps and pool infrastructure).

This guide walks you through **every step** to get your Pool Controller up and running, from ordering parts to integrating with your smart home.

---

## ⚠️ Safety First

> **⚠️ WARNING: This project involves 230V AC mains voltage!**
>
> - **Only proceed if you have basic electronics knowledge.**
> - **Always use a Residual Current Device (RCD/FI circuit breaker) for the pump circuit.**
> - **Disconnect power before working on the circuit.**
> - **Keep low-voltage (sensor) wiring separate from mains wiring.**
> - **If in doubt, consult a qualified electrician.**
> - **This project is NOT certified (no CE/UL mark). For personal use only!**

**Important Safety Resources:**
- [Electrical Safety Guide](safety.md)
- [Safety Model](safety-model.md)
- [Security Checklist](security-checklist.md)

---

## \ud83d\udce6 Step 1: Order Parts

### \ud83d\uded2 Shopping List (BOM)

Use the following table to order all required components.

**Recommended shops:** Amazon, AliExpress, Reichelt, Pollin, Conrad (DE/AT/CH).

| # | Component | Qty | Approx. Cost | Notes | Recommended Links |
|---|-----------|:---:|:------------:|-------|------------------|
| 1 | ESP32 Development Board (e.g., ESP32 DevKit V1, NodeMCU-32S) | 1 | 10\u201315\u20ac | **Must have 4MB+ flash** | [Amazon DE](https://www.amazon.de/s?k=ESP32+DevKit+V1), [Reichelt](https://www.reichelt.de/) |
| 2 | DS18B20 Temperature Sensor (waterproof, stainless steel, 1m cable) | 2 | 8\u201312\u20ac | One for pool, one for solar collector | [Amazon DE](https://www.amazon.de/s?k=DS18B20+wasserfest), [AliExpress](https://www.aliexpress.com/) |
| 3 | 2-Channel 5V Relay Module (with optocoupler isolation) | 1 | 5\u20138\u20ac | **Must be active-high trigger!** (relay activates when GPIO signal is HIGH) | [Amazon DE](https://www.amazon.de/s?k=2+Channel+5V+Relay+Module), [Reichelt](https://www.reichelt.de/) |
| 4 | Resistor 4.7k\u03a9 (\u00bcW, metal film) | 2 | < 1\u20ac | Pull-up for OneWire data lines | [Reichelt](https://www.reichelt.de/), [Conrad](https://www.conrad.de/) |
| 5 | Breadboard + jumper wires (for prototyping) **OR** Perfboard + pin headers (for permanent build) | 1 | 3\u20138\u20ac | Breadboard for testing, perfboard for final build | [Amazon DE](https://www.amazon.de/s?k=breadboard), [Reichelt](https://www.reichelt.de/) |
| 6 | USB Power Supply 5V/\u22651A (e.g., phone charger) | 1 | 5\u201310\u20ac | Powers ESP32 + relay module | Any USB charger |
| 7 | Hookup wire (0.14\u20130.5mm², various colors) | — | 3\u20135\u20ac | For permanent wiring | [Reichelt](https://www.reichelt.de/) |
| 8 | Enclosure (IP54+ for outdoor use) | 1 | 5\u201310\u20ac | Optional but recommended | [Amazon DE](https://www.amazon.de/s?k=IP54+Geh\u00e4use), [Reichelt](https://www.reichelt.de/) |
| 9 | Screw terminals (2-pin, 5mm pitch) | 4\u20136 | 2\u20133\u20ac | For removable connections | [Reichelt](https://www.reichelt.de/) |
| **Total** | | | **~45\u201375\u20ac** | Without pumps/pool infrastructure | |

> **ℹ Note:** The relay module **must** be **active-high** (relay ON when GPIO = HIGH). Many cheap modules are active-low by default but have a jumper to switch the logic. Verify this before permanent installation!

### \ud83d\udd0c ESP32 Board Selection

**Recommended boards:**
- **ESP32 DevKit V1** — Most common, widely available
- **NodeMCU-32S** — Similar to DevKit, good compatibility
- **ESP32-WROOM-32** — Any board with this module

**Avoid:**
- ESP32-S2, ESP32-S3, ESP32-C3 — Different architecture, not currently supported
- Boards with < 4MB flash — Insufficient for firmware

**Verification:** Check that your board has the **ESP32-WROOM-32** module and **4MB+ flash memory**.

### \ud83c\udf10 Relay Module Selection

**Critical:** The relay module **must** have **optocoupler isolation** for safety and **active-high logic** for compatibility with the firmware.

**How to verify active-high logic:**
1. Connect relay VCC to 5V and GND to GND
2. Connect IN1 to 3.3V (from ESP32)
3. Measure between COM1 and NO1 — should show continuity when IN1 = HIGH

**Recommended modules:**
- **Songle SRD-05VDC-SL-C** — Active-high, optocoupler isolated
- **Any module labeled "High Level Trigger" or "Active High"**

> **⚠️ Important:** If your module is active-low, you can either:
> 1. Find a module with a jumper to switch between active-high/active-low
> 2. Modify the firmware to invert the relay logic (change `RELAY_ON` from `HIGH` to `LOW` in `Config.hpp`)

---

## \ud83d\udd0c Step 2: Assemble Hardware

### \ud83d\udccc Required Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Soldering iron | Soldering components | 320\u2013350\u00b0C for leaded solder |
| Solder | Electrical connections | Leaded recommended for beginners |
| Flux | Improve solder flow | Rosin-core flux |
| Wire cutters/strippers | Cut and strip wires | For hookup wire |
| Multimeter | Test continuity and voltage | Essential for debugging |
| Magnifying glass | Inspect solder joints | For quality control |
| Third hand tool | Hold components | Optional but helpful |

### 🪉 Option A: Breadboard Prototyping (Recommended for Beginners)

**Best for:** Testing before permanent installation, learning the circuit.

#### Wiring Diagram

```text
ESP32 Development Board
   
   3.3V --[4.7kΩ]-- GPIO32 -- DS18B20 Solar (DATA)
   3.3V --[4.7kΩ]-- GPIO33 -- DS18B20 Pool (DATA)
   
   GPIO25 -- Relay IN1 (Pool Pump)
   GPIO26 -- Relay IN2 (Solar Pump)
   
   VIN (5V) -- Relay VCC
   GND -- Relay GND
   GND -- DS18B20 GND (both sensors)
   
   USB Power (5V) -- ESP32 VIN
   USB Power (GND) -- ESP32 GND
```

#### Step-by-Step Assembly

1. **Place ESP32 on breadboard**
   - Straddle the center gap of the breadboard
   - Ensure all pins are properly inserted

2. **Add pull-up resistors**
   - Insert 4.7k\u0019 resistors between 3.3V rail and GPIO32, GPIO33
   - These are required for DS18B20 sensor communication

3. **Connect DS18B20 sensors**
   - **Red wire (VDD)** → 3.3V rail
   - **Black wire (GND)** → GND rail
   - **Yellow/White wire (DATA)** → GPIO32 (Solar) and GPIO33 (Pool)
   - **Important:** Each sensor needs its own pull-up resistor

4. **Connect relay module**
   - **VCC** → 5V rail (from USB power)
   - **GND** → GND rail
   - **IN1** → GPIO25 (Pool pump control)
   - **IN2** → GPIO26 (Solar pump control)

5. **Connect power**
   - **USB Power 5V** → ESP32 VIN pin
   - **USB Power GND** → ESP32 GND pin

6. **Verify connections**
   - Use multimeter to check continuity
   - Ensure no short circuits
   - Verify all connections are secure

> **ℹ Tip:** Use different colored jumper wires for different signals to avoid confusion.

### 🪉 Option B: Permanent Installation (Perfboard)

**Best for:** Final installation, outdoor use, long-term operation.

#### Recommended Layout

1. **Plan component placement** on perfboard
2. **Solder ESP32** with pin headers for easy removal
3. **Solder relay module** with sufficient spacing
4. **Add screw terminals** for:
   - Power input (5V/GND)
   - Sensor connections (DATA/GND for each DS18B20)
   - Relay outputs (COM/NO/NC for each relay)
5. **Solder resistors** directly between 3.3V and GPIO pins
6. **Add test points** for debugging with multimeter

#### Soldering Tips

- **Tin your iron** before starting
- **Use flux** for better solder flow
- **Heat both pad and wire** before applying solder
- **Avoid cold solder joints** — should be shiny, not dull
- **Check for bridges** between pins with magnifying glass
- **Test continuity** with multimeter after soldering

> **ℹ Tip:** Use **sleeve or heatshrink tubing** on wire connections for insulation and strain relief.

---

## \u26a1 Step 3: Connect to Pumps and Sensors

### \u26a1 Relay Wiring (230V AC)

> **⚠️ WARNING: Mains voltage! Disconnect power before wiring!**

**Relay terminals:**
- **COM** — Common (input from mains)
- **NO** — Normally Open (output to pump when relay is ON)
- **NC** — Normally Closed (output to pump when relay is OFF)

**For each pump (Pool and Solar):**

```text
Mains L (Phase) --[ Fuse ]--[ RCD/FI Circuit Breaker ]-- Relay COM
Mains N (Neutral) -------------- Relay NO
Relay NO ----- Pump L (Phase)
Relay COM ----- Mains L (Phase)
```

**Important:**
- Use **RCD/FI circuit breaker** (30mA) for each pump circuit
- Use **appropriate wire gauge** for pump current (typically 1.5mm² for pumps up to 2kW)
- Keep **low-voltage wiring (ESP32, sensors) physically separate** from mains wiring
- Use **cable glands** for outdoor installations

### \ud83c\udf21️ DS18B20 Sensor Installation

**Pool Sensor:**
- Place sensor in pool water, **away from direct sunlight**
- Use **waterproof cable gland** for entry into enclosure
- Ensure sensor is **fully submerged** (at least 10cm below surface)
- Avoid placing near **pump intakes** or **heaters**

**Solar Collector Sensor:**
- Place sensor at **highest point** of solar collector
- Use **heat-resistant cable** if collector gets very hot
- Ensure good **thermal contact** with collector pipe
- Protect from **direct weather exposure** if possible

**Cable Routing:**
- Use **outdoor-rated cable** for external installations
- Keep cable runs **as short as possible** (DS18B20 works up to ~100m with proper pull-ups)
- Use **cable ties** for strain relief
- Avoid **sharp bends** or **pinching** cables

---

## ☁ Step 4: Flash Firmware

### 📁 Prerequisites

1. **Install PlatformIO**
   - **VS Code Extension:** [PlatformIO IDE](https://platformio.org/install/ide?install=vscode)
   - **CLI:** `pip install platformio`

2. **Install USB drivers** (if needed)
   - **CP210x:** [Silicon Labs Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
   - **CH340:** [WCH Driver](http://www.wch-ic.com/downloads/CH341SER_ZIP.html)

3. **Clone repository**
   ```bash
   git clone https://github.com/smart-swimmingpool/pool-controller.git
   cd pool-controller
   ```

### \u26a1 Connect ESP32 to Computer

1. Connect ESP32 to computer via **USB cable** (data-capable, not charge-only)
2. Check that device is detected:
   - **Windows:** Device Manager → Ports (COM & LPT)
   - **Linux:** `ls /dev/tty*` (look for `/dev/ttyUSB0` or similar)
   - **macOS:** `/dev/cu.*` or `/dev/tty.*`

3. **Note the serial port** for the next step

### \u26a1 Build and Flash

#### Using PlatformIO CLI
```bash
# Navigate to project directory
cd pool-controller

# Build the firmware (downloads dependencies on first run)
pio run

# Flash to device (replace COM3 with your port)
pio run --target upload --upload-port COM3

# Monitor serial output
pio run --target monitor --upload-port COM3
```

#### Using PlatformIO VS Code Extension
1. Open the project folder in VS Code
2. Click PlatformIO icon in activity bar
3. Select your environment (e.g., `esp32dev`)
4. Click **▶ Build** (checkmark icon)
5. Click **↩ Upload** (arrow icon)
6. Click **→ Monitor** (plug icon) to view serial output

### ✅ Verify Successful Flash

After flashing, you should see in the serial monitor:

```text


 20:12:34.567 [I] PoolController: Starting up...
20:12:34.568 [I] PoolController: Homie for ESP8266/ESP32 v2.0.0
20:12:34.569 [I] PoolController: Firmware version: 3.3.0
20:12:34.570 [I] PoolController: Connecting to WiFi...
```

If the device **doesn't start** or shows **garbled output**:
- Check USB cable (try a different one)
- Check serial port selection
- Check baud rate (should be 115200)
- Try pressing **BOOT button** while powering on

---

## ☁ Step 5: Initial Configuration

### ☁ Method A: Web Interface (Recommended)

1. **Wait for WiFi AP mode**
   - On first boot, the device creates a WiFi hotspot: **`Pool-Controller-Setup`**
   - Password: **not required** (open network)

2. **Connect to hotspot**
   - On your phone/computer, connect to `Pool-Controller-Setup`
   - Open browser to: **`http://192.168.4.1`**

3. **Configure WiFi**
   - Enter your **WiFi SSID** and **password**
   - Click **Save & Connect**

4. **Configure MQTT**
   - Enter your **MQTT broker hostname/IP**
   - Enter **MQTT port** (default: 1883)
   - Enter **MQTT username/password** (if required)
   - Click **Save & Restart**

5. **Device reboots** and connects to your WiFi and MQTT broker

### ☁ Method B: Serial Monitor

1. **Open serial monitor** (baud rate: 115200)
2. **Wait for configuration prompts** or send commands:

```text
# Set WiFi credentials
AT+WIFI=SSID,password

# Set MQTT broker
AT+MQTT=broker.example.com,1883

# Set MQTT credentials (if needed)
AT+MQTTUSER=username,password

# Save and restart
AT+SAVE
AT+RESTART
```

> **Note:** Serial commands are only available in **AP mode** (when not connected to WiFi).

---

## ☁ Step 6: Connect to Smart Home

### ☁ Home Assistant (Recommended)

1. **Ensure MQTT broker is running**
   - Install [Mosquitto Add-on](https://www.home-assistant.io/addons/mosquitto/) or use external broker

2. **Enable MQTT Discovery**
   - In Home Assistant: **Settings → Devices & Services → MQTT**
   - Ensure **Discovery** is enabled

3. **Pool Controller auto-discovers**
   - After connecting to MQTT, devices appear automatically in Home Assistant
   - Go to: **Settings → Devices & Services**
   - Look for **Pool Controller** devices

4. **Add to Dashboard**
   - Create a new dashboard or add entities to existing one
   - Recommended entities:
     - Temperature sensors (Pool, Solar)
     - Switches (Pool Pump, Solar Pump)
     - Select (Operation Mode)
     - Number (Temperature thresholds)

**Complete MQTT Guide:** [MQTT Configuration](mqtt-configuration.md)

### ☁ openHAB

1. **Install MQTT Binding**
   - In Paper UI: **Add-ons → Bindings → MQTT Binding**

2. **Configure MQTT Broker**
   - Edit `services/mqtt.cfg` with your broker settings

3. **Add Pool Controller Items**
   - See [openHAB Configuration](https://github.com/smart-swimmingpool/openhab-config) for example items and sitemaps

### ☁ Node-RED

1. **Install Node-RED**
2. **Add MQTT Input Nodes** for Pool Controller topics
3. **Add Dashboard Nodes** for visualization
4. **Deploy flow**

---

## ✅ Step 7: Test and Verify

### ✅ Check Basic Functionality

1. **Verify WiFi connection**
   - Check serial monitor for: `WiFi connected, IP: 192.168.x.x`
   - Or check your router's client list

2. **Verify MQTT connection**
   - Check serial monitor for: `MQTT connected`
   - Subscribe to `#` topic with MQTT client to see all messages

3. **Verify temperature readings**
   - Check serial monitor for temperature updates
   - Or check MQTT topics: `smart-swimmingpool/pool-controller/temperature/pool`

4. **Test relay control**
   - Use Home Assistant/openHAB to turn pumps on/off
   - Listen for relay clicking sounds
   - Verify pumps start/stop

### ✅ Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| Device doesn't start | Power issue | Check USB cable, power supply, connections |
| No WiFi connection | Wrong credentials | Reconfigure via AP mode or serial |
| No MQTT connection | Wrong broker settings | Verify broker IP/port, check firewall |
| No temperature readings | Sensor wiring issue | Check DS18B20 connections, pull-up resistors |
| Relays don't switch | Wiring issue | Check relay connections, verify active-high logic |
| Pumps don't start | Mains wiring issue | Check relay wiring, RCD, pump power |

**Complete Troubleshooting:** [FAQ](faq.md) and [Troubleshooting Guide](troubleshooting.md)

---

## \ud83c\udf00 Step 8: Final Installation

### \ud83c\udf00 Enclosure

1. **Choose enclosure**
   - **IP54+ rating** for outdoor use
   - **Sufficient size** for ESP32, relay module, and wiring
   - **Ventilation** for heat dissipation (if not waterproof)

2. **Mount components**
   - Use **standoffs** for ESP32 and relay module
   - Use **cable glands** for external connections
   - Keep **mains wiring separate** from low-voltage wiring

3. **Label connections**
   - Label all screw terminals
   - Create a **wiring diagram** for future reference

### \ud83c\udf00 Power Supply

1. **Choose power supply**
   - **USB phone charger** (5V/\u22651A) for indoor use
   - **Outdoor-rated power supply** for external installation
   - **USB power bank** for temporary setup

2. **Power protection**
   - Use **surge protector** for outdoor installations
   - Consider **UPS** for power failure protection

### \ud83c\udf00 Location

1. **Indoor installation** (recommended)
   - Protect from weather
   - Easy access for maintenance
   - Within WiFi range

2. **Outdoor installation**
   - Use **IP65+ enclosure**
   - Use **outdoor-rated cable**
   - Protect from **direct sunlight** and **rain**
   - Ensure **adequate ventilation**

---

## \ud83d\udc89 Next Steps

Now that your Pool Controller is up and running, explore these advanced features:

### \u26a1 Automation

- **Set up automation rules** in Home Assistant/openHAB
- **Temperature-based control:** Turn on solar heating when pool is cold
- **Time-based control:** Run circulation pump during off-peak hours
- **Weather-based control:** Disable solar heating on cloudy days

**Example Home Assistant Automation:**
```yaml
automation:
  - alias: "Enable solar heating when pool is cold"
    trigger:
      - platform: numeric_state
        entity_id: sensor.pool_controller_pool_temp
        below: 25
    condition:
      - condition: state
        entity_id: switch.pool_controller_solar_pump
        state: "off"
      - condition: numeric_state
        entity_id: sensor.pool_controller_solar_temp
        above: 30
    action:
      - service: switch.turn_on
        entity_id: switch.pool_controller_solar_pump
```

### \u26a1 Monitoring

- **Set up Grafana dashboard** for historical data
- **Configure alerts** for abnormal conditions
- **Monitor energy usage** of pumps

**Grafana Dashboard:** [smart-swimmingpool/grafana-dashboard](https://github.com/smart-swimmingpool/grafana-dashboard)

### \u26a1 Advanced Configuration

- **Adjust temperature thresholds** for your climate
- **Configure circulation schedules** based on pool usage
- **Set up multiple operation modes** (Auto, Manual, Boost, Timer)
- **Enable state persistence** for power failure recovery

**Complete Configuration Guide:** [Users Guide](users-guide.md)

---

## \ud83d\ude4f Tips for Success

### \u26a1 Best Practices

1. **Start with breadboard prototyping** before permanent installation
2. **Test each component individually** before connecting everything
3. **Use multimeter** to verify all connections before powering on
4. **Label all wires** for easy troubleshooting
5. **Keep a wiring diagram** for future reference
6. **Test in AP mode first** before configuring WiFi/MQTT
7. **Monitor serial output** during initial setup

### \u26a1 Common Pitfalls

1. **Active-low vs active-high relays** — Verify your relay logic!
2. **Missing pull-up resistors** — DS18B20 sensors won't work without them
3. **Insufficient power supply** — Use 5V/\u22651A for ESP32 + relay
4. **WiFi range issues** — Ensure good signal at installation location
5. **MQTT broker not running** — Verify broker is accessible from ESP32
6. **Mains wiring errors** — Always use RCD and verify with electrician

### \u26a1 Maintenance

1. **Regularly check connections** for corrosion or loose wires
2. **Clean DS18B20 sensors** periodically (especially pool sensor)
3. **Update firmware** when new versions are released
4. **Monitor system health** via MQTT or web interface
5. **Backup configuration** before making changes

---

## \ud83d\udce2 Need Help?

If you encounter issues:

1. **Check this guide again** — Most issues are covered above
2. **Read the [FAQ](faq.md)** — Common problems and solutions
3. **Search [GitHub Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)** — Community support
4. **Open a [GitHub Issue](https://github.com/smart-swimmingpool/pool-controller/issues/new)** — Include:
   - Detailed description of the problem
   - Screenshots (serial monitor, web interface)
   - Your hardware setup (ESP32 model, relay module, sensors)
   - Firmware version (from web interface or serial monitor)
   - Steps to reproduce the issue

---

## \ud83d\udcbb Additional Resources

### \u26a1 Documentation

- [Hardware Guide](hardware-guide.md) — Detailed assembly instructions
- [Users Guide](users-guide.md) — Operation modes and web interface
- [MQTT Configuration](mqtt-configuration.md) — Complete topic reference
- [State Persistence](state-persistence.md) — Settings survival across reboots
- [OTA Updates](ota-updates.md) — Remote firmware updates
- [Software Guide](software-guide.md) — Development and build process

### \u26a1 External Resources

- [PlatformIO Documentation](https://docs.platformio.org/) — Build system
- [ESP32 Datasheet](https://www.espressif.com/en/products/socs/esp32) — Microcontroller
- [DS18B20 Datasheet](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) — Temperature sensor
- [MQTT Protocol](https://mqtt.org/) — Message Queuing Telemetry Transport
- [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) — Smart home integration
- [Electrical Safety](https://www.electricalsafetyfirst.org.uk/) — General safety guidelines

### \u26a1 Community

- [GitHub Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions) — Ask questions, share ideas
- [Home Assistant Community](https://community.home-assistant.io/) — Smart home discussions
- [Reddit r/homeassistant](https://www.reddit.com/r/homeassistant/) — Home automation community

---

<p align="center">
  ✅ Congratulations! You now have a smart swimming pool! ✅\n  \n  Made with ❤️ by the Smart Swimming Pool community
</p>
