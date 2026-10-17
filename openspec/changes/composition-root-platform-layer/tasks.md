# Tasks

- [ ] Define a board platform/profile contract for pins, relay polarity, Dallas topology and local-I/O capabilities.
- [ ] Add concrete composition for generic ESP32, NORVI AE01-R and Olimex ESP32-C6-EVB.
- [ ] Move board-specific object construction out of `PoolController.cpp`.
- [ ] Make `PoolControllerContext` the explicit owner of application/runtime components.
- [ ] Replace remaining `Nodes.hpp` mutable-global access with explicit dependencies.
- [ ] Convert static managers to owned instances only where lifecycle/testability benefits justify it.
- [ ] Confine board-selection `#ifdef` branches to platform/configuration composition.
- [ ] Remove mutable namespace-global controller objects.
- [ ] Update native mocks to construct dependency graphs explicitly.
- [ ] Move files into final layer directories only after dependency cleanup is complete.
- [ ] Verify native tests and all PlatformIO board builds.
- [ ] Run hardware smoke tests for sensors, relays, local UI, MQTT, Web and OTA (see the hardware gates below).

## Hardware gates

Measured on `esp32dev`, `norvi_ae01_r` and the single-core `olimex_esp32_c6_evb`; a gate fails on any board.

- [ ] NVS write failure: with injected open/write failures, every command returns `PERSIST_FAILED` and the
  runtime state is unchanged.
- [ ] Power loss: cut power during 20 consecutive settings writes; the device boots every time with a
  valid configuration and the relays in their safe default.
- [ ] Reconnect soak: 72 h with forced WiFi and MQTT disconnects every 30 min; no reboot, the minimum free
  heap stays above the low-heap threshold, and Home Assistant entities recover after each reconnect.
- [ ] Watchdog and latency: 0 task-WDT resets and worst-case `loop()` iteration ≤ 500 ms during the soak.
