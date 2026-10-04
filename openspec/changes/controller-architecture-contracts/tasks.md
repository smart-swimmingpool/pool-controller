# Tasks

- [x] Add typed `OperationMode` with stable external conversion helpers.
- [x] Keep operation-mode conversion compatible with the firmware C++ toolchain.
- [x] Add fixed-size `ControllerCommand` and source/type enums.
- [x] Cover the existing temperature-circulation setters in the command contract.
- [x] Preserve independent timer-start and timer-end update semantics in the command contract.
- [x] Cover mutable runtime controller settings (loop interval, timezone, time-loss and NORVI button thresholds).
- [x] Add `SensorSnapshot` and `SystemSnapshot` read models.
- [x] Project sensor mapping identity and configured/found state through `SensorSnapshot`.
- [x] Project timer schedule, effective runtime and circulation-extension state through `SystemSnapshot`.
- [x] Project temperature-circulation configuration through `SystemSnapshot`.
- [x] Project runtime controller settings through `SystemSnapshot`.
- [x] Preserve GREEN/YELLOW/RED time degradation in the health read model.
- [x] Assert command/snapshot value types remain trivially copyable.
- [x] Add native tests for compatibility and copy semantics.
- [x] Register native tests in CMake/test runner.
- [ ] Adopt typed commands in MQTT/Web/local UI in a follow-up change.
- [ ] Adopt `SystemSnapshot` in MQTT/Web/display in a follow-up change.
- [ ] Replace direct rule-to-relay writes with application decisions in a follow-up change.
