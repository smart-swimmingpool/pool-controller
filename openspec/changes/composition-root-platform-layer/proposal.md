# Change: Explicit composition root and board platform layer

## Why

`PoolControllerContext` currently claims ownership of the controller subsystems, but most nodes are namespace-global objects and most managers are static singletons. Board-specific `#ifdef` paths also leak into application construction. This weakens lifecycle ownership, testability and board portability.

## What changes

- make `PoolControllerContext` the real composition root for application-owned components
- replace global `Nodes.hpp` access with explicit references/dependencies
- convert static application services to owned instances where lifecycle/test substitution benefits justify it
- keep low-level truly stateless helpers static where appropriate
- isolate NORVI, Olimex and generic ESP32 construction/pins/peripherals behind board-platform composition
- defer physical directory moves until dependencies are actually separated

## Dependencies

This should follow the adapter migration and pure-control work so the composition root does not merely repackage the current tightly coupled object graph.

## Impact

No external protocol changes are intended. The refactor changes construction, ownership and dependency direction and therefore requires full firmware-build and hardware smoke testing.
