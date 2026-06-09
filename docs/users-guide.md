---
title: User Guide
summary: Pool Controller user guide — WiFi setup via AP mode and WPS, web dashboard walkthrough, all operation modes (Auto/Timer/Boost/Manual), MQTT Discovery, and openHAB integration
date: "2020-05-28"
lastmod: "2020-06-02"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "controller", "tutorial"]
menu:
  docs:
    parent: Pool Controller
    name: Users Guide
    weight: 40
---

## Setup

### Initial WiFi setup via Web Interface (AP Mode)

When the controller has no WiFi configured, it starts in **Access Point mode**:

1. Power on the controller
2. Connect your phone/laptop to WiFi SSID **`Pool-Controller-Setup`** (open network)
3. Open a browser and go to **`http://192.168.4.1`** — the captive portal redirects automatically
4. Go to the **WiFi Setup** tab, scan networks, select yours and enter the password
5. The controller saves the config and reboots into **STA mode**
6. Find the controller's IP in your router's DHCP list and reconnect

> **Note:** In AP mode, the web interface has no password (intentional for initial setup).
> Once connected to your WiFi, a login is required (default password: `admin`).

### Initial WiFi setup via WPS

You can trigger WPS onboarding during boot to pair the controller with your router
without typing WiFi credentials manually:

1. Power on (or reset) the controller normally and wait until firmware startup begins.
2. Then press and hold the **BOOT** button for at least 2 seconds.
3. Start **WPS Push Button Connect (PBC)** on your router.
4. Wait until pairing completes.

When WPS succeeds, the controller updates the WiFi credentials and stores them
persistently in `/config.json` on LittleFS.

### Finding the Controller on Your Network

Once connected, find the controller's IP:

- Check your router's DHCP client list
- Or use a network scanner like `nmap`:

  ```bash
  nmap -p 80 192.168.1.0/24  # scan your subnet for open port 80
  ```

- The web interface shows the assigned IP in the dashboard header

## OLED Display (NORVI AE01-R Variant)

The industrial NORVI AE01-R controller includes a **0.96" OLED display**
(128×64 pixels) on the front panel. When your firmware is built with the
`NORVI_AE01_R` flag, the display shows system information across four
switchable pages:

| Page | What You See |
|:----:|--------------|
| **1** | **Main** — Pool and solar water temperatures with pump ON/OFF indicators (filled/empty circle), operation mode (auto/manu/boost/timer), and shortcut hints for the three front buttons |
| **2** | **Network** — Connected WiFi SSID, IP address, MQTT connection status. In AP mode it shows the setup hotspot name and URL |
| **3** | **System** — Uptime (e.g. 3d 12h 30m), current and minimum free heap memory, firmware version |
| **4** | **QR Code** — Generates a scannable QR code of the web dashboard URL so you can open it on your phone without typing the IP address |

**Footer:** Each page has a bottom bar showing the local time (HH:MM, with
automatic DST handling), the firmware version (`vX.Y.Z`), and the current page
number. On page 1 the active operation mode is also shown in the footer.

**Auto WiFi page:** If no WiFi is configured yet (AP mode), the display jumps
directly to page 2 at startup so you can see the setup instructions immediately.

**QR Quick-Access:** Use your phone's camera app to scan the QR code on page 4
— most modern phones recognize the URL and offer to open it in a browser. No
extra app needed.

The display updates every 2 seconds. Navigate pages using the front-panel
buttons (see the [NORVI AE01-R documentation](norvi-ae01-r.md) for details).

## LED Status Codes (Homie Convention)

