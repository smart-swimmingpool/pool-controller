# MQTT Protocol Configuration

The Pool Controller now supports two MQTT protocols:

## 1. Homie Convention (Default)

The [Homie Convention](https://homieiot.github.io/) provides a standardized
MQTT device discovery convention.

To use Homie (default):

```json
{
  "mqtt-protocol": "homie"
}
```

## 2. Home Assistant MQTT Discovery

[Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
allows automatic device discovery in Home Assistant.

To use Home Assistant:

```json
{
  "mqtt-protocol": "homeassistant"
}
```

## Configuration

You can set the MQTT protocol in the Homie configuration UI or in the
`config.json` file:

### Via Homie UI

1. Connect to the device's WiFi AP during initial setup
2. Navigate to the configuration page
3. Set "mqtt-protocol" to either "homie" or "homeassistant"
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

## Features

Both protocols support:

- Temperature sensors (pool, solar, controller)
- Relay switches (pool pump, solar pump)
- Operation modes (auto, manual, boost, timer)
- Configuration via MQTT
- State monitoring

## Migration

If you're migrating from Homie to Home Assistant or vice versa:

1. Update the `mqtt-protocol` setting
2. Reboot the device
3. The device will automatically start publishing in the new format
4. Update your home automation system to use the new topics
