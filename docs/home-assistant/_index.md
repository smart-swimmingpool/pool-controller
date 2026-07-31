---
title: Home Assistant Integration
summary: Pool Controller Home Assistant integration — automatic MQTT Discovery entities, sensor/switch/number/select/time domains, Lovelace dashboard YAML, migration from legacy configs
date: "2026-06-06"
lastmod: "2026-07-31"
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
| `binary_sensor` | `mqtt_connected` | diagnostic | MQTT connection status |
| `select` | `operation_mode` | — | Operating mode (auto/manu/boost/timer) |
| `select` | `pool_sensor` | config | Pool sensor address mapping |
| `select` | `solar_sensor` | config | Solar sensor address mapping |
| `select` | `timezone` | config | Timezone selection |
| `switch` | `pool_pump` | — | Pool circulation pump |
| `switch` | `solar_pump` | — | Solar heating pump |
| `number` | `maximum_pool_temperature` | config | Maximum pool temperature target |
| `number` | `minimum_solar_temperature` | config | Minimum solar activation temperature |
| `number` | `temperature_hysteresis` | config | Temperature hysteresis value |
| `number` | `circulation_temperature_threshold` | config | Temperature-based circulation threshold |
| `number` | `circulation_temperature_factor` | config | Temperature-based circulation factor |
| `number` | `circulation_maximum_runtime` | config | Temperature-based circulation max runtime |
| `time` | `timer_start` | config | Timer start time (HH:MM) |
| `time` | `timer_end` | config | Timer end time (HH:MM) |
| `text` | `ntp_server` | config | NTP server address |
| `update` | `firmware` | config | Firmware update entity |
| `climate` | `pool_thermostat` | config | Pool thermostat (HVAC mode + target temp) |
| `event` | `logs` | diagnostic | Log event stream (MQTT event entity, see [Log Events](#log-events)) |

> **Entity IDs** in HA are generated from the MQTT discovery `name` field. The entity_id will be
> `sensor.<device_prefix>_pool_temperature` etc. — where `<device_prefix>` is typically
> `pool_controller` (from the device name). Check **Developer Tools → Entities** and filter by
> "pool" to find your actual IDs. Replace `pool_controller` in the dashboard YAML with your
> device prefix if it differs.

### Log Events

The controller exposes an [MQTT event entity](https://www.home-assistant.io/integrations/event.mqtt/)
(`event.pool_controller_logs` — object ID `logs`) that updates its state whenever a log-worthy
event occurs: mode changes, pump on/off, WiFi/MQTT connectivity, and warning/error log entries.
See [MQTT Configuration → Events (Log stream)](../mqtt-configuration.md#events-log-stream) for the
full topic and payload reference.

The event entity keeps the last event type in its `event_type` attribute and merges every
additional payload field as an attribute (e.g. `message`).

#### Logbook Automation

Write every event to the HA logbook:

```yaml
automation:
  - alias: "Pool Controller — log events to logbook"
    triggers:
      - trigger: event.received
        target:
          entity_id: event.pool_controller_logs
        options:
          event_type:
            - MODE_CHANGED
            - PUMP_ON
            - PUMP_OFF
            - WIFI_CONNECTED
            - WIFI_DISCONNECTED
            - MQTT_CONNECTED
            - MQTT_DISCONNECTED
            - LOG_WARN
            - LOG_ERROR
    actions:
      - action: logbook.log
        data:
          name: "Pool Controller"
          message: "{{ state_attr('event.pool_controller_logs', 'event_type') }}"
          entity_id: event.pool_controller_logs
```

The `event.received` trigger (Home Assistant 2026.7+) fires when the entity receives a matching
event type; see the [event received trigger documentation](https://www.home-assistant.io/triggers/event.received/).
Trim the `event_type` list to the events you care about, and adjust the entity ID to your device
prefix if it differs (Developer Tools → Entities, filter by "pool").

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
