# Change: Migrate adapters to typed commands and snapshots

## Why

MQTT, Web and local UI still depend directly on global nodes and static managers. That keeps protocol code coupled to mutable application state and makes the single-writer ownership model from #170 difficult to enforce structurally.

## What changes

- translate inbound MQTT/Web/local-UI runtime-controller mutations into `ControllerCommand` values
- process runtime commands only on the Core-1 application owner
- preserve independent timer-start/timer-end update behavior
- publish a complete `SystemSnapshot` after relevant state changes
- make MQTT/Web/display runtime output consume snapshots rather than `Nodes.hpp` or mutable controller settings
- retain existing external MQTT topics, Web payloads and behavior
- keep WiFi/MQTT credentials, admin-password flows and specialized calibration workflows in their dedicated services rather than transporting secrets through the general controller queue
- remove `MqttCommandQueue` only after the typed command path has equivalent coverage and bounded behavior

## Dependencies

- #214 controller architecture contracts
- #216 typed command queue foundation
- #217 snapshot-store foundation
- #170 multicore ownership boundary for sensor I/O

## Impact

The external interfaces remain compatible. The change is internal ownership/coupling cleanup and should reduce the number of translation units that can access mutable nodes and runtime controller settings directly.
