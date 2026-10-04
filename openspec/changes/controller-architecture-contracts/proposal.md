# Change: Controller architecture contracts

## Why

The controller currently exposes mutable global nodes and static managers directly to MQTT, Web and local UI code. PR #170 improves the multicore ownership boundary, but the application still lacks explicit contracts for commands and read-only state. Without those contracts, future refactoring would continue to couple adapters to concrete nodes and would make cross-task access difficult to reason
about.

## What changes

- introduce a typed `OperationMode` while preserving the existing external wire values
- introduce a fixed-size, trivially-copyable `ControllerCommand` contract for inbound adapter requests
- introduce `SensorSnapshot` and `SystemSnapshot` read models for outbound adapters
- keep the new types independent of Arduino, MQTT, Web, display and hardware drivers
- add native tests for parsing, compatibility and value semantics

This change deliberately does not switch existing production call sites yet. It establishes stable contracts that following PRs can adopt incrementally.

## Impact

- no runtime behavior change
- no NVS or MQTT compatibility change
- no additional dynamic allocation
- enables subsequent command-queue and snapshot migrations without a flag day