The controller uses its **built-in LED** to signal the current system state,
following the [Homie Convention](https://homieiot.github.io/) — an industry
standard for IoT device status indication.

| LED Pattern | When? | What it means |
|-------------|-------|---------------|
| ![WiFi connecting](led_wifi.gif) **Slow blink** (1x/sec) | **WiFi connecting** | The controller is trying to join your home WiFi network. This should take 5–20 seconds. |
| ![MQTT connecting](led_mqtt.gif) **Mostly on, brief blip off every 2s** | **WiFi OK, MQTT connecting/disconnected** | Network is up, but the MQTT broker is not yet connected. Check broker address or network. |
| **Rapid blink** (5x/sec) | **AP Mode / Setup** | No WiFi credentials stored. The controller is hosting its own `Pool-Controller-Setup` WiFi network. |
| **Solid on** (always on) | **Fully connected** | Everything is running normally — WiFi + MQTT connected. |
| **Very fast blink** (10x/sec) | **OTA Update in progress** | Firmware is being downloaded and flashed. Do not power off! |
| **Double blink** (two flashes, pause) | **Safe Mode / Error** | Boot-loop detected or critical system degradation. Relays are forced OFF. |

### What to expect at first power-on

1. **Power on** the controller
2. **Rapid blink** — AP mode (no WiFi configured yet)
3. Open the `Pool-Controller-Setup` network and configure your WiFi
4. **Slow blink** — connecting to your home WiFi
5. **Mostly on** — WiFi connected, MQTT connecting
6. **Solid on** — everything is running normally

> **Troubleshooting**: If the LED **never leaves rapid blink**, the controller
> has no WiFi credentials or cannot find your network. Connect to the
> `Pool-Controller-Setup` access point to configure it. If it stays in
> **double-blink**, a critical error has occurred — check the serial log.

## Web Dashboard

The controller provides a modern web dashboard at `http://<controller-ip>/` with the following tabs:

- **Dashboard** — Live telemetry: pool/solar/controller temperature, pump states, free heap, RSSI, operating mode
- **WiFi Setup** — Scan and configure WiFi credentials
- **MQTT Settings** — Broker host, port, username, password
- **Configuration** — Operation mode, temperature limits, hysteresis, timer, timezone
- **Security & Update** — Change admin password, OTA firmware update, restart, factory reset

### Changing Settings via Web UI

1. Login with your admin password (default: `admin`)
2. Navigate to the **Configuration** tab
3. Adjust settings as needed:

    - **Operation Mode**: Automatic (Solar), Manual Control, Boost Pump, Timer Schedule
    - **Max Pool Temp**: Target water temperature (°C)
    - **Min Solar Temp**: Minimum solar collector temperature (°C)
    - **Temperature Hysteresis**: Deadband to prevent rapid toggling (K)
    - **Loop Interval**: How often the controller evaluates rules (seconds)
    - **Timezone**: Select from 10 supported timezones with DST handling

4. Click **Save parameters** — changes are **immediately active** and **persist across reboots**
5. If MQTT/Home Assistant is connected, the new values are automatically published

> **Tip:** You can also change settings programmatically via the REST API — see the [Software Guide](software-guide.md#web-interface--direct-access).

## Settings

There are some specific settings for the controller:

- **Pool max. temperature:** The maximum temperature of the water in the pool which should not be exceeded.

  - Unit: `°C`
  - Default value: `29`

- **Solar min temperature:** The minimum temperature of the heat storage tank, which should not fall below.

  - Unit: `°C`
  - Default value: `50`

- **Hysteresis:** Hysteresis in Kelvin, which is used to verify if heating
  should be enabled or disabled to prevent fast toggling.

  - Unit: `K`
  - Default value: `1`

- **Pump Timer:** time range when pool pump has to run.

  - start h/min
  - end h/min

- **Loop Interval:**

  - Unit: `sec`
  - Default value: `30`

- **Timezone:** Select the timezone for the controller to properly handle local time and daylight saving time (DST) transitions.

  - Available timezones:
    - `0` - Central European Time (Berlin, Paris) with CEST/CET DST
    - `1` - Eastern European Time (Helsinki, Athens) with EEST/EET DST
    - `2` - Western European Time (London, Lisbon) with BST/GMT DST
    - `3` - US Eastern Time (New York, Washington) with EDT/EST DST
    - `4` - US Central Time (Chicago, Houston) with CDT/CST DST
    - `5` - US Mountain Time (Denver) with MDT/MST DST
    - `6` - US Pacific Time (Los Angeles, San Francisco) with PDT/PST DST
    - `7` - Australian Eastern Time (Sydney, Melbourne) with AEDT/AEST DST
    - `8` - Japan Time (Tokyo) - No DST
    - `9` - China Time (Beijing) - No DST
  - Default value: `0` (Central European Time)
  - This setting can be configured during initial setup or changed at runtime via MQTT
  - **Note:** Runtime changes via MQTT (operation-mode/timezone) are temporary.
    To persist the timezone setting across reboots, update the `timezone`
    configuration in the device settings.

## Rules

The **Smart Swimmingpool Controller** implements `Rules` to handle different situations:

### Rule: Manual

The pump for cleaning and solar heating are enabled/disabled completely manual and independent.

### Rule: Timer

This rule enables the cleaning pump based on timer settings. Solar heating is disabled.

### Rule: Auto

This rule enables the cleaning pump based on timer settings. Solar heating is
enabled **smartly** if the cleaning pump is enabled by timer and the heat
storage tank has sufficient temperature.

If the maximum temperature of the pool water is reached, the solar heating is disabled.

### Rule: Boost

Heating of pool water with all power.

## MQTT Interface

The **Smart Swimmingpool Controller** uses [MQTT](http://mqtt.org/) to
communicate with your smart home. It supports
[Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
for automatic device registration — devices appear automatically in Home Assistant
without any manual configuration.

## OpenHAB Integration

The **Smart Swimmingpool Controller** could be integrated in [openHAB](https://www.openhab.org) since version 2.4.

It is possible to interact with the controller to enable/disable the pump or to switch the current rule.

Also it is possible to monitor the current values of temperatures or states.

At least the [settings](#settings) could be updated, too.

- TODO: add example of openhab config.

### Device

### Properties
