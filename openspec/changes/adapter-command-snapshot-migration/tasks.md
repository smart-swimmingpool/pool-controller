# Tasks

- [x] Introduce a single Core-1 controller command handler for the complete runtime command contract.
- [x] Route processed MQTT runtime commands through typed `ControllerCommand` values.
- [x] Preserve independent MQTT timer-start and timer-end updates without stale-state reconstruction.
- [x] Route `/api/mode` and `/api/pump` Web mutations through the same command handler using owner-resolved relative actions.
- [ ] Route the remaining Web runtime mutations through the same command handler.
- [ ] Route local UI runtime mutations through the same command handler.
- [x] Keep provisioning/authentication secrets and specialized calibration workflows behind their dedicated service boundaries.
- [x] Publish `SystemSnapshot` from the Core-1 owner after state updates.
- [x] Populate sensor mapping address/configured/found state in the snapshot.
- [x] Populate GREEN/YELLOW/RED time degradation and runtime controller settings in the snapshot.
- [ ] Migrate MQTT state/discovery reads to snapshot values where applicable.
- [ ] Migrate Web status/config read paths to snapshot values where applicable.
- [ ] Migrate OLED/TFT read paths to snapshot values.
- [ ] Remove direct mutable `Nodes.hpp` and runtime-setting access from migrated adapters.
- [ ] Remove `MqttCommandQueue` after typed callback hand-over has equivalent tests.
- [x] Add native regression tests for dispatch, overflow, partial timer updates and MQTT compatibility.
- [ ] Verify all PlatformIO environments and existing API/MQTT contract tests.
