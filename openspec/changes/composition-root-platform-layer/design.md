# Design: Composition root and platform layer

## Target structure

```text
main.cpp
  |
  v
BoardPlatform / board composition
  |
  v
PoolControllerContext
  +-- application runtime
  +-- command/snapshot boundaries
  +-- adapters
  +-- application services
  +-- driver references/instances
```

`main.cpp` remains a minimal Arduino entry point. Board-specific construction selects the concrete drivers and capabilities. `PoolControllerContext` owns the application lifecycle and receives explicit dependencies rather than discovering them through namespace globals or static service locators.

## Ownership rules

- No mutable namespace-global controller nodes.
- No `Nodes.hpp` service-locator pattern in application/adapters.
- Prefer constructor/reference injection and static object lifetime over heap-allocated dependency graphs.
- Static APIs remain only for constexpr data or genuinely stateless helpers where ownership is irrelevant.
- Hardware drivers contain hardware behavior only and never application rules.
- Adapters depend on application contracts, not concrete sensor/relay nodes.

## Board isolation

The platform layer is responsible for board-dependent composition:

- GPIO assignments
- relay polarity and available relay channels
- Dallas topology: shared bus or dedicated buses
- OLED/TFT availability and concrete display driver
- buttons/encoder/input devices
- board-specific peripheral initialization

Application/domain code must not branch on `NORVI_AE01_R`, `OLIMEX_ESP32_C6_EVB` or similar board macros. Such compile-time selection belongs at the platform/composition boundary.

## Service migration

Static managers are not converted mechanically. Each service is assessed by responsibility:

- services with mutable lifecycle/application state become owned instances where this materially improves ownership and testability;
- low-level stateless helpers stay functions/static utilities;
- hardware/network libraries that internally require singleton callbacks may use a narrow adapter trampoline while the application-facing object remains explicit.

This avoids replacing one global architecture with an unnecessarily large virtual-interface hierarchy.

## Migration order

1. Complete adapter command/snapshot migration so adapters no longer require global nodes.
2. Complete pure control-engine migration so domain logic no longer owns relay drivers.
3. Complete Dallas bus/role separation so sensor topology is composable.
4. Define board platform/profile contracts and concrete board compositions.
5. Move node/driver/application construction into the composition root.
6. Remove remaining mutable namespace globals and `Nodes.hpp` dependencies.
7. Convert selected static managers to explicitly owned services where useful.
8. Move files into `domain/`, `application/`, `adapters/`, `drivers/` and `platform/` only after dependency directions are clean.

## Verification

- native unit tests remain independent of concrete boards
- PlatformIO builds succeed for `esp32dev`, `norvi_ae01_r` and `olimex_esp32_c6_evb`
- no application/domain source contains board-selection preprocessor branches after migration
- code search confirms adapters no longer reach mutable controller state through `Nodes.hpp`
- hardware smoke tests verify relays, sensors, local UI, MQTT, Web and OTA on representative boards
