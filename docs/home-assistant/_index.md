---
title: Home Assistant Integration
summary: Pool Controller Home Assistant integration \u2014 automatic MQTT Discovery entities, sensor/switch/number/select/time domains, Lovelace dashboard YAML, migration from legacy configs
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
payloads on startup and on MQTT reconnect. No manual MQTT configuration is needed \u2014 devices appear in Home
Assistant automatically.

### Available Entities

| Domain          | Object ID                | Category   | Description                                  |
| --------------- | ------------------------ | ---------- | -------------------------------------------- |
| `sensor`        | `pool_temperature`       | \u2014     | Pool water temperature                       |
| `sensor`        | `solar_temperature`      | \u2014     | Solar collector temperature                  |
| `sensor`        | `controller_temperature` | diagnostic | ESP32 chip temperature                       |
| `sensor`        | `free_heap_space`        | diagnostic | Free heap memory                             |
| `sensor`        | `max_alloc_block`        | diagnostic | Largest allocatable block                    |
| `sensor`        | `wifi_signal_strength`   | diagnostic | WiFi signal strength (dBm)                   |
| `sensor`        | `system_uptime`          | diagnostic | Device uptime (duration)                     |
| `sensor`        | `effective_runtime`      | diagnostic | Effective circulation runtime (duration)     |
| `sensor`        | `local_time`             | diagnostic | Current local time                           |
| `binary_sensor` | `pool_sensor_found`      | diagnostic | Pool sensor detection status                 |
| `binary_sensor` | `solar_sensor_found`     | diagnostic | Solar sensor detection status                |
| `binary_sensor` | `mqtt_status`            | diagnostic | MQTT connection status                       |
| `select`        | `mode`                   | \u2014     | Operating mode (auto/manu/boost/timer)       |
| `select`        | `pool_sensor`            | config     | Pool sensor address mapping                  |
| `select`        | `solar_sensor`           | config     | Solar sensor address mapping                 |
| `select`        | `timezone`               | config     | Timezone selection                           |
| `switch`        | `pool_pump`              | \u2014     | Pool circulation pump                        |
| `switch`        | `solar_pump`             | \u2014     | Solar circulation pump                       |
| `number`        | `pool_max_temp`          | config     | Maximum pool temperature (\u00b0C)           |
| `number`        | `solar_min_temp`         | config     | Minimum solar temperature (\u00b0C)          |
| `number`        | `hysteresis`             | config     | Temperature hysteresis (K)                   |
| `number`        | `temp_circ_threshold`    | config     | Circulation temperature threshold (\u00b0C)  |
| `number`        | `temp_circ_factor`       | config     | Circulation temperature factor (min/\u00b0C) |
| `number`        | `temp_circ_max_runtime`  | config     | Circulation maximum runtime (min)            |
| `time`          | `timer_start`            | config     | Timer start time (HH:MM:SS)                  |
| `time`          | `timer_end`              | config     | Timer end time (HH:MM:SS)                    |
| `text`          | `ntp_server`             | config     | NTP server address                           |
| `update`        | `firmware`               | \u2014     | Firmware update entity                       |
| `climate`       | `pool_climate`           | \u2014     | Pool temperature control with preset modes   |

### Entity Details

#### Temperature Sensors

All temperature sensors report values in \u00b0C with 1 decimal precision.

- **`sensor.pool_temperature`**: Current pool water temperature. Used for automation and climate control.
- **`sensor.solar_temperature`**: Current solar collector temperature. Used to determine if solar heating is beneficial.
- **`sensor.controller_temperature`**: ESP32 chip temperature for device monitoring.

#### Binary Sensors

- **`binary_sensor.pool_sensor_found`**: Indicates if the pool temperature sensor is detected (Found/Missing). Uses `connectivity` device class for green/red visualization in HA.
- **`binary_sensor.solar_sensor_found`**: Indicates if the solar temperature sensor is detected (Found/Missing). Uses `connectivity` device class for green/red visualization in HA.
- **`binary_sensor.mqtt_status`**: Indicates MQTT connection status (ON/OFF). Uses `connectivity` device class.

#### Switches

- **`switch.pool_pump`**: Controls the pool circulation pump. Device class: `outlet`.
- **`switch.solar_pump`**: Controls the solar circulation pump. Device class: `outlet`.

#### Number Entities

All number entities support configuration changes via Home Assistant UI:

- **`number.pool_max_temp`**: Maximum desired pool temperature (0.0\u201440.0\u00b0C, step 0.1).
- **`number.solar_min_temp`**: Minimum solar temperature for heating (0.0\u2014100.0\u00b0C, step 0.1).
- **`number.hysteresis`**: Temperature difference required to turn pumps off (0.0\u201410.0 K, step 0.1).
- **`number.temp_circ_threshold`**: Temperature threshold for circulation (0.0\u201440.0\u00b0C, step 0.5).
- **`number.temp_circ_factor`**: Circulation factor based on temperature (0.0\u2014120.0 min/\u00b0C, step 5.0).
- **`number.temp_circ_max_runtime`**: Maximum circulation runtime (60.0\u20141440.0 min, step 15.0).

