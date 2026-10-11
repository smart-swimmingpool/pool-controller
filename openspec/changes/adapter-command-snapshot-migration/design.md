# Design: Adapter command and snapshot migration

## Ownership

The Core-1 application loop is the only owner allowed to mutate controller/domain state and runtime controller settings. Adapter callbacks may parse/validate protocol input, but they must not mutate nodes, runtime settings or relay state directly.

Provisioning/authentication data is deliberately separate: WiFi/MQTT credentials and admin-password flows remain owned by dedicated configuration/auth services and are not copied through the general controller command queue. Likewise, specialized calibration workflows remain behind their dedicated service API until the composition-root/configuration refactor gives them an explicit ownership boundary.

## Inbound path

```text
MQTT callback -> protocol validation -> ControllerCommandQueue
Web handler    -> protocol validation -> ControllerCommandQueue
Local UI       -> interaction mapping -> ControllerCommandQueue
                                      -> Core-1 command handler -> state mutation
```

The typed path covers runtime controller mutations: mode, setpoints, circulation settings, independent timer start/end updates, loop interval, timezone/time-loss thresholds, NORVI button thresholds, sensor mappings and manual relay commands.

For synchronous Web responses, the first migration may process the typed command immediately when the handler is already executing on the owning loop task. The command handler remains the single mutation API for that runtime state.

## Outbound path

```text
Core-1 state -> SystemSnapshot -> ControllerSnapshotStore
                               -> MQTT
                               -> Web status/config projections
                               -> OLED/TFT
```

Adapters receive value copies. They do not receive node pointers or mutable runtime-config references. The snapshot must preserve the existing external projection, including timer runtime state, sensor mapping identity/configured/found state and GREEN/YELLOW/RED time degradation.

## Migration order

1. Add a Core-1 command handler for the complete runtime `ControllerCommand` contract.
2. Route processed MQTT runtime commands through typed commands.
3. Route Web runtime mutations through the same handler.
4. Route local UI runtime mutations through the same handler.
5. Build and publish `SystemSnapshot` from the owning loop.
6. Migrate status/read paths one adapter at a time.
7. Remove adapter dependencies on `Nodes.hpp` and mutable runtime config where no longer needed.
8. Remove `MqttCommandQueue` after the typed queue owns callback hand-over.

## Verification

- native tests for command validation/dispatch and queue overflow
- regression tests for independent timer-start/timer-end updates
- existing MQTT/Web compatibility tests remain unchanged
- PlatformIO builds for all supported boards
- regression test proving MQTT callbacks cannot mutate operation mode or runtime settings directly
- grep/code-search gate showing migrated adapters no longer include `Nodes.hpp` for mutable state access
- verify provisioning/auth flows remain isolated from the general runtime command queue
