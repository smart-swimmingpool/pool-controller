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

The controller publishes these discovery configs:

| Component | Object ID | Device Class | Unit |
|-----------|-----------|-------------|------|
| `sensor` | `pool-temp` | `temperature` | `°C` |
| `sensor` | `solar-temp` | `temperature` | `°C` |
| `sensor` | `controller-temp` | `temperature` | `°C` |
| `switch` | `pool-pump` | — | — |
| `switch` | `solar-pump` | — | — |
| `select` | `operation-mode` | — | — |
| `number` | `pool-max-temp` | — | `°C` |
| `number` | `solar-min-temp` | — | `°C` |
| `number` | `temp-hysteresis` | — | `°C` |

### Debugging HA Discovery

**Common issues**:
1. **Truncated JSON payload** — The serialization buffer must be **≥25% larger** than the actual JSON output. If HA doesn't show entities, check buffer sizes in `MqttPublisher.cpp`. See `Agents.md` §21 and the `cpp-memory-opt` skill.
2. **Device ID mismatch** — `deviceId_` is generated once. Verify it's consistent across reboots.
3. **Discovery retained flag** — HA Discovery messages should be published with `retained=true`.

**Manual verification**:
```bash
# Subscribe to all HA discovery topics
mosquitto_sub -h <broker> -t "homeassistant/+/pool-controller-*/config"

# Subscribe to state updates
mosquitto_sub -h <broker> -t "homeassistant/+/pool-controller-+/state"

# Publish a command (e.g., set operation mode to "boost")
mosquitto_pub -h <broker> -t "homeassistant/select/pool-controller-operation-mode/set" -m "boost"
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
mosquitto_sub -v -h <broker> -t 'homeassistant/+/pool-controller-+/#' 

# Check Homie base topic
mosquitto_sub -v -h <broker> -t 'homie/pool-controller/#'

# Publish test commands
mosquitto_pub -t "homeassistant/switch/pool-controller-pool-pump/set" -m "ON"
mosquitto_pub -t "homeassistant/select/pool-controller-operation-mode/set" -m "auto"
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
