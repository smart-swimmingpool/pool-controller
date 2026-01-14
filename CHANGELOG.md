# Changelog

All notable changes to this project will be documented in this file.

## [3.1.0] - 2026-01-14

### Added
- **Home Assistant MQTT Discovery Support**: Added configurable MQTT protocol support
  - New `mqtt-protocol` configuration setting (homie/homeassistant)
  - Home Assistant native auto-discovery via MQTT
  - Dual protocol support: choose between Homie Convention or Home Assistant Discovery
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
- **24/7 Operation Optimization**: Reduced memory usage and improved stability
  - Eliminated 10+ String allocations per measurement cycle to prevent heap fragmentation
  - Replaced dynamic String allocations with stack-based buffers
  - Added proper millis() overflow handling in all timing loops
  - Reduced memory footprint for long-running deployments

### Updated
- **Library Updates**: Updated dependencies to latest stable versions
  - ArduinoJson: 6.18.0 → 7.3.0 (latest major version)
  - NTPClient: 3.1.0 → 3.2.1 (latest stable)

### Fixed
- **Code Quality Improvements**:
  - Fixed potential millis() overflow issues in timing loops
  - **Fixed critical bug in LoggerNode::logf**: vsnprintf was commented out, causing uninitialized buffer usage and potential crashes
  - Removed duplicate `Homie.isConnected()` checks
  - Added overflow-safe timing utility functions
  - Improved code consistency across all sensor nodes

### Removed
- Removed deprecated RCSwitchNode code from codebase

### Technical Details
- Added `Utils.hpp` with memory-efficient helper functions
- Added `MQTTConfig.hpp` for MQTT protocol configuration
- Added `HomeAssistantMQTT.hpp` for Home Assistant discovery support
- Updated all sensor and relay nodes to use stack-based string conversions
- Optimized OperationModeNode, DallasTemperatureNode, ESP32TemperatureNode, RelayModuleNode

## [3.0.0] - Previous Release
- Initial Homie 3.0 compatible release
- Pool pump and solar pump control
- Temperature monitoring
- Multiple operation modes (auto, manual, boost, timer)
