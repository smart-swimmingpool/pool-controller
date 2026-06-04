---
name: mqtt-debug
description: "MQTT/Homie/Home Assistant Discovery debugging for the pool-controller. Use when asked to debug MQTT communication, verify HA Discovery payloads, test Homie convention compliance, or troubleshoot publish/subscribe issues. 🇩🇪 Deutsche Trigger: MQTT Debug, Homie Konvention, Home Assistant Discovery, Nachrichten verfolgen, mosquitto, Publisher/Subscriber, MQTT Verbindung."
keywords:
  - mqtt debug
  - mqtt fehlersuche
  - homie konvention
  - homie convention
  - home assistant discovery
  - ha discovery
  - mosquitto
  - mqtt verbindung
  - mqtt connection
  - publish subscribe
  - mqtt topics
  - last will testament
  - lwt
---

# MQTT Debugging — Pool Controller

Debugging MQTT integration for the pool-controller. Supports **Homie 3.0** and **Home Assistant MQTT Discovery** protocols.

> **🔍 Code Search**: Use `semble search "publishDiscovery"` or `semble search "MQTT protocol"` to trace MQTT code paths. See `Agents.md` §7 for full `semble` usage.

## Architecture Overview

```
Pool Controller ──WiFi──┐
                         ▼
                   ┌──────────┐
                   │  MQTT    │
                   │  Broker  │
                   └────┬─────┘
                        │
              ┌─────────┼─────────┐
              ▼         ▼         ▼
        ┌─────────┐ ┌─────────┐ ┌─────────┐
        │ openHAB │ │   HA    │ │  Other  │
        │ (Homie) │ │(Discovery│ │  MQTT   │
        └─────────┘ │ + Homie)│ │ Clients │
                    └─────────┘ └─────────┘
```

## Protocol Selection

Configured via `ConfigManager::getMqtt().protocol` in `MQTTConfig.hpp`:
- `MQTTProtocol::HOME_ASSISTANT` (default) — Home Assistant MQTT Discovery
- `MQTTProtocol::HOMIE` — Homie 3.0 convention

Runtime setting: `mqtt-protocol` in device web portal or config.

## Home Assistant MQTT Discovery

**Publisher**: `MqttPublisher.hpp/cpp`

Key methods:
- `MqttPublisher::publishDiscovery()` — publishes all discovery configs
- `MqttPublisher::publishStates()` — updates sensor/switch states
- `MqttPublisher::handleMqttMessage()` — processes incoming HA commands

### Discovery Topics

Base topic format: `homeassistant/<component>/pool-controller/<object-id>/config`

The controller publishes these discovery configs:

**Sensors (temperature):**

| Object ID | Name | Device Class | Unit | Icon |
|-----------|------|-------------|------|------|
| `pool-temp` | Pool Temperature | `temperature` | `°C` | `mdi:pool` |
| `solar-temp` | Solar Temperature | `temperature` | `°C` | `mdi:solar-power` |
| `controller-temp` | Controller Temperature | `temperature` | `°C` | `mdi:thermometer` |

**Sensors (diagnostics):**

| Object ID | Name | Device Class | Unit | Icon |
|-----------|------|-------------|------|------|
| `heap` | Free Heap Space | — | `B` | `mdi:memory` |
| `max-alloc` | Max Alloc Block | — | `B` | `mdi:memory` |
| `rssi` | WiFi Signal Strength | — | `dBm` | `mdi:wifi` |
| `uptime` | System Uptime | — | `s` | `mdi:clock-outline` |

**Switches:**

| Object ID | Name | Icon |
|-----------|------|------|
| `pool-pump` | Pool Pump | `mdi:pump` |
| `solar-pump` | Solar Pump | `mdi:solar-panel` |

**Select:**

| Object ID | Name | Options | Icon |
|-----------|------|---------|------|
| `mode` | Operation Mode | `auto`, `manu`, `boost`, `timer` | `mdi:sync` |

**Numbers (parameters):**

| Object ID | Name | Min | Max | Step | Unit | Icon |
|-----------|------|-----|-----|------|------|------|
| `pool-max-temp` | Max. Pool Temp | 0 | 40 | 0.1 | `°C` | `mdi:thermometer-chevron-up` |
| `solar-min-temp` | Min. Solar Temp | 0 | 100 | 0.1 | `°C` | `mdi:thermometer-chevron-down` |
| `hysteresis` | Temperature Hysteresis | 0 | 10 | 0.1 | `K` | `mdi:delta` |
| `timer-start-h` | Timer Start Hour | 0 | 23 | 1 | `h` | `mdi:clock-start` |
| `timer-start-min` | Timer Start Minute | 0 | 59 | 1 | `min` | `mdi:clock-start` |
| `timer-end-h` | Timer End Hour | 0 | 23 | 1 | `h` | `mdi:clock-end` |
| `timer-end-min` | Timer End Minute | 0 | 59 | 1 | `min` | `mdi:clock-end` |
| `timezone` | Timezone Index | 0 | 9 | 1 | — | `mdi:map-clock` |

### Debugging HA Discovery

