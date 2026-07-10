---
title: Home Assistant Integration
summary: Pool Controller Home Assistant integration — automatic MQTT Discovery entities, sensor/switch/number/select/time domains, Lovelace dashboard YAML, migration from legacy configs
date: "2026-06-06"
lastmod: "2026-06-06"
draft: false
toc: true
type: docs
menu:
  docs:
    parent: Pool Controller
    name: Home Assistant
    weight: 50
---

The Pool Controller integrates seamlessly with [Home Assistant](https://home-assistant.io) via **MQTT Discovery**
(default protocol). All entities are registered automatically when the controller connects to your MQTT broker.

## MQTT Discovery

The controller publishes [HA MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
payloads on startup and on MQTT reconnect. No manual MQTT configuration is needed — devices appear in Home
Assistant automatically.

### Available Entities

Entity IDs are derived from the `name` field in each discovery payload (e.g. name `"Pool Temperature"`
produces `sensor.<prefix>_pool_temperature`). Replace `<prefix>` with your device prefix
(see warning below).

| Domain | Object ID | Category | Description |
|--------|-----------|----------|-------------|
| `sensor` | `pool_temperature` | — | Pool water temperature |
| `sensor` | `solar_temperature` | — | Solar collector temperature |
| `sensor` | `controller_temperature` | diagnostic | ESP32 chip temperature |
| `sensor` | `free_heap_space` | diagnostic | Free heap memory |
| `sensor` | `max_alloc_block` | diagnostic | Largest allocatable block |
| `sensor` | `wifi_signal_strength` | diagnostic | WiFi signal strength (dBm) |
| `sensor` | `system_uptime` | diagnostic | Device uptime (duration) |
| `sensor` | `effective_runtime` | diagnostic | Effective circulation runtime (duration) |
| `sensor` | `local_time` | diagnostic | Current local time |
| `binary_sensor` | `pool_sensor_found` | diagnostic | Pool sensor detection status |
| `binary_sensor` | `solar_sensor_found` | diagnostic | Solar sensor detection status |
| `binary_sensor` | `mqtt_status` | diagnostic | MQTT connection status |
| `select` | `operation_mode` | — | Operating mode (auto/manu/boost/timer) |
| `select` | `pool_sensor` | config | Pool sensor address mapping |
| `select` | `solar_sensor` | config | Solar sensor address mapping |
| `select` | `timezone` | config | Timezone selection |
| `switch` | `pool_pump` | — | Pool circulation pump |
| `switch` | `solar_pump` | — | Solar heating pump |
| `number` | `max_pool_temp` | config | Maximum pool temperature target |
| `number` | `min_solar_temp` | config | Minimum solar activation temperature |
| `number` | `temperature_hysteresis` | config | Temperature hysteresis value |
| `number` | `circ_temp_threshold` | config | Temperature-based circulation threshold |
| `number` | `circ_temp_factor` | config | Temperature-based circulation factor |
| `number` | `circ_max_runtime` | config | Temperature-based circulation max runtime |
| `time` | `timer_start` | config | Timer start time (HH:MM) |
| `time` | `timer_end` | config | Timer end time (HH:MM) |
| `text` | `ntp_server` | config | NTP server address |
| `update` | `firmware` | config | Firmware update entity |
| `climate` | `pool_thermostat` | config | Pool thermostat (HVAC mode + target temp) |

> **Entity IDs** in HA are generated from the MQTT discovery `name` field. The entity_id will be
> `sensor.<device_prefix>_pool_temperature` etc. — where `<device_prefix>` is typically
> `pool_controller` (from the device name). Check **Developer Tools → Entities** and filter by
> "pool" to find your actual IDs. Replace `pool_controller` in the dashboard YAML with your
> device prefix if it differs.

## Lovelace Dashboard

A pre-built Lovelace dashboard configuration is provided in [`dashboard.yaml`](dashboard.yaml).

### Features

Two audience-specific views:

- **🏊 Pool view** — for the pool operator (daily operation): Temperature gauges, mode switching (with active-mode highlighting), timer, pump control, climate thermostat, temperature-based circulation settings, 24h history with controller temperature
- **⚙ System view** — for the IoT developer (diagnostics & configuration): Timezone & NTP, sensor mapping (DS18B20 address selection), system diagnostics (heap, WiFi, uptime, controller temperature, effective runtime), firmware updates

### Setup

1. Copy the YAML file to your HA configuration directory
2. Open HA → **Dashboard → Edit → Three-dot menu → Raw Configuration Editor**
3. Paste the content
4. Adjust entity IDs to match your installation (see warning below)
5. Save

### Dependencies

The mode buttons use [`button-card`](https://github.com/custom-cards/button-card) (custom card) to visually
highlight the active operation mode:

```text
HACS → Frontend → button-card → Install
```

Without `button-card`, replace `custom:button-card` with `type: button` in the grid cards (loses
active-mode highlighting).

### Entity ID Warning

Entity IDs depend on the MQTT discovery configuration — the dashboard uses `pool_controller` as the
device prefix. If your IDs use a different prefix, replace all occurrences in the YAML. Check your
actual entity IDs in **Developer Tools → Entities** (filter by "pool").

## Migration from earlier versions

If upgrading from a previous firmware version, old discovery configs may remain in the MQTT broker. The
controller automatically publishes empty retained configs for obsolete entities
(`number/timer-start-h/min`, `number/timer-end-h/min`, `number/timezone`). After the first MQTT reconnect,
Home Assistant removes the orphaned entities automatically.
