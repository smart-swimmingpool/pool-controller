# Design: Pure control engine

## Target model

```text
ControlInputs + ControllerConfig + ControlState
                    |
                    v
              ControlEngine
                    |
                    v
              ControlDecision
                    |
                    v
     Application safety/output policy
                    |
                    v
               Relay drivers
```

The domain layer must not include or store `RelayModuleNode*`, MQTT, Web, NVS or GPIO dependencies.

## Decision values

The output contains the desired pool-pump and solar-pump states plus a compact reason code suitable for logging/diagnostics. Stateful timer-extension data belongs to explicit `ControlState`, not hidden hardware objects.

## Migration

1. Extract timer/window calculations into pure functions/value objects.
2. Introduce `ControlInputs`, `ControlState` and `ControlDecision`.
3. Port Auto rule and verify behavior equivalence.
4. Port Manual, Boost and Timer rules.
5. Add an application actuator stage that applies safety clamps and relay states.
6. Remove relay pointers from Rules.
7. Collapse or replace `OperationModeNode` rule dispatch once all modes use the new engine.

## Safety invariant

The final actuator stage is the only place where a control decision becomes hardware output. Safe mode and critical degradation can override a decision there before GPIO state changes.
