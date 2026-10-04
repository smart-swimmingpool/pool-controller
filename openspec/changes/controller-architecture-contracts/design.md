# Design: Controller architecture contracts

## Context

The firmware has three relevant ownership domains:

1. sensor acquisition on Core 0 after PR #170
2. mutable application/control state on the Arduino loop task on Core 1
3. external adapters such as MQTT, Web and local UI

Today adapters still access global nodes and static managers directly. The target architecture makes the boundaries explicit: adapters submit commands and consume immutable snapshots; only the application runtime mutates domain state.

## Goals

- keep mutable application state single-writer on Core 1
- keep cross-task data fixed-size and bounded
- remove protocol strings from future domain/application logic
- preserve existing MQTT/Web/NVS operation-mode values
- cover all currently supported mutable controller settings before adapter migration
- project all currently published timer and temperature-circulation state before adapter migration
- avoid heap allocation in the new boundary types
- permit native tests without Arduino headers
- remain compatible with the firmware C++ toolchain, not only the C++17 native-test toolchain

## Non-goals

- replacing all global nodes in this PR
- changing rule behavior
- changing MQTT or Web payloads
- moving files into final layer directories
- introducing a generic event bus

## Decisions

### Typed operation mode

`OperationMode` is an enum used inside future domain/application code. Conversion to and from `auto`, `manu`, `boost` and `timer` happens at persistence/protocol boundaries. The legacy `manu` value is intentionally preserved for compatibility.

Conversion helpers use ordinary `inline` functions rather than C++14-style `constexpr` function bodies so the contract also builds with the current ESP32 firmware compiler mode.

### Fixed-size commands

`ControllerCommand` is a flat, trivially-copyable value type. It may contain fields unused by a particular command. This wastes a small, bounded amount of memory but avoids unions with lifecycle concerns, heap allocation and protocol-specific payload ownership.

The command type set covers the existing controller mutations that the adapter migration must preserve, including pool/solar thresholds, hysteresis and the three temperature-circulation settings (`threshold`, `factor`, `max runtime`). Numeric values share the fixed `value` field and are validated by the Core-1 application handler before being applied.

The eventual command queue is bounded. Adapters must never hold references into mutable controller objects.

### Immutable read models

`SensorSnapshot` represents one coherent acquisition generation and includes a generation counter and timestamp. `SystemSnapshot` is a projection for outbound adapters. It contains values, not references or pointers to mutable nodes.

The read model includes the current timer schedule, effective runtime, circulation extension and active extended end time. It also includes the temperature-circulation configuration. This allows MQTT and Web to preserve their existing output without retaining direct reads from `OperationModeNode`, `Rule` or `ConfigManager` after the adapter migration.

Following PRs will build these snapshots on the owning task and hand copies to MQTT, Web and display code.

### No generic interfaces yet

This change introduces contracts, not a broad virtual-interface hierarchy. Hardware interfaces are extracted later only where replacement/testing requires them.

## Migration sequence

1. introduce these contracts
2. move all inbound MQTT/Web/local-UI mutations to a typed command queue
3. publish `SystemSnapshot` from the Core-1 application runtime
4. migrate MQTT/Web/display reads away from `Nodes.hpp`
5. make the control engine return decisions instead of writing relays directly
6. replace global node/service ownership with an explicit composition root
7. isolate board-specific construction in the platform layer

## Concurrency invariants

- OneWire/Dallas state is owned by the sensor task after startup.
- Mutable controller/domain state is owned by the Core-1 loop task.
- Cross-core communication uses bounded snapshots/commands only.
- No adapter may directly mutate shared nodes from callback tasks.
