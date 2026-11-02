# Tasks

- [ ] Introduce a physical `DallasBus` abstraction with no logical pool/solar role.
- [ ] Add native tests for shared-bus and dedicated-bus topology planning.
- [ ] Move discovery/rescan/ROM enumeration into `DallasBus`.
- [ ] Move role-to-ROM mapping into an explicit value/configuration object.
- [ ] Route runtime mapping changes through the Core-0 sensor command boundary.
- [ ] Publish one complete sensor generation with timestamp and generation number.
- [ ] Preserve reading filtering and 5-second recovery behavior.
- [ ] Remove `sharedSensor_`, `isBusMaster_` and topology branching from logical sensor nodes.
- [ ] Migrate Web/MQTT/UI discovery views to cached value snapshots only.
- [ ] Verify all PlatformIO builds.
- [ ] Verify real NORVI shared-bus hardware.
- [ ] Verify real dedicated-bus ESP32 hardware.

## Hardware gates

Measured on `esp32dev`, `norvi_ae01_r` and the single-core `olimex_esp32_c6_evb`; a gate fails on any board.

- [ ] Sensor reconnect: unplug and replug each probe 10 times; the temperature is invalid while it is
  disconnected and valid again within two rescan cycles after reconnecting.
- [ ] Role stability: across all reconnects and reboots, pool and solar roles never swap ROM addresses
  (NORVI shared bus included).
- [ ] Loop latency during rescan: worst-case `loop()` iteration ≤ 500 ms while a bus rescan runs (C6 included).
