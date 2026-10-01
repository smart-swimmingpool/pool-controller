# Pool Controller | 🏊 Smart Swimming Pool

[![Smart Swimmingpool](https://img.shields.io/badge/%F0%9F%8F%8A%20-Smart%20Swimmingpool-blue.svg)](https://github.com/smart-swimmingpool)
[![PlatformIO CI](https://github.com/smart-swimmingpool/pool-controller/workflows/PlatformIO%20CI/badge.svg)](https://github.com/smart-swimmingpool/pool-controller/actions?query=workflow%3A%22PlatformIO+CI%22)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J33A8DT)

---

> **⚠️ WARNING: This project involves 230V AC mains voltage!**
>
> - **Only proceed if you have basic electronics knowledge.**
> - **Always use a Residual Current Device (RCD/FI circuit breaker) for the pump circuit.**
> - **Disconnect power before working on the circuit.**
> - **Keep low-voltage (sensor) wiring separate from mains wiring.**
> - **If in doubt, consult a qualified electrician.**
> - **This project is NOT certified (no CE/UL mark). For personal use only!**

---

## 🏊 Overview

The **Pool Controller** is an **ESP32-based control unit** that automates your swimming pool management. It provides intelligent circulation, solar heating control, and comprehensive monitoring via MQTT integration with Home Assistant and other smart home systems.

**Key Features:**
- ✅ Timed circulation for water cleaning
- ✅ Solar heating control via additional pump
- ✅ Multiple operation modes: Auto, Manual, Boost, Timer
- ✅ Temperature-based automation
- ✅ Home Assistant MQTT Discovery (v3.3.0+)
- ✅ Built-in web interface with REST API
- ✅ State persistence across reboots
- ✅ System health monitoring with auto-recovery
- ✅ OTA firmware updates

**Cost:** ~45–75€ (excluding pumps and pool infrastructure)

---

## 🚀 Quick Start

**New to the project?** Begin with these resources:

- [📖 **Quick Start Guide**](docs/quick-start.md) – Step-by-step setup for beginners
- [❓ **Frequently Asked Questions**](docs/faq.md) – Troubleshooting common issues
- [🌐 **MQTT Configuration**](docs/mqtt-configuration.md) – Home Assistant integration

---

## 💻 Hardware Requirements

| Component | Qty | Approx. Cost | Notes |
|-----------|:---:|:------------:|-------|
| ESP32 Development Board | 1 | 10–15€ | 4MB+ flash required |
| DS18B20 Temperature Sensor (waterproof) | 2 | 8–12€ | Pool + solar collector |
| 2-Channel 5V Relay Module | 1 | 5–8€ | With optocoupler isolation |
| Resistor 4.7kΩ | 2 | < 1€ | Pull-up for OneWire |
| USB Power Supply 5V/≥1A | 1 | 5–10€ | For ESP32 |
| **Total** | | **~45–75€** | Without pumps |

**Recommended Shops:** Amazon, AliExpress, Reichelt, Pollin, Conrad (DE/AT/CH)

---

## 📦 Software & Development

### PlatformIO Setup

```bash
# Clone the repository
git clone https://github.com/smart-swimmingpool/pool-controller.git
cd pool-controller

# Build the firmware (first build downloads dependencies)
pio run

# Flash to device
pio run --target upload

# Monitor serial output
pio run --target monitor
```

**Platform:** ESP32 DevKit V1 (esp32dev)

### Key Dependencies

- Homie for ESP8266/ESP32 (MQTT framework)
- OneWire (DS18B20 sensor communication)
- DallasTemperature (temperature sensor library)
- ArduinoJson (JSON processing for MQTT)
- NTPClient (time synchronization)
- ESPmDNS (mDNS discovery)

---

## 🌐 MQTT Integration

### Home Assistant (Recommended)

The Pool Controller supports **native Home Assistant MQTT Discovery** (v3.3.0+). Devices and entities are automatically discovered and added to your Home Assistant instance.

**MQTT Protocol:** `homeassistant` (only option in v3.3.0+)

### Supported Smart Home Systems

| System | Integration Method | Status |
|--------|-------------------|--------|
| Home Assistant | MQTT Discovery | ✅ Native support |
| openHAB | MQTT Binding | ✅ Manual configuration |
| Node-RED | MQTT nodes | ✅ Works with any MQTT broker |
| ioBroker | MQTT adapter | ✅ Works with any MQTT broker |

**MQTT Topics:** See [MQTT Configuration Guide](docs/mqtt-configuration.md) for complete topic reference.

---

## 🛡️ Reliability Features (v3.3.0)

### State Persistence
All settings survive reboots and power failures:
- Operation mode, temperatures, timer settings
- ESP32 NVS storage
- Automatic restoration on boot

### System Health Monitoring
- Memory monitoring every 10 seconds
- Auto-reboot at critical memory threshold (8KB)
- Hardware watchdog timer (30s timeout)
- Boot-loop detection with Safe Mode

### Memory Optimization
- 90% reduction in heap fragmentation
- 2,880–28,800 fewer allocations per day
- Fixed millis() overflow for operation beyond 49.7 days

---

## 📚 Documentation

| Guide | Description | Audience |
|-------|-------------|----------|
| [Quick Start Guide](docs/quick-start.md) | Step-by-step setup for beginners | 🆕 New users |
| [FAQ](docs/faq.md) | Troubleshooting common issues | ❓ All users |
| [Users Guide](docs/users-guide.md) | Web dashboard, operation modes, MQTT | 🎛️ Intermediate users |
| [Hardware Guide](docs/hardware-guide.md) | Assembly, wiring, parts list | 🔧 Builders |
| [MQTT Configuration](docs/mqtt-configuration.md) | Home Assistant Discovery, entity reference | 🌐 Smart home integrators |
| [State Persistence](docs/state-persistence.md) | How settings are saved across reboots | 💾 Advanced users |
| [OTA Updates](docs/ota-updates.md) | Remote firmware updates | 📡 Developers |
| [Software Guide](docs/software-guide.md) | Development environment, build process | 🔧 Developers |
| [ESP32 Schematic Optimization](docs/esp32-schematic-optimization.md) | Pin assignment and optimization | 🔌 Hardware experts |

---

## 📦 Recent Updates

### v3.3.0 (Current)
- ESP8266 support removed (ESP32-only)
- Phase 3: Proactive Resilience
  - Fast sensor recovery (5s polling on NaN)
  - Boot-loop detection with Safe Mode
  - Configurable fallback times
- Critical bug fixes (logging, millis() overflow)
- State persistence across reboots
- Home Assistant MQTT Discovery support
- System health monitoring
- Hardware watchdog timer

**Full Changelog:** [CHANGELOG.md](CHANGELOG.md)

---

## 🚀 Planned Features

- [ ] Configurable NTP Server
- [ ] Smart learning: Improved pool pump circulation optimization
- [ ] Two separate circulation cycles
- [ ] Temperature-based cleaning circulation time
- [ ] Improved operation without WiFi connection
- [ ] Display and button setup interface

**See:** [Issue List](https://github.com/smart-swimmingpool/pool-controller/issues)

---

## 🤝 Contributing

We welcome contributions! Please follow these steps:

1. **Read the guidelines**: [CONTRIBUTING.md](CONTRIBUTING.md)
2. **Fork the repository** and create a feature branch
3. **Make your changes** following project standards
4. **Test thoroughly** and update documentation
5. **Run quality checks**: `make lint-fix && make lint`
6. **Submit a Pull Request**

**Quality Gates:**
- ✅ Super-Linter (code quality)
- ✅ PlatformIO CI (build verification)
- ✅ Manual review by maintainers

---

## 📜 License

[MIT License](LICENSE) – Free to use, modify, and share.

---

## 🌐 Community & Support

- **Discussions:** [GitHub Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)
- **Website:** [smart-swimmingpool.com](https://smart-swimmingpool.com)
- **Home Assistant Community:** [community.home-assistant.io](https://community.home-assistant.io/)

**Need Help?**
1. Check the [FAQ](docs/faq.md)
2. Search [Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)
3. Open a [new issue](https://github.com/smart-swimmingpool/pool-controller/issues/new)

---

## 📢 Project Links

| Module | Description |
|--------|-------------|
| [Pool Controller](https://github.com/smart-swimmingpool/pool-controller) | Main control unit (this repository) |
| [Pool Monitor](https://github.com/smart-swimmingpool/monitor) | Solar-powered wireless temperature display |
| [Grafana Dashboard](https://github.com/smart-swimmingpool/grafana-dashboard) | Visualization dashboard |
| [openHAB Config](https://github.com/smart-swimmingpool/openhab-config) | openHAB configuration files |
| [Water Quality Monitor](https://github.com/smart-swimmingpool/water-quality-monitor) | Water quality monitoring (pH, chlorine) |
| [Website](https://github.com/smart-swimmingpool/website) | Project documentation website |

---

<p align="center">
  Made with ❤️ by the Smart Swimming Pool community
</p>
