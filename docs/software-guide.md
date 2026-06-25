---
title: Software Guide
summary: Software development guide for the Pool Controller — PlatformIO build environment, library dependencies, REST API reference, web interface, and code architecture overview
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
    name: Software Guide
    weight: 30
---

## Development Environment

## Required Libraries

- [AsyncMqttClient](https://github.com/marvinroger/async-mqtt-client) @ 0.9.0
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [OneWire](https://github.com/PaulStoffregen/OneWire)
- [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor)
- [DHT sensor library](https://github.com/adafruit/DHT-sensor-library)
- [NTPClient](https://github.com/arduino-libraries/NTPClient) @ 3.2.1
- [Timezone](https://github.com/JChristensen/Timezone) @ 1.2.6
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) @ 7.3.2
- [Bounce2](https://github.com/thomasfredericks/Bounce2)
- [Wire](https://github.com/espressif/arduino-esp32/tree/master/libraries/Wire)

Many thanks to the maintainers of these libraries!

## Pin Configuration

Within the sources at `Config.hpp`, the GPIO pin assignments are defined. For details,
see also the [hardware guide](../hardware-guide/#pin-assignment-firmware-defaults).

```cpp
constexpr uint8_t PIN_DS_SOLAR   = 15;  // Pin of Temp-Sensor Solar (GPIO15)
constexpr uint8_t PIN_DS_POOL    = 16;  // Pin of Temp-Sensor Pool (GPIO16)

constexpr uint8_t PIN_RELAY_POOL  = 18;  // Pin to control pool pump relay
constexpr uint8_t PIN_RELAY_SOLAR = 19;  // Pin to control solar pump relay

constexpr uint8_t TEMP_READ_INTERVAL = 30;
```

## Web Interface & Direct Access

The controller includes a **built-in web server on port 80** that provides a full
management dashboard. It runs in two modes:

| Mode                       | When                               | Access                                                         |
| -------------------------- | ---------------------------------- | -------------------------------------------------------------- |
| **AP Mode** (Access Point) | No WiFi configured (factory state) | SSID `Pool-Controller-Setup`, IP `192.168.4.1`, no password    |
| **STA Mode** (Station)     | Normal WiFi connection             | DHCP IP of the ESP32 in local network, password login required |

### API Endpoints

| Route                    | Auth   | Function                                                   |
| ------------------------ | ------ | ---------------------------------------------------------- |
| `GET /`                  | Cookie | Dashboard SPA (Single Page Application)                    |
| `GET /login`             | Cookie | Login page                                                 |
| `POST /api/login`        | -      | Issue session cookie (SHA-256 password check)              |
| `GET /api/status`        | ❌ No  | Live telemetry (temperatures, pump states, heap, RSSI, temperature thresholds)     |
| `GET /api/scan`          | Yes    | Scan nearby WiFi networks                                  |
| `GET /api/config`        | Yes    | Read current configuration                                 |
| `POST /api/config`       | Yes    | Save configuration (`type=settings\|wifi\|mqtt\|password`) |
| `GET /api/restart`       | Yes    | Reboot the ESP32                                           |
| `GET /api/factory_reset` | Yes    | Wipe config file, reboot into AP setup mode                |
| `POST /api/update`       | Yes    | OTA firmware update (signed .bin upload)                   |

### Using the REST API Directly

You can interact with the controller programmatically:

```bash
# Get live telemetry (no authentication needed)
curl http://<controller-ip>/api/status

# Get session cookie
SESSION=$(curl -s -c - -X POST -d "password=admin" \
  http://<controller-ip>/api/login | grep session | awk '{print $NF}')

# Read configuration
curl -b "session=$SESSION" http://<controller-ip>/api/config

# Write settings (they persist across reboots and notify Home Assistant)
curl -b "session=$SESSION" -X POST \
  -d "type=settings&mode=auto&max_pool=30.0&min_solar=55.0&hysteresis=1.0" \
  http://<controller-ip>/api/config
```

### Authentication

- In **AP mode** the web interface is unprotected (intentional for initial setup)
- In **STA mode**, a cookie-based session is required (15 minute timeout)
- Default password is `admin`
- Password is stored as SHA-256 hash in `/config.json`
- The dashboard always shows live temperatures and threshold values even after
  session expiry, because `/api/status` (unauthenticated) now includes
  `temp_max_pool` and `temp_min_solar`. Configuration writes still require
  a valid session.

## Configuration Persistence

Configuration is persisted in two independent storage systems, ensuring all
settings survive reboots and power failures:

### 1. ConfigManager — Device Configuration (LittleFS)

| File       | `/config.json`                                                      |
| ---------- | ------------------------------------------------------------------- |
| Max Size   | 4 KB                                                                |
| Contents   | WiFi, MQTT, NTP, ControllerSettings, admin password hash            |
| Management | `ConfigManager::load()` at boot, `ConfigManager::save()` on changes |
| Reset      | `ConfigManager::reset()` → factory defaults                         |

### 2. StateManager — Runtime State (ESP32 NVS / Preferences)

| Namespace | `pool-controller`                                                       |
| --------- | ----------------------------------------------------------------------- |
| Contents  | opmode, poolMaxTemp, solarMinTemp, hysteresis, timerStart/End           |
| API       | `StateManager::saveString/Float/Int/Bool` → type-safe key-value storage |

### Data Flow on Settings Changes

```text
Web UI / REST API            MQTT (Home Assistant)
        │                            │
        ▼                            ▼
────────┴────── ConfigManager ───────┴────
                save() → /config.json (LittleFS)
                ↓
        OperationModeNode
        (runtime parameters)
                ↓
        MqttPublisher::publishStates()
        → MQTT topics → Home Assistant
```

> **Note:** When settings are changed via the Web UI, Home Assistant is updated
> on the next measurement cycle (every `loopInterval` seconds, default 10s).
> Changes made via MQTT are confirmed immediately.

## MQTT Communication

The controller uses **Home Assistant MQTT Discovery** exclusively since v3.3.0.
See the dedicated documentation for details:

- **[MQTT Configuration](../mqtt-configuration)** — Protocol, entity reference, Homie migration
- **[Home Assistant Integration](../home-assistant)** — Lovelace dashboard, HA entity IDs

### Clearing retained messages

If you need to clear retained MQTT messages:

```bash
# Clear a specific Home Assistant topic
mosquitto_pub -h hostname -t "homeassistant/sensor/pool-controller/pool-temp/state" -n -r
```

## MQTT Discovery Configuration Persistence

Discovery configuration is persisted in two independent storage systems, ensuring all
settings survive reboots and power failures:

### 1. ConfigManager — Device Settings (LittleFS)

The controller stores WiFi, MQTT, NTP, and device settings in a JSON file on
LittleFS. See [`ConfigManager`](../state-persistence) for details.

### 2. Runtime State (ESP32 NVS / Preferences)

Operation mode, relay states, and temperature parameters are persisted in NVS
for immediate recovery after power loss.

### Data Flow on Settings Changes

```text
Web UI / REST API            MQTT (Home Assistant)
        │                            │
        ▼                            ▼
────────┴────── ConfigManager ───────┴────
                save() → /config.json (LittleFS)
                ↓
        OperationModeNode
        (runtime parameters)
                ↓
        MqttPublisher::publishStates()
        → MQTT topics → Home Assistant
```

> **Note:** When settings are changed via the Web UI, Home Assistant is updated
> on the next measurement cycle (every `loopInterval` seconds, default 10s).
> Changes made via MQTT are confirmed immediately.
