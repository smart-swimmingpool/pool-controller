---
title: Software Guide of Pool Controller
summary:
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

- [RelayModule](https://github.com/YuriiSalimov/RelayModule)
- [Vector](https://github.com/tomstewart89/Vector)
- DallasTemperature
- OneWire
- Adafruit Unified Sensor
- DHT sensor library
- NTPClient @ 3.2.1
- TimeZone @ 1.2.4
- ArduinoJson @ 7.3.0
- Bounce2
- PubSubClient @ 2.8
- Wire

Many thanks to maintainers of these libraries!

## Pin Configuration

Within the sources at `Config.hpp`, the GPIO pin assignments are defined. For details,
see also at [hardware guide](../hardware-guide/#esp32-pin-usage).

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
| `GET /api/status`        | ❌ No  | Live telemetry (temperatures, pump states, heap, RSSI)     |
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

### Protocol

The controller uses **Home Assistant MQTT Discovery** (default) with topic
structure:

```text
homeassistant/<component>/pool-controller/<object-id>/config  (discovery)
homeassistant/<component>/pool-controller/<object-id>/state   (state)
homeassistant/<component>/pool-controller/<object-id>/set     (command)
```

See `docs/mqtt-configuration.md` for a complete entity mapping.

### Clearing retained messages

If you need to clear retained MQTT messages:

```bash
# Clear a specific Home Assistant topic
mosquitto_pub -h hostname -t "homeassistant/sensor/pool-controller/pool-temp/state" -n -r

# Clear a Homie topic
mosquitto_pub -h hostname -t homie -n -r -d
```

## Configuration

Homie supports configuration (e.g. WiFi credentials) using JSON-files.
How to upload JSON config files see [Homie documentation](https://homieiot.github.io/homie-esp8266/docs/develop/configuration/json-configuration-file/).

### Example `config.json`

```json
{
  "name": "Pool Controller",
  "device_id": "pool-controller",
  "wifi": {
    "ssid": "<SSID>",
    "password": "<XXX>"
  },
  "mqtt": {
    "host": "<MQTT_HOST>",
    "port": 1883
  },
  "ota": {
    "enabled": true
  },
  "settings": {
    "loop-interval": 60,
    "temperature-max-pool": 28,
    "temperature-min-solar": 50,
    "temperature-hysteresis": 0.5
  }
}
```

<!-- (duplicate MQTT Communication heading removed — content already covered above) -->
