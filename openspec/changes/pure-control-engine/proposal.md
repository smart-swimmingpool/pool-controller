# Change: Pure control engine

## Why

Current Rule implementations directly own and switch `RelayModuleNode` instances. This couples pool-control decisions to GPIO-facing infrastructure, makes domain tests depend on relay mocks and prevents the application layer from applying one centralized safety/output policy.

## What changes

- represent rule inputs as value objects instead of mutable node references
- make rule evaluation return a `ControlDecision`
- apply relay outputs only in the Core-1 application/runtime layer
- preserve current Auto, Manual, Boost and Timer behavior during migration
- centralize final safety clamps before actuator writes

## Impact

The refactor should not change external behavior. The main benefit is deterministic native testing of control logic and a clear boundary between decision making and hardware side effects.