#### Select Entities

- **`select.mode`**: Operation mode selection. Options: `auto`, `manu` (manual), `boost`, `timer`.
- **`select.pool_sensor`**: Pool sensor address mapping. Options are dynamically populated from detected 1-Wire devices.
- **`select.solar_sensor`**: Solar sensor address mapping. Options are dynamically populated from detected 1-Wire devices.
- **`select.timezone`**: Timezone selection for local time display.

#### Time Entities

- **`time.timer_start`**: Start time for timer-based circulation (HH:MM:SS format).
- **`time.timer_end`**: End time for timer-based circulation (HH:MM:SS format).

#### Text Entities

- **`text.ntp_server`**: NTP server address for time synchronization.

#### Update Entity

- **`update.firmware`**: Firmware update entity for OTA updates.

#### Climate Entity

- **`climate.pool_climate`**: Thermostat-style climate control with the following features:
  - **Current Temperature**: Pool water temperature
  - **Target Temperature**: Configurable via `number.pool_max_temp`
  - **Temperature Unit**: \u00b0C
  - **Preset Modes**: Maps to operation modes
    - `none` \u2192 `auto`
    - `manual` \u2192 `manu`
    - `schedule` \u2192 `timer`
    - `boost` \u2192 `boost`

### State Classes

All sensor entities now include appropriate `state_class` for Home Assistant history statistics:

- **`measurement`**: Temperature sensors, heap, max_alloc_block, wifi_signal_strength, effective_runtime
- **`total_increasing`**: System uptime

This enables long-term statistics and energy monitoring in Home Assistant.

### Device Classes

- **`connectivity`**: binary_sensor.pool_sensor_found, binary_sensor.solar_sensor_found, binary_sensor.mqtt_status
- **`signal_strength`**: sensor.wifi_signal_strength (RSSI in dBm)
- **`temperature`**: All temperature sensors
- **`outlet`**: pool_pump, solar_pump switches

### Migration from Legacy Configs

If you have existing MQTT configurations for `sensor.pool_sensor_found` or `sensor.solar_sensor_found` (which were text sensors with "Found"/"Missing" states), these will be automatically cleaned up on MQTT connect. The new entities use the `binary_sensor` domain with proper device class for better visualization.

Old config topics:

- `homeassistant/sensor/pool-controller/pool-sensor-found/config`
- `homeassistant/sensor/pool-controller/solar-sensor-found/config`

These topics will be removed when the controller connects to MQTT.

### Lovelace Dashboard

A ready-to-use Lovelace dashboard YAML is available in [dashboard.yaml](dashboard.yaml). Import it via:

1. Home Assistant \u2192 Settings \u2192 Dashboards \u2192 Add Dashboard
2. Select "Import" and paste the YAML content
3. Customize cards as needed

The dashboard includes:

- Temperature overview cards
- Pump control cards
- Mode selection
- Configuration parameters
- System diagnostics

### Troubleshooting

#### Entities Not Appearing

1. **Check MQTT Connection**: Verify the controller is connected to your MQTT broker.
2. **Check Discovery Topics**: Look for messages on `homeassistant/#` topics.
3. **Restart Home Assistant**: Sometimes HA needs a restart to pick up new discovery messages.
4. **Check Logs**: The controller logs MQTT discovery messages on startup.

#### Device Not Available

- Verify the MQTT broker is running and accessible
- Check the controller's serial output for connection errors
- Ensure the controller has network connectivity

#### Temperature Readings Incorrect

- Verify sensor wiring (1-Wire bus for DS18B20 sensors)
- Check for electrical interference on long sensor cables
- Ensure proper pull-up resistors are installed

## Configuration

No configuration is required for MQTT Discovery. The controller automatically:

- Generates a unique device identifier based on MAC address
- Publishes discovery messages for all entities on startup
- Re-publishes discovery messages on MQTT reconnect
- Updates entity states periodically

### Customizing Entity IDs

To customize entity IDs, modify the `objectId` parameters in the `publishDiscovery()` method and update the corresponding state publishing calls.

### MQTT Topics

All entities follow the Home Assistant MQTT Discovery convention:

```
homeassistant/<component>/pool-controller/<object_id>/config
homeassistant/<component>/pool-controller/<object_id>/state
```

For commands (switches, numbers, selects):

```
homeassistant/<component>/pool-controller/<object_id>/set
```

### Availability

All entities use the same availability topic:

```
homeassistant/sensor/pool-controller/availability
```

This topic publishes `online` when the controller is connected and `offline` when disconnected.
