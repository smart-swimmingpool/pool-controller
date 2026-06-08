---
title: MQTT Configuration
summary: Home Assistant MQTT Discovery configuration, entity reference table, and migration from Homie for the ESP32 Pool Controller
date: "2026-06-07"
lastmod: "2026-06-07"
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

The controller publishes the following entities via MQTT Discovery:

| Function                       | HA component/object-id   | State topic                                                  | Command topic                                             |
| ------------------------------ | ------------------------ | ------------------------------------------------------------ | --------------------------------------------------------- |
| Solar temperature              | `sensor/solar-temp`      | `homeassistant/sensor/pool-controller/solar-temp/state`      | -                                                         |
| Pool temperature               | `sensor/pool-temp`       | `homeassistant/sensor/pool-controller/pool-temp/state`       | -                                                         |
| Controller temperature (ESP32) | `sensor/controller-temp` | `homeassistant/sensor/pool-controller/controller-temp/state` | -                                                         |
| Pool pump relay                | `switch/pool-pump`       | `homeassistant/switch/pool-controller/pool-pump/state`       | `homeassistant/switch/pool-controller/pool-pump/set`      |
| Solar pump relay               | `switch/solar-pump`      | `homeassistant/switch/pool-controller/solar-pump/state`      | `homeassistant/switch/pool-controller/solar-pump/set`     |
| Operation mode                 | `select/mode`            | `homeassistant/select/pool-controller/mode/state`            | `homeassistant/select/pool-controller/mode/set`           |
| Pool max temp                  | `number/pool-max-temp`   | `homeassistant/number/pool-controller/pool-max-temp/state`   | `homeassistant/number/pool-controller/pool-max-temp/set`  |
| Solar min temp                 | `number/solar-min-temp`  | `homeassistant/number/pool-controller/solar-min-temp/state`  | `homeassistant/number/pool-controller/solar-min-temp/set` |
| Hysteresis                     | `number/hysteresis`      | `homeassistant/number/pool-controller/hysteresis/state`      | `homeassistant/number/pool-controller/hysteresis/set`     |
| Timer start time               | `text/timer-start`       | `homeassistant/text/pool-controller/timer-start/state`       | `homeassistant/text/pool-controller/timer-start/set`      |
| Timer end time                 | `text/timer-end`         | `homeassistant/text/pool-controller/timer-end/state`         | `homeassistant/text/pool-controller/timer-end/set`        |
| Timezone index                 | `number/timezone`        | `homeassistant/number/pool-controller/timezone/state`        | `homeassistant/number/pool-controller/timezone/set`       |
| Timezone info                  | `sensor/timezone-info`   | `homeassistant/sensor/pool-controller/timezone-info/state`   | -                                                         |
| Log output                     | `sensor/log`             | `homeassistant/sensor/pool-controller/log/state`             | -                                                         |
| Log level                      | `select/log-level`       | `homeassistant/select/pool-controller/log-level/state`       | `homeassistant/select/pool-controller/log-level/set`      |
| Log to serial                  | `switch/log-serial`      | `homeassistant/switch/pool-controller/log-serial/state`      | `homeassistant/switch/pool-controller/log-serial/set`     |
| OTA update trigger             | `button/ota-update`      | -                                                            | `homeassistant/button/pool-controller/ota-update/set`     |
| OTA status                     | `sensor/ota-status`      | `homeassistant/sensor/pool-controller/ota-status/state`      | -                                                         |
| Effective Runtime              | `sensor/effective-runtime` | `homeassistant/sensor/pool-controller/effective-runtime/state` | -                                                       |
| Circ. Temp Threshold           | `number/temp-circ-threshold` | `homeassistant/number/pool-controller/temp-circ-threshold/state` | `homeassistant/number/pool-controller/temp-circ-threshold/set` |
| Circ. Temp Factor              | `number/temp-circ-factor` | `homeassistant/number/pool-controller/temp-circ-factor/state`   | `homeassistant/number/pool-controller/temp-circ-factor/set`   |
| Circ. Max Runtime              | `number/temp-circ-max-runtime` | `homeassistant/number/pool-controller/temp-circ-max-runtime/state` | `homeassistant/number/pool-controller/temp-circ-max-runtime/set` |

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
