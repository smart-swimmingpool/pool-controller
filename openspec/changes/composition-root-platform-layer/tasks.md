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
- [ ] Run hardware smoke tests for sensors, relays, local UI, MQTT, Web and OTA.
