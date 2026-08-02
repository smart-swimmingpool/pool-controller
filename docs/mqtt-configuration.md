---
title: MQTT Configuration
summary: Home Assistant MQTT Discovery configuration, entity reference table, and migration from Homie for the ESP32 Pool Controller
date: "2026-06-07"
lastmod: "2026-07-31"
draft: false
toc: true
type: docs
tags: ["docs", "mqtt", "home-assistant", "discovery", "configuration"]
menu:
  docs:
    parent: Pool Controller
    name: MQTT Configuration
    weight: 45
---

# MQTT Configuration (Home Assistant Discovery)

The Pool Controller uses [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
for automatic device registration. All devices appear automatically in Home Assistant
without any manual configuration.

> **Note**: The former Homie protocol support was removed in v3.3.0. The controller
> exclusively uses Home Assistant MQTT Discovery.

## Configuration

Configure your MQTT broker connection via the Web UI (Settings → MQTT tab):

- **MQTT Hostname/IP**: Your MQTT broker address
- **MQTT Port**: Default 1883
- **MQTT Username/Password**: Optional authentication credentials

After saving and rebooting, all entities appear automatically in Home Assistant.

### Via the REST API

The MQTT settings can also be configured programmatically:

```bash
curl -b "session=<session>" -X POST \
  -d "type=mqtt&host=192.168.1.100&port=1883&username=mqtt_user&password=secret" \
  http://<controller-ip>/api/config
```

## Home Assistant Entities

The controller publishes the following entities via MQTT Discovery, grouped by type with their standard HA entity category:

| Category     | Display in HA                           |
| ------------ | --------------------------------------- |
| _(none)_     | Main device page — primary measurements |
| `control`    | **Controls** section                    |
| `config`     | **Configuration** section               |
| `diagnostic` | **Diagnostics** section                 |

### Sensors (read-only)

| Function                       | HA component/object-id             | Entity Category | State topic                                                            |
| ------------------------------ | ---------------------------------- | --------------- | ---------------------------------------------------------------------- |
| Solar temperature              | `sensor/solar-temp`                | —               | `homeassistant/sensor/pool-controller/solar-temp/state`                |
| Pool temperature               | `sensor/pool-temp`                 | —               | `homeassistant/sensor/pool-controller/pool-temp/state`                 |
| Controller temperature (ESP32) | `sensor/controller-temp`           | `diagnostic`    | `homeassistant/sensor/pool-controller/controller-temp/state`           |
| Free Heap Space                | `sensor/heap`                      | `diagnostic`    | `homeassistant/sensor/pool-controller/heap/state`                      |
| Max Alloc Block                | `sensor/max-alloc`                 | `diagnostic`    | `homeassistant/sensor/pool-controller/max-alloc/state`                 |
| WiFi Signal Strength           | `sensor/rssi`                      | `diagnostic`    | `homeassistant/sensor/pool-controller/rssi/state`                      |
| System Uptime                  | `sensor/uptime`                    | `diagnostic`    | `homeassistant/sensor/pool-controller/uptime/state`                    |
| Local Time                     | `sensor/local-time`                | `diagnostic`    | `homeassistant/sensor/pool-controller/local-time/state`                |
| Timezone info                  | `sensor/timezone-info`             | `diagnostic`    | `homeassistant/sensor/pool-controller/timezone-info/state`             |
| Log output                     | `sensor/log`                       | `diagnostic`    | `homeassistant/sensor/pool-controller/log/state`                       |
| OTA status                     | `sensor/ota-status`                | `diagnostic`    | `homeassistant/sensor/pool-controller/ota-status/state`                |
| Effective Runtime              | `sensor/effective-runtime`         | `diagnostic`    | `homeassistant/sensor/pool-controller/effective-runtime/state`         |
| Solar Sensor Found             | `binary_sensor/solar-sensor-found` | `diagnostic`    | `homeassistant/binary_sensor/pool-controller/solar-sensor-found/state` |
| Pool Sensor Found              | `binary_sensor/pool-sensor-found`  | `diagnostic`    | `homeassistant/binary_sensor/pool-controller/pool-sensor-found/state`  |

### Controls

| Function         | HA component/object-id | Entity Category | Command topic                                         |
| ---------------- | ---------------------- | --------------- | ----------------------------------------------------- |
| Pool pump relay  | `switch/pool-pump`     | `control`       | `homeassistant/switch/pool-controller/pool-pump/set`  |
| Solar pump relay | `switch/solar-pump`    | `control`       | `homeassistant/switch/pool-controller/solar-pump/set` |
| Operation mode   | `select/mode`          | `control`       | `homeassistant/select/pool-controller/mode/set`       |

### Configuration (settable via HA)

| Function             | HA component/object-id         | Entity Category | Command topic                                                    |
| -------------------- | ------------------------------ | --------------- | ---------------------------------------------------------------- |
| Pool max temp        | `number/pool-max-temp`         | `config`        | `homeassistant/number/pool-controller/pool-max-temp/set`         |
| Solar min temp       | `number/solar-min-temp`        | `config`        | `homeassistant/number/pool-controller/solar-min-temp/set`        |
| Hysteresis           | `number/hysteresis`            | `config`        | `homeassistant/number/pool-controller/hysteresis/set`            |
| Circ. temp threshold | `number/temp-circ-threshold`   | `config`        | `homeassistant/number/pool-controller/temp-circ-threshold/set`   |
| Circ. temp factor    | `number/temp-circ-factor`      | `config`        | `homeassistant/number/pool-controller/temp-circ-factor/set`      |
| Circ. max runtime    | `number/temp-circ-max-runtime` | `config`        | `homeassistant/number/pool-controller/temp-circ-max-runtime/set` |
| Timer start time     | `time/timer-start`             | `config`        | `homeassistant/time/pool-controller/timer-start/set`             |
| Timer end time       | `time/timer-end`               | `config`        | `homeassistant/time/pool-controller/timer-end/set`               |
| Timezone             | `select/timezone`              | `config`        | `homeassistant/select/pool-controller/timezone/set`              |
| NTP Server           | `text/ntp-server`              | `config`        | `homeassistant/text/pool-controller/ntp-server/set`              |
| Log level            | `select/log-level`             | `config`        | `homeassistant/select/pool-controller/log-level/set`             |
| Log to serial        | `switch/log-serial`            | `config`        | `homeassistant/switch/pool-controller/log-serial/set`            |
| Solar Sensor mapping | `select/solar-sensor`          | `config`        | `homeassistant/select/pool-controller/solar-sensor/set`          |
| Pool Sensor mapping  | `select/pool-sensor`           | `config`        | `homeassistant/select/pool-controller/pool-sensor/set`           |

### Buttons & Diagnostics

| Function           | HA component/object-id   | Entity Category | Command topic                                              |
| ------------------ | ------------------------ | --------------- | ---------------------------------------------------------- |
| OTA update trigger | `button/ota-update`      | `config`        | `homeassistant/button/pool-controller/ota-update/set`      |
| Firmware           | `update/firmware-update` | `config`        | `homeassistant/update/pool-controller/firmware-update/set` |
| Pool Thermostat    | `climate/thermostat`     | `config`        | (mode + temperature via climate topics)                    |

### Events (Log stream)

The controller publishes **curated log events** as an HA `event` entity (for
automations and notifications) plus a **raw log stream** for external tools:

| Function           | HA component/object-id | Entity Category | Topic                                                                  |
| ------------------ | ---------------------- | --------------- | ---------------------------------------------------------------------- |
| Curated log events | `event/logs`           | `diagnostic`    | `homeassistant/event/pool-controller/logs/state` (discovery `.../config`) |

- **Discovery topic**: `homeassistant/event/pool-controller/logs/config`
  (`"platform": "event"`) with `event_types`:
  `LOG_WARN`, `LOG_ERROR`, `MODE_CHANGED`, `PUMP_ON`, `PUMP_OFF`,
  `WIFI_CONNECTED`, `WIFI_DISCONNECTED`, `MQTT_CONNECTED`, `MQTT_DISCONNECTED`
- **State payload** (published on change only, deduplicated by log sequence):
  ```json
  {"event_type": "MODE_CHANGED", "message": "Mode switched to auto"}
  ```
- **Raw stream** `pool-controller/log` (JSON Lines, WARN/ERROR only):
  ```json
  {"seq": 42, "t": 123456, "level": "warning", "msg": "..."}
  ```

### Entity Category Reference

| Category     | Description                                          |
| ------------ | ---------------------------------------------------- |
| _(none)_     | Primary measurements, shown on the device front page |
| `control`    | Primary controls (switches, mode selection)          |
| `config`     | Configuration values and settings                    |
| `diagnostic` | Device diagnostics and system information            |

## Features

All entities support:

- Temperature sensors (pool, solar, controller)
- Relay switches (pool pump, solar pump)
- Operation modes (auto, manual, boost, timer)
- Temperature-based circulation parameters (threshold, factor, max runtime)
- Configuration via MQTT
- State monitoring

## Home Assistant Dashboard Example

A ready-to-use Lovelace dashboard example with:

- quick mode switching (`auto`, `timer`, `boost`, `manu`)
- highlighted pool and solar/storage temperatures
- a dedicated configuration view
- a combined history chart for switching times and temperatures

is available in:

- [`docs/home-assistant-dashboard-pool.yaml`](home-assistant-dashboard-pool.yaml)

## Migration from Homie (Pre-v3.3.0)

If you're upgrading from an older firmware that used the Homie protocol:

1. The **Homie protocol has been removed** in v3.3.0 — the controller now uses
   **Home Assistant MQTT Discovery exclusively**
2. After flashing the new firmware, **delete stale retained Homie messages**
   from your MQTT broker to avoid confusion:
   ```bash
   mosquitto_pub -h broker -t homie -n -r
   mosquitto_sub -t 'homie/#' -v | cut -d' ' -f1 | xargs -I{} mosquitto_pub -h broker -t {} -n -r
   ```
3. In Home Assistant, delete the old device entry (Settings → Devices & services →
   MQTT → Devices → pool-controller → Delete) — the new entities will appear
   automatically via MQTT Discovery
4. Reboot the device to trigger fresh discovery announcements
