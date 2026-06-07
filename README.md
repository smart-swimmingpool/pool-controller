# Pool Controller 3.3 | 🏊 Smart Swimming Pool

[![Smart Swimmingpool](https://img.shields.io/badge/%F0%9F%8F%8A%20-Smart%20Swimmingpool-blue.svg)](https://github.com/smart-swimmingpool)
[![PlatformIO CI](https://github.com/smart-swimmingpool/pool-controller/workflows/PlatformIO%20CI/badge.svg)](https://github.com/smart-swimmingpool/pool-controller/actions?query=workflow%3A%22PlatformIO+CI%22)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v1.4%20adopted-ff69b4.svg)](code-of-conduct.md)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J33A8DT)

## 🏊 The MQTT-enabled Smart Swimming Pool Controller 🎛️

Manage your swimming pool the smart way - enjoy it in a comfortable
and affordable (less than 100€) way with professional-grade reliability.

Discussions: <https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions>

## Main Features

### Pool Management

- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] Multiple operation modes: Auto, Manual, Boost, Timer

### MQTT Integration

- [x] [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) - Native HA integration
- [x] Independent of specific smart home servers
  - [x] [Home Assistant](https://www.home-assistant.io/) via native MQTT Discovery
  - [x] [openHAB](https://www.openhab.org) via MQTT

### Reliability & 24/7 Operation (v3.2.0)

- [x] **State Persistence** - All settings survive reboots and power failures
  - Operation mode, temperatures, timer settings automatically restored
  - ESP32 NVS storage
- [x] **System Health Monitoring** - Continuous health checks
  - Memory monitoring every 10 seconds
  - Auto-reboot at critical memory threshold (8KB)
  - Hardware watchdog timer (30s timeout)
- [x] **Memory Optimization** - Efficient resource usage
  - 90% reduction in heap fragmentation
  - 2,880-28,800 fewer allocations per day
  - Fixed millis() overflow for operation beyond 49.7 days
- [x] **Automatic Recovery** - Self-healing capabilities
  - Auto-recovery from memory exhaustion
  - Watchdog timer prevents system hangs
  - Sensor auto-recovery with fast re-polling
  - Boot-loop detection with Safe Mode
  - NTP graceful degradation (3-stage)
  - Zero manual intervention required

### Built-in Web Interface

- [x] **Full Web Dashboard** - Direct device management without Home Assistant
  - AP Mode: Connects as `Pool-Controller-Setup` WiFi hotspot at `192.168.4.1`
  - STA Mode: Web server on port 80 at the device's local IP
  - REST API for programmatic access (`/api/status`, `/api/config`, etc.)
  - Password-protected with session management (cookie-based, SHA-256)
  - Tabs: Dashboard, WiFi Setup, MQTT Settings, Configuration, Security & Update
  - OTA firmware update via web interface

### Developer Features

- [x] **Over-The-Air (OTA) Updates** - Remote firmware updates via WiFi
  - No physical access required for updates
  - Password-protected secure updates
  - mDNS discovery support
- [x] Time sync via NTP (configurable server, default: pool.ntp.org)
- [x] Configurable timezone with DST support (10 major timezones available)
- [x] Logging information via MQTT
- [x] Modern libraries (ArduinoJson 7.3.0, NTPClient 3.2.1)
- [x] Clean, formatted code following project standards

## Recent Updates (v3.2.0)

### ESP8266 Support Removed

- Codebase is now ESP32-only — cleaner, faster, more reliable
- Removed all `#ifdef ESP8266` conditional compilation
- Platform: esp32dev (ESP32 DevKit V1)

### Phase 3 — Proactive Resilience

- **P7: Fast Sensor Recovery** — DallasTemperatureNode polls every 5s (instead of 300s) when sensor reads NaN
- **P8: Boot-Loop Detection** — NVS-based boot counter, Safe Mode after 4
  consecutive short boots (<5 min), all relays forced OFF
- **P9: Configurable Fallback Times** — ConfigManager settings
  `time-loss-green-hours` and `time-loss-red-hours` replace hardcoded NTP
  thresholds

### v3.1.0 (Previous Release)

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

- `mqtt-protocol = "homeassistant"` - Home Assistant native discovery **(only option)**

See [docs/mqtt-configuration.md](docs/mqtt-configuration.md) for setup details.

### State Persistence

All controller states are automatically saved and restored:

- Operation modes and settings
- Temperature thresholds
- Timer configurations
- Relay states

See [docs/state-persistence.md](docs/state-persistence.md) for details.

## Guides

- [Users Guide](docs/users-guide.md)
- [Hardware Guide](docs/hardware-guide.md)
- [ESP32 Schaltplananalyse & Optimierung (DE)](docs/esp32-schematic-optimization-de.md)
- [Software Guide](docs/software-guide.md)
- [MQTT Configuration Guide](docs/mqtt-configuration.md)
- [State Persistence & Monitoring](docs/state-persistence.md)
- [Over-The-Air Updates](docs/ota-updates.md)
- [Home Assistant Integration](docs/home-assistant/) — MQTT Discovery, Lovelace Dashboard

## Development

### Local Linting & Formatting

Use the provided `Makefile` tasks to maintain code quality:

```bash
# Auto-fix formatting issues (C++, Markdown, YAML)
make lint-fix

# Run all linters (same as CI)
make lint

# Build for all platforms
make build

# See all available tasks
make help
```

**Before each commit**, run:

```bash
make lint-fix && make lint
```

### Optional: Install Pre-Commit Hook

Automatically run linting checks before every commit:

```bash
ln -s ../../scripts/pre-commit.sh .git/hooks/pre-commit
```

To bypass the hook when needed: `git commit --no-verify`

## Contributing

We welcome contributions! Before submitting a pull request, please:

1. **Read the coding guidelines**: [`.github/CODING_GUIDELINES.md`](.github/CODING_GUIDELINES.md)
2. **Run local linting**: `make lint-fix && make lint`
3. **Test your changes**: `make build` (builds for ESP32)
4. **Check for issues**: See [`.github/QUICK_REFERENCE.md`](.github/QUICK_REFERENCE.md) for common fixes

All code must pass the same Super-Linter checks run in CI (cpplint for C/C++, EditorConfig, and
Markdown/YAML/JSON validation) before merge.

## Credits

- [Lübbe Onken](http://github.com/luebbe) for `TimeClientHelper`

## License

[LICENSE](LICENSE)

---

[DIY My Smart Home](https://medium.com/diy-my-smart-home)
