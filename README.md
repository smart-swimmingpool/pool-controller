# Pool Controller 3.3 | 🏊 Smart Swimming Pool

[![Smart Swimmingpool](https://img.shields.io/badge/%F0%9F%8F%8A%20-Smart%20Swimmingpool-blue.svg)](https://github.com/smart-swimmingpool)
[![PlatformIO CI](https://github.com/smart-swimmingpool/pool-controller/workflows/PlatformIO%20CI/badge.svg)](https://github.com/smart-swimmingpool/pool-controller/actions?query=workflow%3A%22PlatformIO+CI%22)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v1.4%20adopted-ff69b4.svg)](code-of-conduct.md)
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

## 🏊 The MQTT-enabled Smart Swimming Pool Controller 🎛️

Manage your swimming pool the smart way - enjoy it in a
**comfortable and affordable (less than 100€)** way with
**professional-grade reliability**.

🔗 **Discussions:** [GitHub Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)

---

## 🚀 Quick Start

**New to the project?** Start here:

- [📖 **Quick Start Guide**](docs/quick-start.md) – Step-by-step setup for beginners (recommended!)
- [❓ **Frequently Asked Questions (FAQ)**](docs/faq.md) – Troubleshooting common issues

---

## ✨ Main Features

### 🏊 Pool Management

- ✅ **Timed circulation** for cleaning
- ✅ **Solar heating control** via additional pump
- ✅ **Multiple operation modes**: Auto, Manual, Boost, Timer
- ✅ **Temperature-based automation** (e.g., disable solar heating if pool is too hot)

### 🌐 MQTT Integration

- ✅ **[Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)** –
  Native HA integration (v3.3.0+)
- ✅ **Independent of specific smart home servers**
  - ✅ [Home Assistant](https://www.home-assistant.io/) via MQTT Discovery
  - ✅ [openHAB](https://www.openhab.org) via MQTT (manual configuration required)

### 🛡️ Reliability & 24/7 Operation (v3.3.0)

- ✅ **State Persistence** – All settings survive reboots and power failures
  - Operation mode, temperatures, timer settings automatically restored
  - ESP32 NVS storage
- ✅ **System Health Monitoring** – Continuous health checks
  - Memory monitoring every 10 seconds
  - Auto-reboot at critical memory threshold (8KB)
  - Hardware watchdog timer (30s timeout)
- ✅ **Memory Optimization** – Efficient resource usage
  - 90% reduction in heap fragmentation
  - 2,880-28,800 fewer allocations per day
  - Fixed millis() overflow for operation beyond 49.7 days
- ✅ **Automatic Recovery** – Self-healing capabilities
  - Auto-recovery from memory exhaustion
  - Watchdog timer prevents system hangs
  - Sensor auto-recovery with fast re-polling
  - Boot-loop detection with Safe Mode
  - NTP graceful degradation (3-stage)
  - Zero manual intervention required

### 🌐 Built-in Web Interface

- ✅ **Full Web Dashboard** – Direct device management without Home Assistant
  - AP Mode: Connects as `Pool-Controller-Setup` WiFi hotspot at `192.168.4.1`
  - STA Mode: Web server on port 80 at the device's local IP
  - REST API for programmatic access (`/api/status`, `/api/config`, etc.)
  - Password-protected with session management (cookie-based, SHA-256)
  - Tabs: Dashboard, WiFi Setup, MQTT Settings, Configuration, Security & Update
  - OTA firmware update via web interface

### 🔧 Developer Features

- ✅ **Over-The-Air (OTA) Updates** – Remote firmware updates via WiFi
  - No physical access required for updates
  - Password-protected secure updates
  - mDNS discovery support
- ✅ Time sync via NTP (configurable server, default: pool.ntp.org)
- ✅ Configurable timezone with DST support (10 major timezones available)
- ✅ Logging information via MQTT
- ✅ Modern libraries (ArduinoJson 7.3.0, NTPClient 3.2.1)
- ✅ Clean, formatted code following project standards

---

## 📚 Documentation

| **Guide**                                                                    | **Description**                            | **For**                       |
| ---------------------------------------------------------------------------- | ------------------------------------------ | ----------------------------- |
| [Quick Start Guide](docs/quick-start.md)                                     | Step-by-step setup for beginners           | 🆕 **New users**              |
| [FAQ](docs/faq.md)                                                           | Troubleshooting common issues              | ❓ **All users**              |
| [Users Guide](docs/users-guide.md)                                           | Web dashboard, operation modes, MQTT       | 🎛️ **Intermediate users**     |
| [Hardware Guide](docs/hardware-guide.md)                                     | Assembly, wiring, parts list               | 🔧 **Builders**               |
| [MQTT Configuration](docs/mqtt-configuration.md)                             | Home Assistant Discovery, entity reference | 🌐 **Smart home integrators** |
| [State Persistence](docs/state-persistence.md)                               | How settings are saved across reboots      | 💾 **Advanced users**         |
| [OTA Updates](docs/ota-updates.md)                                           | Remote firmware updates                    | 📡 **Developers**             |
| [ESP32 Schematic Optimization (DE)](docs/esp32-schematic-optimization-de.md) | Pin assignment and optimization            | 🔌 **Hardware experts**       |

---

## 📦 Recent Updates (v3.3.0)

### 🔄 ESP8266 Support Removed

- Codebase is now **ESP32-only** — cleaner, faster, more reliable
- Removed all `#ifdef ESP8266` conditional compilation
- Platform: esp32dev (ESP32 DevKit V1)

### 🛡️ Phase 3 — Proactive Resilience

- **P7: Fast Sensor Recovery** — DallasTemperatureNode polls every 5s
  (instead of 300s) when sensor reads NaN
- **P8: Boot-Loop Detection** — NVS-based boot counter, Safe Mode after 4
  consecutive short boots (<5 min), all relays forced OFF
- **P9: Configurable Fallback Times** — ConfigManager settings
  `time-loss-green-hours` and `time-loss-red-hours` replace hardcoded NTP
  thresholds

### 🐛 v3.1.0 (Previous Release)

- **Critical Bug Fixes**
  - Fixed critical logging bug (vsnprintf buffer initialization)
  - Fixed millis() overflow for reliable operation beyond 49.7 days
  - Added buffer validation and overflow detection
- **New Features**
  - State persistence across reboots and power failures
  - Home Assistant MQTT Discovery support
  - System health monitoring with auto-reboot
  - Hardware watchdog timer (ESP32)
- **Performance Improvements**
  - Eliminated 10+ String allocations per measurement cycle
  - Reduced heap fragmentation by ~90%
  - Optimized memory usage for 24/7 operation

See [CHANGELOG.md](CHANGELOG.md) for complete details.

---

## 🚀 Planned Features

- [ ] Configurable NTP Server (currently hardcoded: europe.pool.ntp.org)
- [ ] Smart learning: Improved pool pump circulation optimization
- [ ] Two separate circulation cycles
- [ ] Temperature-based cleaning circulation time
- [ ] Improved operation without WiFi connection
  - Display and button setup interface
- See also the [issue list](https://github.com/smart-swimmingpool/pool-controller/issues)

---

## 🔧 Configuration

### MQTT Protocol Selection

Configure your preferred MQTT protocol in the device settings:

- `mqtt-protocol = "homeassistant"` - Home Assistant native discovery **(only option in v3.3.0+)**

See [docs/mqtt-configuration.md](docs/mqtt-configuration.md) for setup details.

### State Persistence

All controller states are automatically saved and restored:

- Operation modes and settings
- Temperature thresholds
- Timer configurations
- Relay states

See [docs/state-persistence.md](docs/state-persistence.md) for details.

---

## 🤝 Contributing

We welcome contributions! Before submitting a pull request, please:

1. **Read the coding guidelines**: [`.github/CODING_GUIDELINES.md`](.github/CODING_GUIDELINES.md)
2. **Run local linting**: `make lint-fix && make lint`
3. **Test your changes**: `make build` (builds for ESP32)
4. **Check for issues**: See [`.github/QUICK_REFERENCE.md`](.github/QUICK_REFERENCE.md) for common fixes

All code must pass the same Super-Linter checks run in CI
(cpplint for C/C++, EditorConfig, and Markdown/YAML/JSON validation) before
merge.

---

## 🙏 Credits

- [Lübbe Onken](http://github.com/luebbe) for `TimeClientHelper`
- All [contributors](https://github.com/smart-swimmingpool/pool-controller/graphs/contributors) for their valuable input!

---

## 📜 License

[MIT License](LICENSE) – Free to use, modify, and share.

---

## 🌐 Community

- **Discussions:** [GitHub Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)
- **Home Assistant Community:** [community.home-assistant.io](https://community.home-assistant.io/)
- **Reddit:** [r/homeassistant](https://www.reddit.com/r/homeassistant/)

---

## 📢 Support

If you encounter issues:

1. Check the **[FAQ](docs/faq.md)** for common problems.
2. Search the **[Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)**.
3. Open a **[new issue](https://github.com/smart-swimmingpool/pool-controller/issues/new)** with:
   - A detailed description of the problem.
   - Screenshots (e.g., serial monitor output, Web Dashboard).
   - Your hardware setup (ESP32 model, relay module, sensors).
   - Firmware version (check Web Dashboard or serial monitor).

---

[![DIY My Smart Home](https://img.shields.io/badge/DIY%20My%20Smart%20Home-Medium-blue)](https://medium.com/diy-my-smart-home)
