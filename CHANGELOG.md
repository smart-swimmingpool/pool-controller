# Changelog

All notable changes to this project will be documented in this file.

## [3.3.0](https://github.com/smart-swimmingpool/pool-controller/compare/v3.2.0...v3.3.0) (2026-06-06)


### Features

* Cleanup and fixes ([#72](https://github.com/smart-swimmingpool/pool-controller/issues/72)) ([90d6e07](https://github.com/smart-swimmingpool/pool-controller/commit/90d6e07383b0f26f9675ba7f25e306bf4d5b3b51))
* **ha:** Replace timer H/Min number entities with single HH:MM text entities ([#74](https://github.com/smart-swimmingpool/pool-controller/issues/74)) ([27c337f](https://github.com/smart-swimmingpool/pool-controller/commit/27c337fd1ed311cbe2752aa64ad76967adeccead))
* NTP server config via Web UI + MQTT, local time display ([#83](https://github.com/smart-swimmingpool/pool-controller/issues/83)) ([c45bd55](https://github.com/smart-swimmingpool/pool-controller/commit/c45bd55ad86564929a08b274fbec1e6ff8a9ad14))
* NVS config backup, MQTT error reporting, web UI improvements ([#82](https://github.com/smart-swimmingpool/pool-controller/issues/82)) ([817500a](https://github.com/smart-swimmingpool/pool-controller/commit/817500a0332a15a1f9425737d78c7c3b25b3396b))
* OTA update from GitHub releases, semver release management, config safety ([#77](https://github.com/smart-swimmingpool/pool-controller/issues/77)) ([2972b34](https://github.com/smart-swimmingpool/pool-controller/commit/2972b3493d6bb9c2b8416fd281bdae76bf75ea70))
* restructure web UI settings tabs and HA entity categories ([#87](https://github.com/smart-swimmingpool/pool-controller/issues/87)) ([712b037](https://github.com/smart-swimmingpool/pool-controller/commit/712b03763a32e6417c9ab5f7322bcaddb6b85668))


### Bug Fixes

* mount LittleFS during web portal startup ([#85](https://github.com/smart-swimmingpool/pool-controller/issues/85)) ([592a4c8](https://github.com/smart-swimmingpool/pool-controller/commit/592a4c8105b3ebf54391a371650100e08fbebab1))

## [3.2.0](https://github.com/smart-swimmingpool/pool-controller/compare/v3.1.0...v3.2.0) (2026-05-22)

### Features

- change default MQTT protocol to HomeAssistant
  ([5d4cad9](https://github.com/smart-swimmingpool/pool-controller/commit/5d4cad9a3173f67e0d3db6c8722207f1e91a8a80))
- semver release pipeline via release-please (Conventional Commits)
  ([#29](https://github.com/smart-swimmingpool/pool-controller/issues/29))
  ([102b18b](https://github.com/smart-swimmingpool/pool-controller/commit/102b18b64af6364e59147baf462b970cc73e5d7d))
- **wifi:** add WPS onboarding for initial WiFi provisioning
  ([#67](https://github.com/smart-swimmingpool/pool-controller/issues/67))
  ([6983af6](https://github.com/smart-swimmingpool/pool-controller/commit/6983af6f68110b1e38e286d5c28fdcc1becb5496))

### Bug Fixes

- **ci:** make website dispatch workflow non-blocking on token issues
  ([#69](https://github.com/smart-swimmingpool/pool-controller/issues/69))
  ([4e03bd3](https://github.com/smart-swimmingpool/pool-controller/commit/4e03bd3a1e0eb71b7c3538e60a5da05d03a1a35a))
- ESP32 compatibility, memory & buffer fixes + AGENTS.md update
  ([#68](https://github.com/smart-swimmingpool/pool-controller/issues/68))
  ([b95efbd](https://github.com/smart-swimmingpool/pool-controller/commit/b95efbdbd946c99142c5f688a13c1bf77aa1e7db))
- improve English grammar and word choice in documentation
  ([#47](https://github.com/smart-swimmingpool/pool-controller/issues/47))
  ([2d71dcd](https://github.com/smart-swimmingpool/pool-controller/commit/2d71dcdf5081f0650fa1ad05ce9b447d95806c41))
- master Branch renamed to main
  ([f0d0223](https://github.com/smart-swimmingpool/pool-controller/commit/f0d0223ab34884d903e8ef541ae24fc02328faf6))
- Remove space between platform name and version specifier in nodemcuv2
  environment
  ([0f34a68](https://github.com/smart-swimmingpool/pool-controller/commit/0f34a689038d78021b88aba3c528cf8c66bdd17c))

## [3.1.0] - 2026-01-14

### Added

- **Over-The-Air (OTA) Updates**: Remote firmware updates via WiFi

  - Password-protected secure updates through Homie library
  - mDNS discovery support for easy device location
  - PlatformIO and Arduino IDE integration
  - Comprehensive documentation in [OTA Updates Guide](docs/ota-updates.md)
  - Example configurations in `platformio.ini`

- **Home Assistant MQTT Discovery Support**: Added configurable MQTT
  protocol support

  - New `mqtt-protocol` configuration setting (homie/homeassistant)
  - Home Assistant native auto-discovery via MQTT
  - Dual protocol support: choose between Homie Convention or Home
    Assistant Discovery
  - See [MQTT Configuration Guide](docs/mqtt-configuration.md) for details

- **State Persistence**: All controller states now persisted across reboots

  - Operation mode (auto/manual/boost/timer)
  - Temperature settings (pool max, solar min, hysteresis)
  - Timer settings (start/end times)
  - Relay states (pool pump, solar pump)
  - Automatic restoration after power failure or reboot

- **System Health Monitoring**: Added watchdog and memory monitoring
  - Automatic reboot on critical low memory conditions
  - Hardware watchdog timer support (ESP32)
  - Memory fragmentation monitoring (ESP8266)
  - Low memory warnings logged

### Improved

- **24/7 Operation Optimization**: Reduced memory usage and improved
  stability
  - Eliminated 10+ String allocations per measurement cycle to prevent heap
    fragmentation
  - Replaced dynamic String allocations with stack-based buffers
  - Added proper millis() overflow handling in all timing loops
  - Reduced memory footprint for long-running deployments

### Updated

- **Library Updates**: Updated dependencies to latest stable versions

  - ArduinoJson: 6.18.0 → 7.3.0 (latest major version)
  - NTPClient: 3.1.0 → 3.2.1 (latest stable)

- **GitHub Actions Workflows**: Updated to latest versions
  - actions/checkout: v1/v2 → v3
  - actions/setup-python: v1 → v4 (Python 3.11)
  - github/codeql-action: v1 → v2
  - github/super-linter: v2.1.0 → v5
  - Added PlatformIO caching for faster builds

### Fixed

- **Code Quality Improvements**

  - Fixed potential millis() overflow issues in timing loops
  - **Fixed critical bug in LoggerNode::logf**: vsnprintf was commented
    out, causing uninitialized buffer usage and potential crashes
  - Removed duplicate `Homie.isConnected()` checks
  - Added overflow-safe timing utility functions
  - Improved code consistency across all sensor nodes

- **Build Pipeline**
  - Fixed static member initialization in SystemMonitor causing multiple
    definition errors
  - Moved static initialization from header to SystemMonitor.cpp
  - Build now compiles cleanly on all platforms

### Removed

- Removed deprecated RCSwitchNode code from codebase

### Technical Details

- Added `Utils.hpp` with memory-efficient helper functions
- Added `MQTTConfig.hpp` for MQTT protocol configuration
- Added `HomeAssistantMQTT.hpp` for Home Assistant discovery support
- Added `StateManager.hpp` for state persistence
- Added `SystemMonitor.hpp` and `SystemMonitor.cpp` for health monitoring
- Updated all sensor and relay nodes to use stack-based string conversions
- Optimized OperationModeNode, DallasTemperatureNode, ESP32TemperatureNode,
  RelayModuleNode

## [3.0.0] - Previous Release

- Initial Homie 3.0 compatible release
- Pool pump and solar pump control
- Temperature monitoring
- Multiple operation modes (auto, manual, boost, timer)
