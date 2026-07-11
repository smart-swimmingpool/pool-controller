# Changelog

All notable changes to this project will be documented in this file.

## [4.2.0](https://github.com/smart-swimmingpool/pool-controller/compare/v4.1.1...v4.2.0) (2026-07-11)


### Features

* **web:** add /api/fs/upload endpoint for OTA-safe web asset deployment ([cee0b4a](https://github.com/smart-swimmingpool/pool-controller/commit/cee0b4aad3eedcc8e322086898e15bea18f87fca))
* **web:** make dashboard and operating parameters visible without login ([919bbde](https://github.com/smart-swimmingpool/pool-controller/commit/919bbde875d92ddf53a2a831bbcb356528344cc7))


### Bug Fixes

* **ci:** fix native test build and clang-format violations ([ba6a1a8](https://github.com/smart-swimmingpool/pool-controller/commit/ba6a1a864d741e84ed9ab9632290de0cbc0aaac4))
* **norvi:** move solar pump relay from R1 (GPIO12) to R5 (GPIO33) ([#158](https://github.com/smart-swimmingpool/pool-controller/issues/158)) ([5cf8fda](https://github.com/smart-swimmingpool/pool-controller/commit/5cf8fdaecafcc48350a6ec4008fda075af844a0c))
* **web:** use streaming multipart upload for /api/fs/upload ([0c38f50](https://github.com/smart-swimmingpool/pool-controller/commit/0c38f506be573f97f58788885ddce26a384c0420))

## [4.1.1](https://github.com/smart-swimmingpool/pool-controller/compare/v4.1.0...v4.1.1) (2026-07-07)


### Bug Fixes

* address CodeQL code scanning alerts — thread-safe time functions, path-exclude libdeps, update skills ([6ebbae1](https://github.com/smart-swimmingpool/pool-controller/commit/6ebbae1f487fcd87610931f417ae753a2649addf))
* address Codex clean-code review findings ([f1d6da2](https://github.com/smart-swimmingpool/pool-controller/commit/f1d6da2afeba26f5f4a874424584b5bc25741ec7))
* **ci:** filter .pio/libdeps from CodeQL SARIF instead of paths-ignore ([a345790](https://github.com/smart-swimmingpool/pool-controller/commit/a34579099c4df7377929652c62538702351d80ae))
* **homeassistant:** remove 'none' from preset_modes discovery config ([d05a083](https://github.com/smart-swimmingpool/pool-controller/commit/d05a08352cffb804d3eb1f0d3b8561a42273da57))

## [4.1.0](https://github.com/smart-swimmingpool/pool-controller/compare/v4.0.2...v4.1.0) (2026-07-07)


### Features

* **ha:** add circulation-extension sensor showing extra minutes beyond base timer ([0270355](https://github.com/smart-swimmingpool/pool-controller/commit/0270355afa48ab645aed4a4fd658858233d8f7cb))
* **ha:** add circulation-extension sensor showing extra minutes beyond base timer ([9339d25](https://github.com/smart-swimmingpool/pool-controller/commit/9339d25e5f03984da82c8aacd372992d531c43e7))
* **web-ui:** redesign timer display as intuitive time range instead of abstract minutes ([366d647](https://github.com/smart-swimmingpool/pool-controller/commit/366d64755e845375ae6bd0b7c6bd39d5a4f7b690))
* **web-ui:** redesign timer display as intuitive time range instead of abstract minutes ([1ced447](https://github.com/smart-swimmingpool/pool-controller/commit/1ced447730549e4a73d077ecda8857de68093a38))
* **web-ui:** show circulation extension (extra minutes) on dashboard ([69e21bf](https://github.com/smart-swimmingpool/pool-controller/commit/69e21bf1891f9178d4d556edb18d2dabc107f1c6))
* **web-ui:** show circulation extension (extra minutes) on dashboard ([e7cbc24](https://github.com/smart-swimmingpool/pool-controller/commit/e7cbc24ab9952cdaa3aaa1ee3d90c8495b6f64cf))


### Bug Fixes

* **clang-format:** apply clang-format to MqttPublisher.cpp ([8058c1d](https://github.com/smart-swimmingpool/pool-controller/commit/8058c1d5481f1be80564efa62d10fb16cda3a78c))
* **relay:** avoid int→bool ambiguity in constructor overloads ([fcc616c](https://github.com/smart-swimmingpool/pool-controller/commit/fcc616c96faa733e7fa16548c890994c37047ab0))
* **relay:** correct relay polarity for NORVI AE01-R active-HIGH relays ([6e92aa0](https://github.com/smart-swimmingpool/pool-controller/commit/6e92aa087e4a192bd3f3bf8cd8b3c42d842e02b6))
* resolve MegaLinter issues in PR [#139](https://github.com/smart-swimmingpool/pool-controller/issues/139) ([0c3cea8](https://github.com/smart-swimmingpool/pool-controller/commit/0c3cea8b528cca90fbefb282def9ca1dced5f57f))
* **test:** add missing saveSensorMapping/loadSensorMapping to mock ConfigManager ([00d349b](https://github.com/smart-swimmingpool/pool-controller/commit/00d349bbf706542b3ee14aca8467e102029bcbd3))
* **test:** add strlcat fallback for native test builds ([c6463fb](https://github.com/smart-swimmingpool/pool-controller/commit/c6463fb101c7bbd9ec6b0305abc212d0eaa30be6))
* use MegaLinter v9.6.0 (v9.7.0 does not exist as release tag) ([81b679f](https://github.com/smart-swimmingpool/pool-controller/commit/81b679f4de263caa16cc5e5cee9055356190b4fa))
* **web-ui:** use consistent muted styling for Extension telemetry item ([b0988db](https://github.com/smart-swimmingpool/pool-controller/commit/b0988dbdee031f2be3225e71d4245ee9cca21942))
* **web-ui:** use consistent muted styling for Extension telemetry item ([6ae6b99](https://github.com/smart-swimmingpool/pool-controller/commit/6ae6b99413e602cad61b708cca743d0892719baa))

## [4.0.2](https://github.com/smart-swimmingpool/pool-controller/compare/v4.0.1...v4.0.2) (2026-06-30)

### Bug Fixes

- **ci:** suppress cppcheck syntaxError for ESP-IDF version macros ([028e62a](https://github.com/smart-swimmingpool/pool-controller/commit/028e62a1c1de030fbdc07c42f53d1e7e4d18c89f))
- **config:** correct Doxygen description — uses NVS (Preferences), not LittleFS ([d1e9910](https://github.com/smart-swimmingpool/pool-controller/commit/d1e9910ba2e9327642e78b14e04bb7361cffd05d))
- correct #pragmaonce to #pragma once in RuleAuto.hpp ([6963f6c](https://github.com/smart-swimmingpool/pool-controller/commit/6963f6cc60f1e59058e5174437f8ee22f226bfe4))
- **ha:** use name-based entity IDs matching HA generation ([22c56bd](https://github.com/smart-swimmingpool/pool-controller/commit/22c56bd6c5b47562950cd36027929123c95082ac))

## [4.0.1](https://github.com/smart-swimmingpool/pool-controller/compare/v4.0.0...v4.0.1) (2026-06-28)

### Bug Fixes

- **ci:** use temp file to avoid gh release upload name collision ([d570745](https://github.com/smart-swimmingpool/pool-controller/commit/d57074557ca2314d99cf6ce8fe188a115848f646))
- **mqtt:** remove duplicate Home Assistant Discovery entities ([9ef1526](https://github.com/smart-swimmingpool/pool-controller/commit/9ef1526f3550b12cb3826bbb39af8e12823a02e3))
- **network:** remove always-true WPS function pointer check ([b0a339b](https://github.com/smart-swimmingpool/pool-controller/commit/b0a339bc40e011bd0ba74d6bbf311e9ff70e7d7f))
- **network:** remove always-true WPS function pointer check ([e9a01bf](https://github.com/smart-swimmingpool/pool-controller/commit/e9a01bfcac79cff3efab83d1c80b920381e6079e))
- **ota:** select correct firmware binary per board type ([34f93d2](https://github.com/smart-swimmingpool/pool-controller/commit/34f93d26547c37a6d60803427aad330d64b8de6a))
- **pool-controller:** stop overriding persisted TimerSetting with hardcoded values ([3cfd43f](https://github.com/smart-swimmingpool/pool-controller/commit/3cfd43fbf67e643137710a84d27b333de9e55295))
- **wps:** correct ESP-IDF version guard for esp_wifi_wps_start API ([ab98105](https://github.com/smart-swimmingpool/pool-controller/commit/ab98105cdd79088f312a577896fe9b3f584af0f4))
- **wps:** correct ESP-IDF version guard for esp_wifi_wps_start API ([8d92db8](https://github.com/smart-swimmingpool/pool-controller/commit/8d92db896bf92dc92178ed60726722f9682672ca))
- **wps:** correct ESP-IDF version guard for esp_wifi_wps_start API ([#124](https://github.com/smart-swimmingpool/pool-controller/issues/124)) ([ab98105](https://github.com/smart-swimmingpool/pool-controller/commit/ab98105cdd79088f312a577896fe9b3f584af0f4))

## [4.0.0](https://github.com/smart-swimmingpool/pool-controller/compare/v3.3.0...v4.0.0) (2026-06-25)

### ⚠ BREAKING CHANGES

- The Homie MQTT protocol has been removed entirely. The controller now exclusively uses Home Assistant MQTT Discovery.

### Features

- add Home Assistant climate/thermostat MQTT entity ([d5daeb9](https://github.com/smart-swimmingpool/pool-controller/commit/d5daeb9f7f4dcad2fb2573404db898bd765bb743))
- add mDNS responder for pool-controller.local discovery ([#95](https://github.com/smart-swimmingpool/pool-controller/issues/95)) ([b1252e7](https://github.com/smart-swimmingpool/pool-controller/commit/b1252e724750c0fa2cfaac1a25f66cc69dce9ba2))
- convert WebUI to Progressive Web App (PWA) ([b2f05d1](https://github.com/smart-swimmingpool/pool-controller/commit/b2f05d132902474539d9e92bf4d3029e42e73eda))
- **docs:** add KiCad 9.0 schematic generator and PDF exports ([#104](https://github.com/smart-swimmingpool/pool-controller/issues/104)) ([ff46191](https://github.com/smart-swimmingpool/pool-controller/commit/ff46191ddf8a77bf70babaede6aab5f7a07e1feb))
- **docs:** Add Quick Start Guide, FAQ, and Safety Warnings ([#107](https://github.com/smart-swimmingpool/pool-controller/issues/107)) ([b0152e1](https://github.com/smart-swimmingpool/pool-controller/commit/b0152e1fc735dd44811d43082a439513ba065fac))
- **mqtt:** set HA entity_category for all discovery entities ([375f40f](https://github.com/smart-swimmingpool/pool-controller/commit/375f40fe7d8fd8c9daa4c2b02fa5572ff6ea127a))
- NORVI AE01-R hardware support with OLED display ([#117](https://github.com/smart-swimmingpool/pool-controller/issues/117)) ([964a1bd](https://github.com/smart-swimmingpool/pool-controller/commit/964a1bdc746f80b196f6e48d944a3e7575046a82))
- **norvi:** add OLED display with 4 info pages, button navigation, and QR code ([34681e2](https://github.com/smart-swimmingpool/pool-controller/commit/34681e23be3a3eccf483a77d42e1d6e92a84f286))
- OLED Menu-Navigation, Sensor-Mapping in WebUI+HA, mDNS ([f72bbe2](https://github.com/smart-swimmingpool/pool-controller/commit/f72bbe2c7622259a1592afe66d52fa17f5e94fa5))
- optimierte Pin-Belegung (GPIO32/33/25/26) + Status-LED mit Homie-Blinkcodes ([c6ef387](https://github.com/smart-swimmingpool/pool-controller/commit/c6ef387ee16d64f1be46933f2d5b1b2d4116b7d1))
- temperature-based circulation time with continuous extension ([b16a9d0](https://github.com/smart-swimmingpool/pool-controller/commit/b16a9d04251b28f5a5ebf0e2907b64bdaae4ca3d))
- **ui:** add About section in More bottom sheet ([5919624](https://github.com/smart-swimmingpool/pool-controller/commit/59196247d43e934cb7b8ea60f642ed57f86320bb))
- **ui:** iOS-style bottom tab bar with glassmorphism ([38eec43](https://github.com/smart-swimmingpool/pool-controller/commit/38eec4356f103e2711a2fa0165a09ba23fe43013))
- **web:** add temperature-based circulation parameters to Web UI ([963bcde](https://github.com/smart-swimmingpool/pool-controller/commit/963bcde09209ebabb8f0c42bb17d1fe3491e77cc))
- **web:** format effective runtime as duration in WebUI and HA ([b460b4f](https://github.com/smart-swimmingpool/pool-controller/commit/b460b4f5fea1bc054123adfd166d26f109309965))

### Bug Fixes

- add native test infrastructure with ASan and coverage reporting ([#98](https://github.com/smart-swimmingpool/pool-controller/issues/98)) ([1421a39](https://github.com/smart-swimmingpool/pool-controller/commit/1421a399b006d374d3e792495be230f1e73b02aa))
- docs synced de/en ([909304d](https://github.com/smart-swimmingpool/pool-controller/commit/909304d27c94429fd9031a9f20a4c8ef6939d67c))
- **ha:** select-Entity-Status ohne Temperatur publishen ([e6f32d7](https://github.com/smart-swimmingpool/pool-controller/commit/e6f32d70847f71d84468a25caf0756b1a72d0e59))
- **ha:** Sensor-Mapping Discovery bei jedem MQTT-Reconnect publishen ([1ab82d9](https://github.com/smart-swimmingpool/pool-controller/commit/1ab82d99c806b2b8f7923b26e0c33a6fe6696708))
- **mqtt:** prevent dangling pointer and duplicate callback on reconnect ([25090d7](https://github.com/smart-swimmingpool/pool-controller/commit/25090d772cbe6c58cba178e87a4e16bf318648e6))
- restore telemetry updates and HA entity visibility ([716dfe6](https://github.com/smart-swimmingpool/pool-controller/commit/716dfe66a49507b6a0cb33b904c28bfb6101e171))
- review findings for temperature-based circulation ([8784f25](https://github.com/smart-swimmingpool/pool-controller/commit/8784f2506e5b0798759c76631a1703773c479efc))
- **ui:** display dashboard thresholds without-auth via /api/status, 4-across mode cards ([e68d4bf](https://github.com/smart-swimmingpool/pool-controller/commit/e68d4bfb268f6ab02ac15ce3186ecbe51ffc05ed))
- **web:** clarify that timer start/end also applies in Auto mode ([5985a00](https://github.com/smart-swimmingpool/pool-controller/commit/5985a008e3a6168fa9b291c5210b3224eb2dd014))
- **web:** remove leftover merge conflict markers in index.html ([#118](https://github.com/smart-swimmingpool/pool-controller/issues/118)) ([907b676](https://github.com/smart-swimmingpool/pool-controller/commit/907b67695357724857c8a73b54bd228dbf5d4459))

### Miscellaneous Chores

- remove deprecated Homie references across codebase ([1df0705](https://github.com/smart-swimmingpool/pool-controller/commit/1df070560d0441c365ecd11ef86649976468a8b3))

## [3.3.0](https://github.com/smart-swimmingpool/pool-controller/compare/v3.2.0...v3.3.0) (2026-06-06)

### Features

- Cleanup and fixes
  ([#72](https://github.com/smart-swimmingpool/pool-controller/issues/72))
  ([90d6e07](https://github.com/smart-swimmingpool/pool-controller/commit/90d6e07383b0f26f9675ba7f25e306bf4d5b3b51))
- **ha:** Replace timer H/Min number entities with single HH:MM text entities
  ([#74](https://github.com/smart-swimmingpool/pool-controller/issues/74))
  ([27c337f](https://github.com/smart-swimmingpool/pool-controller/commit/27c337fd1ed311cbe2752aa64ad76967adeccead))
- NTP server config via Web UI + MQTT, local time display
  ([#83](https://github.com/smart-swimmingpool/pool-controller/issues/83))
  ([c45bd55](https://github.com/smart-swimmingpool/pool-controller/commit/c45bd55ad86564929a08b274fbec1e6ff8a9ad14))
- NVS config backup, MQTT error reporting, web UI improvements
  ([#82](https://github.com/smart-swimmingpool/pool-controller/issues/82))
  ([817500a](https://github.com/smart-swimmingpool/pool-controller/commit/817500a0332a15a1f9425737d78c7c3b25b3396b))
- OTA update from GitHub releases, semver release management, config safety
  ([#77](https://github.com/smart-swimmingpool/pool-controller/issues/77))
  ([2972b34](https://github.com/smart-swimmingpool/pool-controller/commit/2972b3493d6bb9c2b8416fd281bdae76bf75ea70))
- restructure web UI settings tabs and HA entity categories
  ([#87](https://github.com/smart-swimmingpool/pool-controller/issues/87))
  ([712b037](https://github.com/smart-swimmingpool/pool-controller/commit/712b03763a32e6417c9ab5f7322bcaddb6b85668))

### Bug Fixes

- mount LittleFS during web portal startup
  ([#85](https://github.com/smart-swimmingpool/pool-controller/issues/85))
  ([592a4c8](https://github.com/smart-swimmingpool/pool-controller/commit/592a4c8105b3ebf54391a371650100e08fbebab1))

### Miscellaneous

- **refactor:** migrate config from LittleFS JSON to NVS/Preferences
  ([#84](https://github.com/smart-swimmingpool/pool-controller/issues/84))
  ([ba841f5](https://github.com/smart-swimmingpool/pool-controller/commit/ba841f56e7a360c2ab10f4bccfa53ee3e8a4ffe2))

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

- **Web Dashboard**: Built-in configuration interface with:

  - Pool temperature and mode controls
  - Timer configuration
  - Timezone and NTP settings
  - System information display
  - Firmware update management

- **Home Assistant MQTT Discovery**: Automatic device registration via Homie
  convention

- **Configurable WiFi Provisioning**: Support for WPS and fallback Access Point
  mode

### Changed

- Migrated from manual MQTT topics to Homie 3.0.1 convention for standardized
  IoT device communication

### Fixed

- Timer scheduling issues with timezone handling
- MQTT reconnection stability improvements
- Memory optimization for long-term operation

## [3.0.0] - 2025-11-11

### Changed

- Complete rewrite from Arduino to PlatformIO with ESP-IDF 5.x framework
- Migrated from ESPAsyncWebServer to built-in ESP-IDF HTTP server
- Migrated from AsyncMqttClient to ESP-MQTT (esp_mqtt)
- Migrated from ArduinoJson to ESP-IDF JSON (cJSON)
- Removed Homie dependency — now uses MQTT directly

### Added

- FreeRTOS task watchdog with 30s timeout
- Boot-loop detection with automatic safe mode
- System degradation management with graceful fallbacks
- Temperature hysteresis configuration
- Solar heating control with configurable thresholds

### Removed

- Arduino framework dependency
- Homie library dependency
- ESPAsyncWebServer dependency
- AsyncMqttClient dependency

## [2.0.0] - 2024-08-01

### Added

- Initial Arduino-based ESP32 pool controller implementation
- Temperature monitoring via DS18B20 sensors
- Relay control for pool pump and solar heating
- Timer scheduling for pool pump operation
- Web dashboard for configuration and monitoring
- MQTT integration with Homie convention
- OTA firmware updates
- Home Assistant discovery support