**Common issues**:
1. **Truncated JSON payload** — The serialization buffer must be **≥25% larger** than the actual JSON output. If HA doesn't show entities, check buffer sizes in `MqttPublisher.cpp`. See `Agents.md` §21 and the `cpp-memory-opt` skill.
2. **Device ID mismatch** — `deviceId_` is generated once. Verify it's consistent across reboots.
3. **Discovery retained flag** — HA Discovery messages should be published with `retained=true`.
4. **`device_class: 'measurement'` / `expected SensorDeviceClass` errors** — These come from **stale retained MQTT messages of the old Homie framework** (Vorgänger-Firmware). The old Homie auto-discovery published sensor entities with `device_class: "measurement"`, which is not a valid `SensorDeviceClass` in newer Home Assistant versions. The current firmware uses correct component types (`number` for parameters, `sensor` only for actual sensors) and valid device classes (`temperature` or none). Fix: delete retained messages or delete the old device entry in HA (→ device registry).
5. **Device disappears after cleanup / no new entities appear** — Discovery messages are only published **on boot / MQTT connect**. After deleting the old MQTT device in HA, a reboot of the ESP32 is required to re-publish discovery. Use `pio run --target upload` or power-cycle the device.

**Manual verification**:
```bash
# Subscribe to all HA discovery topics
mosquitto_sub -h <broker> -t "homeassistant/+/pool-controller/+/config"

# Subscribe to state updates
mosquitto_sub -h <broker> -t "homeassistant/+/pool-controller/+/state"

# Publish a command (e.g., set operation mode to "boost")
mosquitto_pub -h <broker> -t "homeassistant/select/pool-controller/mode/set" -m "boost"
```

## Homie 3.0 Convention

The Homie convention uses device-topic structure:
```
homie/<device-id>/
├── $homie         → "3.0"
├── $name          → "Smart Pool Controller"
├── $nodes         → "pool-temp,solar-temp,..."
├── $implementation → "esp32"
├── pool-temp/
│   ├── $name      → "Pool Temperature"
│   ├── $type      → "temperature"
│   ├── $unit      → "°C"
│   └── temperature → "25.5"
└── ...
```

## Homie → HA Discovery Migration

The old Homie-based firmware auto-generated Home Assistant discovery from Homie properties. This created topics in the format:
```
homeassistant/sensor/<mac>_<node>_<property>/config
```
with `device_class: "measurement"` — which is **invalid** in newer HA versions (expected `SensorDeviceClass` enum).

The current firmware (`MqttPublisher.cpp`) uses native HA MQTT Discovery with correct component types:
- Temperatures → `sensor` with `device_class: "temperature"`
- Diagnostics → `sensor` without `device_class`
- Pump relays → `switch`
- Operation mode → `select`
- Parameters (hysteresis, timer, etc.) → `number` (not `sensor`)
- Device block: `manufacturer: "smart-swimmingpool"`, `name: "Pool Controller"`

### Cleaning Up Stale Homie Retained Messages

After migrating to the new firmware, old Homie discovery messages may remain on the MQTT broker as retained messages, causing errors like:
```
Error 'expected SensorDeviceClass or one of 'date', 'enum', 'timestamp', ...' 
when processing MQTT discovery message topic: 'homeassistant/sensor/<mac>_.../config'
```

**Fix options:**

1. **Delete old device entry in HA** → Einstellungen → Geräte & Dienste → MQTT → Gerät löschen, dann ESP32 **neu starten** (Discovery wird nur beim Boot/MQTT-Connect gepublisht)

2. **Manually clear retained topics** via mosquitto_pub:
   ```bash
   mosquitto_pub -h <broker> -t "homeassistant/sensor/<mac>_#" -n -r
   ```

3. **HA MQTT-Konfiguration aktualisieren** → Schaltfläche "Konfiguration aktualisieren" in der MQTT-Integration

## MQTT Connection Flow

From `NetworkManager.hpp`:
1. WiFi connects (retry every `kWiFiRetryIntervalMs` = 5000ms)
2. MQTT connects (retry every `kMqttRetryIntervalMs` = 5000ms)
3. On MQTT connect → publish Discovery + States
4. Periodically publish States every `loopInterval` seconds (default 10)

## Debugging Tools

```bash
# Monitor all MQTT traffic from the controller
mosquitto_sub -v -h <broker> -t "#" | grep "pool-controller"

# Watch HA discovery specifically
mosquitto_sub -v -h <broker> -t 'homeassistant/+/pool-controller/+/config'

# Watch HA state updates
mosquitto_sub -v -h <broker> -t 'homeassistant/+/pool-controller/+/state'

# Check Homie base topic (old firmware, still useful for legacy)
mosquitto_sub -v -h <broker> -t 'homie/5ccf7fb97572/#'

# List all retained Homie discovery (stale cleanup)
mosquitto_sub -v -h <broker> -t 'homeassistant/sensor/5ccf7fb97572_#' --retained-only

# Delete stale retained Homie discovery
mosquitto_pub -h <broker> -t "homeassistant/sensor/5ccf7fb97572_#" -n -r

# Publish test commands (current firmware)
mosquitto_pub -t "homeassistant/switch/pool-controller/pool-pump/set" -m "ON"
mosquitto_pub -t "homeassistant/select/pool-controller/mode/set" -m "auto"
mosquitto_pub -t "homeassistant/number/pool-controller/hysteresis/set" -m "2.0"
```

## Serial Debug Output for MQTT

When monitoring the ESP32 serial output:
```
✓ Network initialized
✓ MQTT connected
✓ Discovery published
  Published states: ...
```

If MQTT connection fails, check:
1. WiFi credentials are correct (WiFi config in web portal)
2. Broker address and port are reachable
3. Credentials (if broker requires auth)
4. TLS settings match broker
