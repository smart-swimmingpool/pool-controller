# MQTT Protocol Configuration

The Pool Controller now supports two MQTT protocols:

## 1. Home Assistant MQTT Discovery **(Default)**

[Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
allows automatic device discovery in Home Assistant.

To use Home Assistant (default):

```json
{
  "mqtt-protocol": "homeassistant"
}
```

## 2. Homie Convention (Legacy)

The [Homie Convention](https://homieiot.github.io/) provides a standardized
MQTT device discovery convention.

To use Homie:

```json
{
  "mqtt-protocol": "homie"
}
```

## Configuration

You can set the MQTT protocol in the Homie configuration UI or in the
`config.json` file:

### Via Homie UI

1. Connect to the device's WiFi AP during initial setup
2. Navigate to the configuration page
3. Set "mqtt-protocol" to either "homeassistant" (default) or "homie"
4. Save and reboot

### Via config.json

Add or modify the setting in your device's `config.json`:

```json
{
  "name": "Pool Controller",
  "settings": {
    "mqtt-protocol": "homeassistant"
  }
}
```

## Protocol Differences

### Homie Convention

- Topic structure: `homie/<device-id>/<node-id>/<property>`
- Example: `homie/pool-controller/pool-temp/temperature`
- Standardized device discovery
- Works with openHAB, Home Assistant (via Homie integration)

### Home Assistant MQTT Discovery

- Topic structure: `homeassistant/<component>/<device-id>/<object-id>/config`
- Example: `homeassistant/sensor/pool-controller/pool-temp/config`
- Native Home Assistant auto-discovery
- Optimized for Home Assistant

## Home Assistant Mapping

The table maps Homie properties to Home Assistant discovery objects.

| Function                       | Homie node/property                                   | HA component/object-id   | State topic                                                  | Command topic                                             |
| ------------------------------ | ----------------------------------------------------- | ------------------------ | ------------------------------------------------------------ | --------------------------------------------------------- |
| Solar temperature              | `homie/pool-controller/solar-temp/temperature`        | `sensor/solar-temp`      | `homeassistant/sensor/pool-controller/solar-temp/state`      | -                                                         |
| Pool temperature               | `homie/pool-controller/pool-temp/temperature`         | `sensor/pool-temp`       | `homeassistant/sensor/pool-controller/pool-temp/state`       | -                                                         |
| Controller temperature (ESP32) | `homie/pool-controller/controller-temp/temperature`   | `sensor/controller-temp` | `homeassistant/sensor/pool-controller/controller-temp/state` | -                                                         |
| Pool pump relay                | `homie/pool-controller/pool-pump/switch`              | `switch/pool-pump`       | `homeassistant/switch/pool-controller/pool-pump/state`       | `homeassistant/switch/pool-controller/pool-pump/set`      |
| Solar pump relay               | `homie/pool-controller/solar-pump/switch`             | `switch/solar-pump`      | `homeassistant/switch/pool-controller/solar-pump/state`      | `homeassistant/switch/pool-controller/solar-pump/set`     |
| Operation mode                 | `homie/pool-controller/operation-mode/mode`           | `select/mode`            | `homeassistant/select/pool-controller/mode/state`            | `homeassistant/select/pool-controller/mode/set`           |
| Pool max temp                  | `homie/pool-controller/operation-mode/pool-max-temp`  | `number/pool-max-temp`   | `homeassistant/number/pool-controller/pool-max-temp/state`   | `homeassistant/number/pool-controller/pool-max-temp/set`  |
| Solar min temp                 | `homie/pool-controller/operation-mode/solar-min-temp` | `number/solar-min-temp`  | `homeassistant/number/pool-controller/solar-min-temp/state`  | `homeassistant/number/pool-controller/solar-min-temp/set` |
| Hysteresis                     | `homie/pool-controller/operation-mode/hysteresis`     | `number/hysteresis`      | `homeassistant/number/pool-controller/hysteresis/state`      | `homeassistant/number/pool-controller/hysteresis/set`     |
| Timer start time               | `homie/pool-controller/operation-mode/timer-start`    | `text/timer-start`       | `homeassistant/text/pool-controller/timer-start/state`       | `homeassistant/text/pool-controller/timer-start/set`      |
| Timer end time                 | `homie/pool-controller/operation-mode/timer-end`      | `text/timer-end`         | `homeassistant/text/pool-controller/timer-end/state`         | `homeassistant/text/pool-controller/timer-end/set`        |
| Timezone index                 | `homie/pool-controller/operation-mode/timezone`       | `number/timezone`        | `homeassistant/number/pool-controller/timezone/state`        | `homeassistant/number/pool-controller/timezone/set`       |
| Timezone info                  | `homie/pool-controller/operation-mode/timezone-info`  | `sensor/timezone-info`   | `homeassistant/sensor/pool-controller/timezone-info/state`   | -                                                         |
| Log output                     | `homie/pool-controller/Log/log`                       | `sensor/log`             | `homeassistant/sensor/pool-controller/log/state`             | -                                                         |
| Log level                      | `homie/pool-controller/Log/Level`                     | `select/log-level`       | `homeassistant/select/pool-controller/log-level/state`       | `homeassistant/select/pool-controller/log-level/set`      |
| Log to serial                  | `homie/pool-controller/Log/LogSerial`                 | `switch/log-serial`      | `homeassistant/switch/pool-controller/log-serial/state`      | `homeassistant/switch/pool-controller/log-serial/set`     |
| OTA update trigger             | -                                                     | `button/ota-update`      | -                                                            | `homeassistant/button/pool-controller/ota-update/set`     |
| OTA status                     | -                                                     | `sensor/ota-status`      | `homeassistant/sensor/pool-controller/ota-status/state`      | -                                                         |

## Features

Both protocols support:

- Temperature sensors (pool, solar, controller)
- Relay switches (pool pump, solar pump)
- Operation modes (auto, manual, boost, timer)
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

## Migration

If you're migrating from Homie to Home Assistant or vice versa:

1. Update the `mqtt-protocol` setting (default: "homeassistant")
2. Reboot the device
3. The device will automatically start publishing in the new format
4. Update your home automation system to use the new topics
