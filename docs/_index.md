---
linktitle: Pool Controller
summary: ESP32-based control unit for intelligent pool management with Home Assistant MQTT Discovery

title: Pool Controller
date: "2024-01-01"
lastmod: "2026-06-28"
draft: false
toc: true
type: docs
featured: true

menu:
  docs:
    parent: Pool Controller
    name: Overview
    weight: 10

tags: ["docs", "esp32", "controller", "tutorial"]
---

<span style="text-shadow: none;">
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller/subscription" data-size="large" data-show-count="true" aria-label="Watch smart-swimmingpool/pool-controller on GitHub">Watch</a>
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller" data-icon="octicon-star" data-size="large" data-show-count="true" aria-label="Star this on GitHub">Star</a><script async defer src="https://buttons.github.io/buttons.js"></script>
</span>

# Pool Controller | 🏊 Smart Swimming Pool

The **Pool Controller** is the **central control unit** for your smart swimming pool. Built around an **ESP32 microcontroller**, it provides intelligent automation for pool circulation, solar heating, and comprehensive monitoring.

## Main Features
- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) - Native HA integration
- [x] Independent of specific smarthome servers
  - [x] [Home Assistant](https://home-assistant.io) via native MQTT Discovery
  - [x] [openHAB](https://www.openhab.org) via MQTT (manual configuration required)
- [x] Timesync via NTP (europe.pool.ntp.org)
- [x] Logging of system events and diagnostics

### 🏊 Pool Management
- **Timed circulation** for automatic water cleaning
- **Solar heating control** via additional pump
- **Multiple operation modes**: Auto, Manual, Boost, Timer
- **Temperature-based automation** (disable solar heating when pool is too hot)

### 🌐 Smart Home Integration
- **Home Assistant MQTT Discovery** (v3.3.0+) — Native integration with automatic entity creation
- **MQTT protocol** — Works with any MQTT-compatible smart home system
- **REST API** — Direct device control via HTTP
- **Web Dashboard** — Built-in web interface for configuration and monitoring

### 🛡️ Reliability & 24/7 Operation
- **State Persistence** — All settings survive reboots and power failures
- **System Health Monitoring** — Continuous health checks with auto-recovery
- **Memory Optimization** — Efficient resource usage for long-term operation
- **Hardware Watchdog** — Automatic recovery from system hangs

### 🔧 Developer Features
- **Over-The-Air (OTA) Updates** — Remote firmware updates via WiFi
- **NTP Time Synchronization** — Automatic time sync with configurable servers
- **Timezone Support** — DST handling for 10 major timezones
- **Comprehensive Logging** — Debug information via MQTT

## 📦 Quick Start

**New users:** Begin with the [Quick Start Guide](quick-start.md) for step-by-step setup instructions.

**Experienced users:** See the [Hardware Guide](hardware-guide.md) for parts list and wiring, or the [MQTT Configuration](mqtt-configuration.md) for smart home integration.

## 🌐 MQTT Topics Overview

The Pool Controller publishes and subscribes to the following MQTT topics:

```text
# State Topics
smart-swimmingpool/pool-controller/state

# Temperature Topics
smart-swimmingpool/pool-controller/temperature/pool
smart-swimmingpool/pool-controller/temperature/solar

# Pump Control Topics
smart-swimmingpool/pool-controller/pump/pool/state
smart-swimmingpool/pool-controller/pump/solar/state

# Mode & Settings
smart-swimmingpool/pool-controller/mode
smart-swimmingpool/pool-controller/settings
```

**Complete Reference:** [MQTT Configuration Guide](mqtt-configuration.md)

## 💻 Hardware Requirements

| Component | Qty | Approx. Cost | Notes |
|-----------|:---:|:------------:|-------|
| ESP32 Development Board | 1 | 10–15€ | 4MB+ flash required |
| DS18B20 Temperature Sensor (waterproof) | 2 | 8–12€ | Pool + solar collector |
| 2-Channel 5V Relay Module | 1 | 5–8€ | With optocoupler isolation |
| Resistor 4.7kΩ | 2 | < 1€ | Pull-up for OneWire |
| USB Power Supply 5V/≥1A | 1 | 5–10€ | For ESP32 |
| **Total** | | **~45–75€** | Without pumps |

## 🚀 Getting Started

1. **Order Parts** — See [Hardware Guide](hardware-guide.md) for complete BOM
2. **Assemble Hardware** — Follow wiring diagrams and safety instructions
3. **Flash Firmware** — Use PlatformIO to build and upload
4. **Configure WiFi & MQTT** — Via web interface or serial monitor
5. **Integrate with Smart Home** — Auto-discovery with Home Assistant

## 📢 Support & Community

- **Documentation:** [smart-swimmingpool.com](https://smart-swimmingpool.com)
- **Discussions:** [GitHub Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)
- **Issues:** [GitHub Issues](https://github.com/smart-swimmingpool/pool-controller/issues)

## 📜 Additional Resources

- [PlatformIO Documentation](https://docs.platformio.org/) — Build system and development
- [ESP32 Datasheet](https://www.espressif.com/en/products/socs/esp32) — Microcontroller specifications
- [MQTT Protocol](https://mqtt.org/) — Message Queuing Telemetry Transport
- [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) — Smart home integration
- [DS18B20 Datasheet](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) — Temperature sensor specifications

## 📡️ Version Information

**Current Version:** v3.3.0 (ESP32-only)

**Release Notes:** [CHANGELOG.md](https://github.com/smart-swimmingpool/pool-controller/blob/main/CHANGELOG.md)

**Previous Versions:** ESP8266 support was removed in v3.3.0. Use v3.2.x or earlier for ESP8266.
