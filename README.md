# Pool Controller 3.1 | 🏊 Smart Swimmingpool

[![Smart Swimmingpool](https://img.shields.io/badge/%F0%9F%8F%8A%20-Smart%20Swimmingpool-blue.svg)](https://github.com/smart-swimmingpool)
[![PlatformIO CI](https://github.com/smart-swimmingpool/pool-controller/workflows/PlatformIO%20CI/badge.svg)](https://github.com/smart-swimmingpool/pool-controller/actions?query=workflow%3A%22PlatformIO+CI%22)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v1.4%20adopted-ff69b4.svg)](code-of-conduct.md)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

[![works with MQTT Homie](https://homieiot.github.io/img/works-with-homie.svg "[works with MQTT Homie")](https://homieiot.github.io/)

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J33A8DT)

## 🏊 The MQTT-enabled Smart Swimmingpool Controller 🎛️

Manage your swimming pool the smart way - enjoy it in a comfortable
and affordable (less than 100€) way with professional-grade reliability.

Discussions: <https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions>

## Main Features

### Pool Management

- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] Multiple operation modes: Auto, Manual, Boost, Timer

### MQTT Integration

- [x] **Configurable MQTT protocols** - Choose your preferred protocol
  - [x] [Homie 3.0](https://homieiot.github.io/) - IoT convention
  - [x] [Home Assistant MQTT
    Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
    - Native HA integration
- [x] Independent of specific smart home servers
  - [x] [openHAB](https://www.openhab.org) (v2.4+) via MQTT Homie
  - [x] [Home Assistant](https://www.home-assistant.io/) via Homie or
    native MQTT Discovery

### Reliability & 24/7 Operation (v3.1.0)

- [x] **State Persistence** - All settings survive reboots and power failures
  - Operation mode, temperatures, timer settings automatically restored
  - ESP32 NVS / ESP8266 EEPROM storage
- [x] **System Health Monitoring** - Continuous health checks
  - Memory monitoring every 10 seconds
  - Auto-reboot at critical memory threshold (4KB ESP8266, 8KB ESP32)
  - Hardware watchdog timer (ESP32, 30s timeout)
- [x] **Memory Optimization** - Efficient resource usage
  - 90% reduction in heap fragmentation
  - 2,880-28,800 fewer allocations per day
  - Fixed millis() overflow for operation beyond 49.7 days
- [x] **Automatic Recovery** - Self-healing capabilities
  - Auto-recovery from memory exhaustion
  - Watchdog timer prevents system hangs
  - Zero manual intervention required

### Developer Features

- [x] **Over-The-Air (OTA) Updates** - Remote firmware updates via WiFi
  - No physical access required for updates
  - Password-protected secure updates
  - mDNS discovery support
- [x] Time sync via NTP (configurable server, default: pool.ntp.org)
- [x] Configurable timezone with DST support (10 major timezones available)
- [x] Logging information via MQTT
- [x] Modern libraries (ArduinoJson 6.21.5, NTPClient 3.2.1)
- [x] Clean, formatted code following project standards

## Recent Updates (v3.1.0)

### Critical Bug Fixes

- Fixed critical logging bug (vsnprintf buffer initialization)
- Fixed millis() overflow for reliable operation beyond 49.7 days
- Added buffer validation and overflow detection

### New Features

- State persistence across reboots and power failures
- Home Assistant MQTT Discovery support
- System health monitoring with auto-reboot
- Hardware watchdog timer (ESP32)

### Performance Improvements

- Eliminated 10+ String allocations per measurement cycle
- Reduced heap fragmentation by ~90%
- Optimized memory usage for 24/7 operation

See [CHANGELOG.md](CHANGELOG.md) for complete details.

## Planned Features

- [ ] Configurable NTP Server (currently hardcoded: europe.pool.ntp.org)
- [ ] Smart learning: Improved pool pump circulation optimization
- [ ] Two separate circulation cycles
- [ ] Temperature-based cleaning circulation time
- [ ] Improved operation without WiFi connection
  - Display and button setup interface
- See also the [issue list](https://github.com/smart-swimmingpool/pool-controller/issues)

## Configuration

### MQTT Protocol Selection

Configure your preferred MQTT protocol in the device settings:

- `mqtt-protocol = "homie"` - Homie 3.0 convention (default)
- `mqtt-protocol = "homeassistant"` - Home Assistant native discovery

See [docs/mqtt-configuration.md](docs/mqtt-configuration.md) for setup details.

### State Persistence

All controller states are automatically saved and restored:

- Operation modes and settings
- Temperature thresholds
- Timer configurations
- Relay states (ESP32)

See [docs/state-persistence.md](docs/state-persistence.md) for details.

## Guides

- [Users Guide](docs/users-guide.md)
- [Hardware Guide](docs/hardware-guide.md)
- [Software Guide](docs/software-guide.md)
- [MQTT Configuration Guide](docs/mqtt-configuration.md) (New in v3.1.0)
- [State Persistence & Monitoring](docs/state-persistence.md) (New in v3.1.0)
- [Over-The-Air Updates](docs/ota-updates.md) (New in v3.1.0)
- [Optimization Report](docs/optimization-report.md) (New in v3.1.0)
- [Edge Case Analysis](docs/edge-case-analysis.md) - Reliability analysis
  and potential failure modes

## Contributing

We welcome contributions! Before submitting a pull request, please:

1. **Read the coding guidelines**: [`.github/CODING_GUIDELINES.md`](.github/CODING_GUIDELINES.md)
2. **Format your code**: Run `clang-format -i src/**/*.cpp src/**/*.hpp`
3. **Check for issues**: See [`.github/QUICK_REFERENCE.md`](.github/QUICK_REFERENCE.md) for common fixes
4. **Test your changes**: Build for both `esp32dev` and `nodemcuv2` environments

All code must pass Super-Linter checks (clang-format, EditorConfig, etc.) before merge.

## Credits

- [Community of Homie-ESP8266](https://gitter.im/homie-iot/ESP8266)
- [Lübbe Onken](http://github.com/luebbe) for `TimeClientHelper`
- [Ian Hubbertz](https://github.com/euphi) for [HomieLoggerNode](https://github.com/euphi/HomieLoggerNode)

## License

[LICENSE](LICENSE)

---

[DIY My Smart Home](https://medium.com/diy-my-smart-home)
