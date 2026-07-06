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

# Pool Controller | \ud83c\udfca Smart Swimming Pool

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

### \ud83c\udfca Pool Management
- **Timed circulation** for automatic water cleaning
- **Solar heating control** via additional pump
- **Multiple operation modes**: Auto, Manual, Boost, Timer
- **Temperature-based automation** (disable solar heating when pool is too hot)

### \ud83c\udf10 Smart Home Integration
- **Home Assistant MQTT Discovery** (v3.3.0+) \u2014 Native integration with automatic entity creation
- **MQTT protocol** \u2014 Works with any MQTT-compatible smart home system
- **REST API** \u2014 Direct device control via HTTP
- **Web Dashboard** \u2014 Built-in web interface for configuration and monitoring

### \ud83d\udee1\ufe0f Reliability & 24/7 Operation
- **State Persistence** \u2014 All settings survive reboots and power failures
- **System Health Monitoring** \u2014 Continuous health checks with auto-recovery
- **Memory Optimization** \u2014 Efficient resource usage for long-term operation
- **Hardware Watchdog** \u2014 Automatic recovery from system hangs

### \ud83d\udd27 Developer Features
- **Over-The-Air (OTA) Updates** \u2014 Remote firmware updates via WiFi
- **NTP Time Synchronization** \u2014 Automatic time sync with configurable servers
- **Timezone Support** \u2014 DST handling for 10 major timezones
- **Comprehensive Logging** \u2014 Debug information via MQTT

## \ud83d\udce6 Quick Start

**New users:** Begin with the [Quick Start Guide](quick-start.md) for step-by-step setup instructions.

**Experienced users:** See the [Hardware Guide](hardware-guide.md) for parts list and wiring, or the [MQTT Configuration](mqtt-configuration.md) for smart home integration.

## \ud83c\udf10 MQTT Topics Overview

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

## \ud83d\udcbb Hardware Requirements

| Component | Qty | Approx. Cost | Notes |
|-----------|:---:|:------------:|-------|
| ESP32 Development Board | 1 | 10\u201315\u20ac | 4MB+ flash required |
| DS18B20 Temperature Sensor (waterproof) | 2 | 8\u201312\u20ac | Pool + solar collector |
| 2-Channel 5V Relay Module | 1 | 5\u20138\u20ac | With optocoupler isolation |
| Resistor 4.7k\u03a9 | 2 | < 1\u20ac | Pull-up for OneWire |
| USB Power Supply 5V/\u22651A | 1 | 5\u201310\u20ac | For ESP32 |
| **Total** | | **~45\u201375\u20ac** | Without pumps |

## \ud83d\ude80 Getting Started

1. **Order Parts** \u2014 See [Hardware Guide](hardware-guide.md) for complete BOM
2. **Assemble Hardware** \u2014 Follow wiring diagrams and safety instructions
3. **Flash Firmware** \u2014 Use PlatformIO to build and upload
4. **Configure WiFi & MQTT** \u2014 Via web interface or serial monitor
5. **Integrate with Smart Home** \u2014 Auto-discovery with Home Assistant

## \ud83d\udce2 Support & Community

- **Documentation:** [smart-swimmingpool.com](https://smart-swimmingpool.com)
- **Discussions:** [GitHub Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)
- **Issues:** [GitHub Issues](https://github.com/smart-swimmingpool/pool-controller/issues)

## \ud83d\udcdc Additional Resources

- [PlatformIO Documentation](https://docs.platformio.org/) \u2014 Build system and development
- [ESP32 Datasheet](https://www.espressif.com/en/products/socs/esp32) \u2014 Microcontroller specifications
- [MQTT Protocol](https://mqtt.org/) \u2014 Message Queuing Telemetry Transport
- [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) \u2014 Smart home integration
- [DS18B20 Datasheet](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) \u2014 Temperature sensor specifications

## \ud83d\udce1\ufe0f Version Information

**Current Version:** v3.3.0 (ESP32-only)

**Release Notes:** [CHANGELOG.md](https://github.com/smart-swimmingpool/pool-controller/blob/main/CHANGELOG.md)

**Previous Versions:** ESP8266 support was removed in v3.3.0. Use v3.2.x or earlier for ESP8266.
