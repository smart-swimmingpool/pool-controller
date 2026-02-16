# Changelog

All notable changes to this project will be documented in this file.

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
