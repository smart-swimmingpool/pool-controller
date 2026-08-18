# Changelog

All notable changes to this project will be documented in this file.

## [5.0.0](https://github.com/smart-swimmingpool/pool-controller/compare/v4.2.1...v5.0.0) (2026-08-18)


### ⚠ BREAKING CHANGES

* The Homie MQTT protocol has been removed entirely. The controller now exclusively uses Home Assistant MQTT Discovery.

### Features

* add Home Assistant climate/thermostat MQTT entity ([d5daeb9](https://github.com/smart-swimmingpool/pool-controller/commit/d5daeb9f7f4dcad2fb2573404db898bd765bb743))
* add mDNS responder for pool-controller.local discovery ([#95](https://github.com/smart-swimmingpool/pool-controller/issues/95)) ([b1252e7](https://github.com/smart-swimmingpool/pool-controller/commit/b1252e724750c0fa2cfaac1a25f66cc69dce9ba2))
* add Olimex ESP32-C6 local UI ([#175](https://github.com/smart-swimmingpool/pool-controller/issues/175)) ([2777cc9](https://github.com/smart-swimmingpool/pool-controller/commit/2777cc960a9d41afd77fe8edf43ec1fd54df1f02))
* **calibration:** guided NORVI button calibration wizard ([#182](https://github.com/smart-swimmingpool/pool-controller/issues/182)) ([c4fc499](https://github.com/smart-swimmingpool/pool-controller/commit/c4fc499f458d20010e525050c7832c4dfa817675))
* change default MQTT protocol to HomeAssistant ([5d4cad9](https://github.com/smart-swimmingpool/pool-controller/commit/5d4cad9a3173f67e0d3db6c8722207f1e91a8a80))
* Cleanup and fixes ([#72](https://github.com/smart-swimmingpool/pool-controller/issues/72)) ([90d6e07](https://github.com/smart-swimmingpool/pool-controller/commit/90d6e07383b0f26f9675ba7f25e306bf4d5b3b51))
* convert WebUI to Progressive Web App (PWA) ([b2f05d1](https://github.com/smart-swimmingpool/pool-controller/commit/b2f05d132902474539d9e92bf4d3029e42e73eda))
* **display:** action menu, wizard-cancel, and step indicator ([b0d717d](https://github.com/smart-swimmingpool/pool-controller/commit/b0d717dfedc51fe695e942982ea18f02f8ceb90b))
* **display:** horizontal text scrolling for long strings ([c7e03fb](https://github.com/smart-swimmingpool/pool-controller/commit/c7e03fbf34425185e0276d1193437ee524098925))
* **display:** long-press progress bar and auto-return warning ([9b49135](https://github.com/smart-swimmingpool/pool-controller/commit/9b49135eccfa60f8bcb8b70832909db45c5899fd))
* **display:** merge Aktionsmenü, Wizard-Cancel und Schritt-Indikator ([#148](https://github.com/smart-swimmingpool/pool-controller/issues/148)) ([4ac6cc2](https://github.com/smart-swimmingpool/pool-controller/commit/4ac6cc2d1ee8920c03cb40bd05ebce59a3fb59db))
* **display:** merge Hint-Labels, Footer-Format und S1-Wrap ([#147](https://github.com/smart-swimmingpool/pool-controller/issues/147)) ([7264a33](https://github.com/smart-swimmingpool/pool-controller/commit/7264a33645e6ee9fad4bd203524abf28cfceb4c3))
* **display:** merge Long-Press-Fortschrittsbalken und Auto-Return-Warnung ([#149](https://github.com/smart-swimmingpool/pool-controller/issues/149)) ([bc3e02e](https://github.com/smart-swimmingpool/pool-controller/commit/bc3e02e186910f986a87ead37f356439c7549d3c))
* **display:** update hint labels, footer page format, and S1 wrap ([1da577a](https://github.com/smart-swimmingpool/pool-controller/commit/1da577a83928df0bab392dc9926278b625ea97ad))
* **docs:** add KiCad 9.0 schematic generator and PDF exports ([#104](https://github.com/smart-swimmingpool/pool-controller/issues/104)) ([ff46191](https://github.com/smart-swimmingpool/pool-controller/commit/ff46191ddf8a77bf70babaede6aab5f7a07e1feb))
* **docs:** Add Quick Start Guide, FAQ, and Safety Warnings ([#107](https://github.com/smart-swimmingpool/pool-controller/issues/107)) ([b0152e1](https://github.com/smart-swimmingpool/pool-controller/commit/b0152e1fc735dd44811d43082a439513ba065fac))
* **ha:** add circulation-extension sensor showing extra minutes beyond base timer ([0270355](https://github.com/smart-swimmingpool/pool-controller/commit/0270355afa48ab645aed4a4fd658858233d8f7cb))
* **ha:** add circulation-extension sensor showing extra minutes beyond base timer ([9339d25](https://github.com/smart-swimmingpool/pool-controller/commit/9339d25e5f03984da82c8aacd372992d531c43e7))
* **ha:** add display precision, circulating action, outlet class, MQTT sensor, cleaner names ([85c1ffd](https://github.com/smart-swimmingpool/pool-controller/commit/85c1ffd3725cdcfbf0dffa37be515a52da46f8d3))
* **ha:** add state_class, binary_sensor, RSSI class, and climate preset modes ([94daa52](https://github.com/smart-swimmingpool/pool-controller/commit/94daa5213f876b74921383e247100796877b38fd))
* **ha:** Replace timer H/Min number entities with single HH:MM text entities ([#74](https://github.com/smart-swimmingpool/pool-controller/issues/74)) ([27c337f](https://github.com/smart-swimmingpool/pool-controller/commit/27c337fd1ed311cbe2752aa64ad76967adeccead))
* **logging:** add log console view (LogCapture ring buffer, REST /api/logs, MQTT event export) ([#169](https://github.com/smart-swimmingpool/pool-controller/issues/169)) ([93cc7a0](https://github.com/smart-swimmingpool/pool-controller/commit/93cc7a0c875a2006902e2e8aa625faddaa0201c0))
* **mqtt:** set HA entity_category for all discovery entities ([375f40f](https://github.com/smart-swimmingpool/pool-controller/commit/375f40fe7d8fd8c9daa4c2b02fa5572ff6ea127a))
* NORVI AE01-R hardware support with OLED display ([#117](https://github.com/smart-swimmingpool/pool-controller/issues/117)) ([964a1bd](https://github.com/smart-swimmingpool/pool-controller/commit/964a1bdc746f80b196f6e48d944a3e7575046a82))
* **norvi:** add OLED display with 4 info pages, button navigation, and QR code ([34681e2](https://github.com/smart-swimmingpool/pool-controller/commit/34681e23be3a3eccf483a77d42e1d6e92a84f286))
* NTP server config via Web UI + MQTT, local time display ([#83](https://github.com/smart-swimmingpool/pool-controller/issues/83)) ([c45bd55](https://github.com/smart-swimmingpool/pool-controller/commit/c45bd55ad86564929a08b274fbec1e6ff8a9ad14))
* NVS config backup, MQTT error reporting, web UI improvements ([#82](https://github.com/smart-swimmingpool/pool-controller/issues/82)) ([817500a](https://github.com/smart-swimmingpool/pool-controller/commit/817500a0332a15a1f9425737d78c7c3b25b3396b))
* OLED Menu-Navigation, Sensor-Mapping in WebUI+HA, mDNS ([f72bbe2](https://github.com/smart-swimmingpool/pool-controller/commit/f72bbe2c7622259a1592afe66d52fa17f5e94fa5))
* optimierte Pin-Belegung (GPIO32/33/25/26) + Status-LED mit Homie-Blinkcodes ([c6ef387](https://github.com/smart-swimmingpool/pool-controller/commit/c6ef387ee16d64f1be46933f2d5b1b2d4116b7d1))
* OTA update from GitHub releases, semver release management, config safety ([#77](https://github.com/smart-swimmingpool/pool-controller/issues/77)) ([2972b34](https://github.com/smart-swimmingpool/pool-controller/commit/2972b3493d6bb9c2b8416fd281bdae76bf75ea70))
* restructure web UI settings tabs and HA entity categories ([#87](https://github.com/smart-swimmingpool/pool-controller/issues/87)) ([712b037](https://github.com/smart-swimmingpool/pool-controller/commit/712b03763a32e6417c9ab5f7322bcaddb6b85668))
* semver release pipeline via release-please (Conventional Commits) ([#29](https://github.com/smart-swimmingpool/pool-controller/issues/29)) ([102b18b](https://github.com/smart-swimmingpool/pool-controller/commit/102b18b64af6364e59147baf462b970cc73e5d7d))
* **system:** log concrete reason when entering safe mode ([#179](https://github.com/smart-swimmingpool/pool-controller/issues/179)) ([e7ac809](https://github.com/smart-swimmingpool/pool-controller/commit/e7ac809bf0a24219843f29c21689e924692d14c6))
* temperature-based circulation time with continuous extension ([b16a9d0](https://github.com/smart-swimmingpool/pool-controller/commit/b16a9d04251b28f5a5ebf0e2907b64bdaae4ca3d))
* **ui:** add About section in More bottom sheet ([5919624](https://github.com/smart-swimmingpool/pool-controller/commit/59196247d43e934cb7b8ea60f642ed57f86320bb))
* **ui:** iOS-style bottom tab bar with glassmorphism ([38eec43](https://github.com/smart-swimmingpool/pool-controller/commit/38eec4356f103e2711a2fa0165a09ba23fe43013))
* **web-ui:** redesign timer display as intuitive time range instead of abstract minutes ([366d647](https://github.com/smart-swimmingpool/pool-controller/commit/366d64755e845375ae6bd0b7c6bd39d5a4f7b690))
* **web-ui:** redesign timer display as intuitive time range instead of abstract minutes ([1ced447](https://github.com/smart-swimmingpool/pool-controller/commit/1ced447730549e4a73d077ecda8857de68093a38))
* **web-ui:** show circulation extension (extra minutes) on dashboard ([69e21bf](https://github.com/smart-swimmingpool/pool-controller/commit/69e21bf1891f9178d4d556edb18d2dabc107f1c6))
* **web-ui:** show circulation extension (extra minutes) on dashboard ([e7cbc24](https://github.com/smart-swimmingpool/pool-controller/commit/e7cbc24ab9952cdaa3aaa1ee3d90c8495b6f64cf))
* **web:** add /api/fs/upload endpoint for OTA-safe web asset deployment ([cee0b4a](https://github.com/smart-swimmingpool/pool-controller/commit/cee0b4aad3eedcc8e322086898e15bea18f87fca))
* **web:** add temperature-based circulation parameters to Web UI ([963bcde](https://github.com/smart-swimmingpool/pool-controller/commit/963bcde09209ebabb8f0c42bb17d1fe3491e77cc))
* **web:** format effective runtime as duration in WebUI and HA ([b460b4f](https://github.com/smart-swimmingpool/pool-controller/commit/b460b4f5fea1bc054123adfd166d26f109309965))
* **web:** make dashboard and operating parameters visible without login ([919bbde](https://github.com/smart-swimmingpool/pool-controller/commit/919bbde875d92ddf53a2a831bbcb356528344cc7))
* **web:** replace pump text toggle with visible toggle-switch controls ([#166](https://github.com/smart-swimmingpool/pool-controller/issues/166)) ([e37e7bd](https://github.com/smart-swimmingpool/pool-controller/commit/e37e7bdcc6882590187b1172c1f51fc10772fadd))
* **wifi:** add WPS onboarding for initial WiFi provisioning ([#67](https://github.com/smart-swimmingpool/pool-controller/issues/67)) ([6983af6](https://github.com/smart-swimmingpool/pool-controller/commit/6983af6f68110b1e38e286d5c28fdcc1becb5496))


### Bug Fixes

* add native test infrastructure with ASan and coverage reporting ([#98](https://github.com/smart-swimmingpool/pool-controller/issues/98)) ([1421a39](https://github.com/smart-swimmingpool/pool-controller/commit/1421a399b006d374d3e792495be230f1e73b02aa))
* address CodeQL code scanning alerts — thread-safe time functions, path-exclude libdeps, update skills ([6ebbae1](https://github.com/smart-swimmingpool/pool-controller/commit/6ebbae1f487fcd87610931f417ae753a2649addf))
* address Codex clean-code review findings ([f1d6da2](https://github.com/smart-swimmingpool/pool-controller/commit/f1d6da2afeba26f5f4a874424584b5bc25741ec7))
* **button:** Add ADC filtering and improved debouncing for NORVI front panel buttons ([#177](https://github.com/smart-swimmingpool/pool-controller/issues/177)) ([206954b](https://github.com/smart-swimmingpool/pool-controller/commit/206954babbe94858c5de4b51cceebfb70d78307b))
* **button:** stop phantom NORVI operation-mode switches ([#181](https://github.com/smart-swimmingpool/pool-controller/issues/181)) ([cdf4083](https://github.com/smart-swimmingpool/pool-controller/commit/cdf4083098adbc2ee9034d6755a89e3d93e3276e))
* **ci:** add checkout step to auto-merge-release workflow ([bd19bea](https://github.com/smart-swimmingpool/pool-controller/commit/bd19bea4ee4773a18cf7896870301ef4648e070c))
* **ci:** filter .pio/libdeps from CodeQL SARIF instead of paths-ignore ([a345790](https://github.com/smart-swimmingpool/pool-controller/commit/a34579099c4df7377929652c62538702351d80ae))
* **ci:** fix native test build and clang-format violations ([ba6a1a8](https://github.com/smart-swimmingpool/pool-controller/commit/ba6a1a864d741e84ed9ab9632290de0cbc0aaac4))
* **ci:** make website dispatch workflow non-blocking on token issues ([#69](https://github.com/smart-swimmingpool/pool-controller/issues/69)) ([4e03bd3](https://github.com/smart-swimmingpool/pool-controller/commit/4e03bd3a1e0eb71b7c3538e60a5da05d03a1a35a))
* **ci:** suppress cppcheck syntaxError for ESP-IDF version macros ([028e62a](https://github.com/smart-swimmingpool/pool-controller/commit/028e62a1c1de030fbdc07c42f53d1e7e4d18c89f))
* **ci:** use temp file to avoid gh release upload name collision ([d570745](https://github.com/smart-swimmingpool/pool-controller/commit/d57074557ca2314d99cf6ce8fe188a115848f646))
* **clang-format:** apply clang-format to MqttPublisher.cpp ([8058c1d](https://github.com/smart-swimmingpool/pool-controller/commit/8058c1d5481f1be80564efa62d10fb16cda3a78c))
* **config:** correct Doxygen description — uses NVS (Preferences), not LittleFS ([d1e9910](https://github.com/smart-swimmingpool/pool-controller/commit/d1e9910ba2e9327642e78b14e04bb7361cffd05d))
* correct #pragmaonce to #pragma once in RuleAuto.hpp ([6963f6c](https://github.com/smart-swimmingpool/pool-controller/commit/6963f6cc60f1e59058e5174437f8ee22f226bfe4))
* **display:** ADC threshold calibration and QR code sizing ([ca6810f](https://github.com/smart-swimmingpool/pool-controller/commit/ca6810f5b4777eb7fc3227a9aad3e3643b1d85c4))
* **display:** address PR [#147](https://github.com/smart-swimmingpool/pool-controller/issues/147) review comments ([fddff74](https://github.com/smart-swimmingpool/pool-controller/commit/fddff74d37f67dfead71dad64f1b3c86377efa8e))
* docs synced de/en ([909304d](https://github.com/smart-swimmingpool/pool-controller/commit/909304d27c94429fd9031a9f20a4c8ef6939d67c))
* **docs:** align HA entity table with name-based generation, fix domain errors ([#157](https://github.com/smart-swimmingpool/pool-controller/issues/157)) ([689c24e](https://github.com/smart-swimmingpool/pool-controller/commit/689c24e957cb13e38ecaadd13e32f843d49a115b))
* **docs:** correct relative paths for images and links ([#164](https://github.com/smart-swimmingpool/pool-controller/issues/164)) ([1870c3e](https://github.com/smart-swimmingpool/pool-controller/commit/1870c3eb93413bfd140fa14ca08bc74f91b0fdf0))
* ESP32 compatibility, memory & buffer fixes + AGENTS.md update ([#68](https://github.com/smart-swimmingpool/pool-controller/issues/68)) ([b95efbd](https://github.com/smart-swimmingpool/pool-controller/commit/b95efbdbd946c99142c5f688a13c1bf77aa1e7db))
* **ha:** select-Entity-Status ohne Temperatur publishen ([e6f32d7](https://github.com/smart-swimmingpool/pool-controller/commit/e6f32d70847f71d84468a25caf0756b1a72d0e59))
* **ha:** Sensor-Mapping Discovery bei jedem MQTT-Reconnect publishen ([1ab82d9](https://github.com/smart-swimmingpool/pool-controller/commit/1ab82d99c806b2b8f7923b26e0c33a6fe6696708))
* **ha:** use name-based entity IDs matching HA generation ([22c56bd](https://github.com/smart-swimmingpool/pool-controller/commit/22c56bd6c5b47562950cd36027929123c95082ac))
* **homeassistant:** remove 'none' from preset_modes discovery config ([d05a083](https://github.com/smart-swimmingpool/pool-controller/commit/d05a08352cffb804d3eb1f0d3b8561a42273da57))
* improve English grammar and word choice in documentation ([#47](https://github.com/smart-swimmingpool/pool-controller/issues/47)) ([2d71dcd](https://github.com/smart-swimmingpool/pool-controller/commit/2d71dcdf5081f0650fa1ad05ce9b447d95806c41))
* **logging:** trace operation mode command sources ([#174](https://github.com/smart-swimmingpool/pool-controller/issues/174)) ([dd91530](https://github.com/smart-swimmingpool/pool-controller/commit/dd91530253f2752808bf3f6b646dbfc8b199a43b))
* master Branch renamed to main ([f0d0223](https://github.com/smart-swimmingpool/pool-controller/commit/f0d0223ab34884d903e8ef541ae24fc02328faf6))
* mount LittleFS during web portal startup ([#85](https://github.com/smart-swimmingpool/pool-controller/issues/85)) ([592a4c8](https://github.com/smart-swimmingpool/pool-controller/commit/592a4c8105b3ebf54391a371650100e08fbebab1))
* move solar pump relay to R2 (GPIO13), add RC snubber docs ([#165](https://github.com/smart-swimmingpool/pool-controller/issues/165)) ([5f8b964](https://github.com/smart-swimmingpool/pool-controller/commit/5f8b964894c9e639b1b3c8aa923b14727668243c))
* **mqtt:** prevent dangling pointer and duplicate callback on reconnect ([25090d7](https://github.com/smart-swimmingpool/pool-controller/commit/25090d772cbe6c58cba178e87a4e16bf318648e6))
* **mqtt:** remove duplicate Home Assistant Discovery entities ([9ef1526](https://github.com/smart-swimmingpool/pool-controller/commit/9ef1526f3550b12cb3826bbb39af8e12823a02e3))
* **network:** remove always-true WPS function pointer check ([b0a339b](https://github.com/smart-swimmingpool/pool-controller/commit/b0a339bc40e011bd0ba74d6bbf311e9ff70e7d7f))
* **network:** remove always-true WPS function pointer check ([e9a01bf](https://github.com/smart-swimmingpool/pool-controller/commit/e9a01bfcac79cff3efab83d1c80b920381e6079e))
* **norvi:** move solar pump relay from R1 (GPIO12) to R5 (GPIO33) ([#158](https://github.com/smart-swimmingpool/pool-controller/issues/158)) ([5cf8fda](https://github.com/smart-swimmingpool/pool-controller/commit/5cf8fdaecafcc48350a6ec4008fda075af844a0c))
* **ota:** select correct firmware binary per board type ([34f93d2](https://github.com/smart-swimmingpool/pool-controller/commit/34f93d26547c37a6d60803427aad330d64b8de6a))
* **pool-controller:** stop overriding persisted TimerSetting with hardcoded values ([3cfd43f](https://github.com/smart-swimmingpool/pool-controller/commit/3cfd43fbf67e643137710a84d27b333de9e55295))
* **relay:** avoid int→bool ambiguity in constructor overloads ([fcc616c](https://github.com/smart-swimmingpool/pool-controller/commit/fcc616c96faa733e7fa16548c890994c37047ab0))
* **relay:** correct relay polarity for NORVI AE01-R active-HIGH relays ([6e92aa0](https://github.com/smart-swimmingpool/pool-controller/commit/6e92aa087e4a192bd3f3bf8cd8b3c42d842e02b6))
* Remove space between platform name and version specifier in nodemcuv2 environment ([0f34a68](https://github.com/smart-swimmingpool/pool-controller/commit/0f34a689038d78021b88aba3c528cf8c66bdd17c))
* resolve MegaLinter issues in PR [#139](https://github.com/smart-swimmingpool/pool-controller/issues/139) ([0c3cea8](https://github.com/smart-swimmingpool/pool-controller/commit/0c3cea8b528cca90fbefb282def9ca1dced5f57f))
* restore telemetry updates and HA entity visibility ([716dfe6](https://github.com/smart-swimmingpool/pool-controller/commit/716dfe66a49507b6a0cb33b904c28bfb6101e171))
* review findings for temperature-based circulation ([8784f25](https://github.com/smart-swimmingpool/pool-controller/commit/8784f2506e5b0798759c76631a1703773c479efc))
* sensor setup UX, button thresholds, QR page layout ([7028689](https://github.com/smart-swimmingpool/pool-controller/commit/70286894c8f9048508c2f984f25e3a655aa5b731))
* **sensor:** apply address filter to running instance immediately ([#160](https://github.com/smart-swimmingpool/pool-controller/issues/160)) ([cbaad12](https://github.com/smart-swimmingpool/pool-controller/commit/cbaad120bed421c4921cdf01a28c4d51556277b9))
* solar pump hysteresis at max pool temperature ([c5a4e76](https://github.com/smart-swimmingpool/pool-controller/commit/c5a4e763fefbab08140bea709b54be8348843893))
* **test:** add missing saveSensorMapping/loadSensorMapping to mock ConfigManager ([00d349b](https://github.com/smart-swimmingpool/pool-controller/commit/00d349bbf706542b3ee14aca8467e102029bcbd3))
* **test:** add strlcat fallback for native test builds ([c6463fb](https://github.com/smart-swimmingpool/pool-controller/commit/c6463fb101c7bbd9ec6b0305abc212d0eaa30be6))
* **timer:** keep pump running when extension wraps past midnight ([#180](https://github.com/smart-swimmingpool/pool-controller/issues/180)) ([2a8f919](https://github.com/smart-swimmingpool/pool-controller/commit/2a8f91993e890dd12ffb54a7d22b68c417ef764f))
* **ui:** display dashboard thresholds without-auth via /api/status, 4-across mode cards ([e68d4bf](https://github.com/smart-swimmingpool/pool-controller/commit/e68d4bfb268f6ab02ac15ce3186ecbe51ffc05ed))
* use MegaLinter v9.6.0 (v9.7.0 does not exist as release tag) ([81b679f](https://github.com/smart-swimmingpool/pool-controller/commit/81b679f4de263caa16cc5e5cee9055356190b4fa))
* **web-ui:** use consistent muted styling for Extension telemetry item ([b0988db](https://github.com/smart-swimmingpool/pool-controller/commit/b0988dbdee031f2be3225e71d4245ee9cca21942))
* **web-ui:** use consistent muted styling for Extension telemetry item ([6ae6b99](https://github.com/smart-swimmingpool/pool-controller/commit/6ae6b99413e602cad61b708cca743d0892719baa))
* **web:** clarify that timer start/end also applies in Auto mode ([5985a00](https://github.com/smart-swimmingpool/pool-controller/commit/5985a008e3a6168fa9b291c5210b3224eb2dd014))
* **web:** correct license text from GPL-3.0 to MIT in dashboard footer ([1d22f05](https://github.com/smart-swimmingpool/pool-controller/commit/1d22f05d95d2929c63e5ff63f63e48b32dda958d))
* **web:** make /api/fs/upload work by also accepting upload.filename ([09765b8](https://github.com/smart-swimmingpool/pool-controller/commit/09765b80922853933f3629fc3e439ee155828577))
* **web:** remove leftover merge conflict markers in index.html ([#118](https://github.com/smart-swimmingpool/pool-controller/issues/118)) ([907b676](https://github.com/smart-swimmingpool/pool-controller/commit/907b67695357724857c8a73b54bd228dbf5d4459))
* **web:** stop tabs from reopening WiFi settings after navigation ([726bde4](https://github.com/smart-swimmingpool/pool-controller/commit/726bde45fcf32aceab80d6751d8f33430b223d8f))
* **web:** use streaming multipart upload for /api/fs/upload ([0c38f50](https://github.com/smart-swimmingpool/pool-controller/commit/0c38f506be573f97f58788885ddce26a384c0420))
* **wps:** correct ESP-IDF version guard for esp_wifi_wps_start API ([ab98105](https://github.com/smart-swimmingpool/pool-controller/commit/ab98105cdd79088f312a577896fe9b3f584af0f4))
* **wps:** correct ESP-IDF version guard for esp_wifi_wps_start API ([8d92db8](https://github.com/smart-swimmingpool/pool-controller/commit/8d92db896bf92dc92178ed60726722f9682672ca))
* **wps:** correct ESP-IDF version guard for esp_wifi_wps_start API ([#124](https://github.com/smart-swimmingpool/pool-controller/issues/124)) ([ab98105](https://github.com/smart-swimmingpool/pool-controller/commit/ab98105cdd79088f312a577896fe9b3f584af0f4))


### Performance Improvements

* **web:** optimize PWA startup ([#184](https://github.com/smart-swimmingpool/pool-controller/issues/184)) ([3d61ebd](https://github.com/smart-swimmingpool/pool-controller/commit/3d61ebd096b9dcd2bd1b2a8f0ee07215745e2693))


### Miscellaneous Chores

* remove deprecated Homie references across codebase ([1df0705](https://github.com/smart-swimmingpool/pool-controller/commit/1df070560d0441c365ecd11ef86649976468a8b3))

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
