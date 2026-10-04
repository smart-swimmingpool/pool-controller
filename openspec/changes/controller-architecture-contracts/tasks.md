# Tasks

- [x] Add typed `OperationMode` with stable external conversion helpers.
- [x] Add fixed-size `ControllerCommand` and source/type enums.
- [x] Add `SensorSnapshot` and `SystemSnapshot` read models.
- [x] Assert command/snapshot value types remain trivially copyable.
- [x] Add native tests for compatibility and copy semantics.
- [x] Register native tests in CMake/test runner.
- [ ] Adopt typed commands in MQTT/Web/local UI in a follow-up change.
- [ ] Adopt `SystemSnapshot` in MQTT/Web/display in a follow-up change.
- [ ] Replace direct rule-to-relay writes with application decisions in a follow-up change.
