# Design: Controller architecture contracts

## Context

The firmware has three relevant ownership domains:

1. sensor acquisition on Core 0 after PR #170
2. mutable application/control state on the Arduino loop task on Core 1
3. external adapters such as MQTT, Web and local UI

Today adapters still access global nodes and static managers directly. The target architecture makes the boundaries explicit: adapters submit
commands and consume immutable snapshots; only the application runtime mutates domain state.

## Goals

- keep mutable application state single-writer on Core 1
- keep cross-task data fixed-size and bounded
- remove protocol strings from future domain/application logic
- preserve existing MQTT/Web/NVS operation-mode values
- cover all currently supported runtime controller mutations before adapter migration
- preserve independent timer-start and timer-end update semantics
- project all currently published timer, sensor-mapping, time-degradation, network/heap diagnostics and runtime controller-setting state before adapter migration
- avoid heap allocation in the new boundary types
- permit native tests without Arduino headers
- remain compatible with the firmware C++ toolchain, not only the C++17 native-test toolchain

## Non-goals

- replacing all global nodes in this PR
- changing rule behavior
- changing MQTT or Web payloads
- moving files into final layer directories
- introducing a generic event bus
- moving WiFi/MQTT credentials or authentication secrets through the general controller command queue

Provisioning/authentication secrets remain owned by dedicated configuration/auth services. The command queue covers runtime controller/domain
settings that must obey the Core-1 single-writer rule.

## Decisions

### Typed operation mode

`OperationMode` is an enum used inside future domain/application code. Conversion to and from `auto`, `manu`, `boost` and `timer` happens at
persistence/protocol boundaries. The legacy `manu` value is intentionally preserved for compatibility.

Conversion helpers use ordinary `inline` functions rather than C++14-style `constexpr` function bodies so the contract also builds with the
current ESP32 firmware compiler mode.

### Fixed-size commands

`ControllerCommand` is a flat, trivially-copyable value type. It may contain fields unused by a particular command. This wastes a small, bounded
amount of memory but avoids unions with lifecycle concerns, heap allocation and protocol-specific payload ownership.

Decimal controller settings use `value`; integer-only settings use `integerValue`. The application handler validates range and semantic
constraints before applying either field.

The command set covers the existing runtime mutations required by MQTT/Web/local UI: mode, temperature setpoints, hysteresis,
temperature-circulation settings, loop interval, timezone, NTP server, time-loss thresholds, NORVI button thresholds, sensor mapping, absolute
manual relay commands and relative pump/mode actions.

Timer start and timer end are separate commands. This preserves the current APIs where MQTT updates start/end independently and Web may submit
only some timer fields, without requiring an adapter to read stale state and synthesize a complete timer value.

`SET_NTP_SERVER` owns an `NtpServerValue` (128 bytes including the NUL terminator), shared with `ControllerSettingsSnapshot::ntpServer`.
`assign()` accepts 1..127 bytes, rejects null/empty/oversized/embedded-NUL inputs without modifying the previous value, and never truncates. The
handler must check `valid()` before using a queued value as a C string. `valid()` accepts only the canonical layout `assign()` produces: a nonempty
prefix of non-NUL bytes, the first NUL terminator, and only zero bytes after it; raw queued values with a terminator followed by nonzero bytes are
rejected. NTP is a runtime setting, not a secret: Core 1 validates/persists it and
asks the time service to reconfigure. The time service owns client lifecycle and networking; adapters do not mutate `ConfigManager::getNtp()`.

The bound matches the existing MQTT setter (`0 < len < 128`). The legacy Web setter and persisted strings are currently unbounded. Before
migrating them in #218, validate new Web input explicitly and report an error for out-of-contract values; do not silently truncate or replace an
oversized legacy NVS value. Such a legacy value must remain intact behind an immutable time-configuration service projection until explicitly
corrected. This PR changes no existing call sites or stored values.

`TOGGLE_POOL_PUMP`, `TOGGLE_SOLAR_PUMP` and `CYCLE_MODE` encode relative intent. The handler evaluates every accepted action in FIFO order
against its current state, ignoring the absolute `enabled`/`mode` payload fields. No adapter may synthesize these actions from a possibly stale
snapshot, and a queue must not coalesce repeated actions. The cycle remains `auto -> manu -> boost -> timer -> auto`.

`PumpToggleModePolicy` preserves the existing use cases explicitly: Web uses `REQUIRE_MANUAL`, NORVI uses `KEEP_MODE`, Olimex uses
`ENTER_MANUAL`. The latter enters manual mode and toggles the then-current relay in one handler operation, avoiding a partially accepted
two-command submission. The owner rechecks policy and safety at execution; the policy cannot bypass safety overrides. Default commands require
manual mode. Absolute MQTT ON/OFF commands remain unchanged.

The eventual command queue is bounded. Adapters must never hold references into mutable controller objects.

### Immutable read models

`SensorSnapshot` represents one coherent acquisition generation and includes a generation counter and timestamp. It also carries pool/solar
mapping identity: configured address, configured state and found state. This lets MQTT preserve sensor-select and sensor-found entities without
touching Dallas nodes.

It also carries `detected`, a fixed array of 20 `DetectedSensorSnapshot` values, and `detectedCount` in 0..20. Each entry owns its 8-byte ROM and
a temperature with an independent validity flag. The list includes unassigned and unreadable devices. Only `[0, detectedCount)` is populated. The
acquisition owner enumerates both physical buses, deduplicates ROMs (including a shared bus), keeps at most 20 in deterministic scan order, and
publishes inventory, role mappings, readings, generation and timestamp together. Invalid readings retain their ROM; Web omits the temperature as
today, and MQTT can still offer that address. On a rescan, a fresh value replaces the entire old inventory, including a zero-device result.
Adapters never initiate Dallas reads.

The shared cap is 20 devices total, not 20 per role or per bus. Overflow does not increase the count or overwrite storage. Producer regression
tests for deduplication, the 21st device, both bus topologies and rescan replacement are required in #218/#220; this contract PR tests the
empty/full value representation and independent copies.

`SystemSnapshot` is a projection for outbound adapters. It contains values, not references or pointers to mutable nodes. The read model includes:

- timer schedule, effective runtime, circulation extension and active extended end time
- temperature-circulation configuration
- runtime controller settings, including bounded NTP server text, used by Web/config/status projections
- three-state time degradation (`GREEN`, `YELLOW`, `RED`) in addition to the coarse time-valid flag
- sensor role mappings and configured/found state, plus the detected-device inventory
- local IPv4 as four octets plus explicit validity, independent of `String`, `WiFi` and `NetworkManager`
- total free heap and maximum allocatable heap so fragmentation diagnostics do not require direct `ESP` access

The snapshot producer selects the same effective address as the existing status path: the SoftAP address in AP mode, otherwise the station local
address. It marks the value invalid when no meaningful address is available. Formatting the four octets into JSON or display text is an adapter
responsibility and happens only after the immutable copy crosses the ownership boundary.

Following PRs will build these snapshots on the owning task and hand copies to MQTT, Web and display code.

### No generic interfaces yet

This change introduces contracts, not a broad virtual-interface hierarchy. Hardware interfaces are extracted later only where replacement/testing
requires them.

## Migration sequence

The minimal dependency graph is `#214 -> {#216, #217} -> #218 -> #219`, with `#220` depending on `#214` and the sensor ownership boundary in
`#170`; `#221` waits for `#218`, `#219` and `#220`. Queue and snapshot store can be reviewed independently. There is no intrinsic dependency
between the pure control engine and the Dallas bus refactor.

| Stage | PR | Smallest merge gate / remaining work |
| --- | --- | --- |
| Contract | #214 | These value types, native boundary tests and OpenSpec; no production migration. |
| Transport | #216 and #217 | Update to the completed contract; verify long NTP value copies, repeated relative FIFO actions, full 20-device snapshot replacement/concurrent reads and the 596-byte diagnostics-complete snapshot. Check larger copy/critical-section budgets. Neither PR needs the other. |
| Adapter adoption | #218 | Already has a handler and typed MQTT runtime parsing at `059681e`; retain this work. Add NTP dispatch/application/projection, relative actions and policies, Web/local UI producers, complete snapshot publication including local IP/max-alloc heap, and all read-side migrations. Its current checked task claiming a complete handler must be revisited for the added variants. |
| Pure control | #219 | After #218, extract decisions and central safety/actuator policy; verify all four modes, timers and relative manual actions without changing behavior. Current PR is a specification draft. |
| Sensor ownership | #220 | After #214 and #170, replace the temporary sensor projection bridge with DallasBus/role separation. Can proceed independently of #219. Verify shared/dedicated buses on real hardware. Current PR is a specification draft. |
| Composition | #221 | After #218, #219 and #220, remove global ownership and isolate board construction; avoid moving coupled globals into a new directory. Current PR is a specification draft. |

`#170` is still an open draft at `3f00319` (inspection on 2026-10-05); sensor-task ownership is not yet present on main. It is not a prerequisite
for merging these unused contracts or #216/#217. Before #218 enables cross-task sensor reads/mapping writes, integrate and verify #170's
ownership boundary. Core 1 must request mapping/rescan work through that boundary, never call OneWire/Dallas itself after sensor startup. On
single-core boards, the same ownership rule applies even when tasks share a core.

To avoid making the whole #220 refactor a prerequisite for #218, first project the existing sensor owner's cached discovery/measurement
generation into `SensorSnapshot`. This bridge must enumerate both buses and enforce the same inventory invariants; it cannot assemble a snapshot
by reading live Dallas objects from Core 1. #220 then replaces internals behind the stable contract. If no such bridge is provided, #220 becomes
a hard predecessor of #218's sensor-read migration.

A conservative serial merge order is therefore **#214, #216, #217, #218, #219, #220, #221**, with #170 integrated before enabling the
sensor-dependent part of #218. #216/#217 may swap; #220 may move earlier once its ownership and hardware gates are satisfied. Reconcile stacked
branches after the foundation merges; #218 already contains queue/store code and must not reintroduce duplicate implementations.

## Verification and resource impact

- Native contract tests cover NTP lengths 0/1/127/128, null and embedded-NUL input, invalid raw buffers including noncanonical queued values,
  unchanged output on rejection,
  command/snapshot copy ownership, relative action identity and mode policy, empty/full inventory, invalid readings, local IPv4 copy semantics
  and both heap diagnostics.
- #218 must add real handler/queue regressions: two accepted toggles restore the original state when safety permits; two cycles advance twice and
  four cycles wrap even without a snapshot refresh; Web rejects outside manual mode; Olimex mode-entry plus toggle is atomic; NORVI keeps its
  mode. An overflowed action must be explicitly rejected, not silently lost.
- Native C++11 compilation checks the headers independently of Arduino; normal native tests run with AddressSanitizer. All three firmware builds,
  MegaLinter and CodeQL remain CI gates.
- On the native GCC ABI, `ControllerCommand` is 156 bytes, `SensorSnapshot` 376 bytes and `SystemSnapshot` 596 bytes. The 16-entry queue in #216
  stores 2496 payload bytes (2048 more than before); each system snapshot copy/store grows by 464 bytes over the pre-contract read model. There
  is no heap allocation in these values. Firmware ABI sizes, critical-section duration and task stack margins must be checked when transports are
  adopted.
- This PR introduces no active instances/call-site changes, so production RAM/Flash/CPU behavior remains unchanged. Reverting its contract commit
  is sufficient before adoption; after adoption, dependent consumers must be reverted together.

## Concurrency invariants

- OneWire/Dallas state is owned by the sensor task after startup.
- Mutable controller/domain state is owned by the Core-1 loop task.
- Cross-core communication uses bounded snapshots/commands only.
- No adapter may directly mutate shared nodes or runtime controller settings from callback tasks.
- No outbound adapter may query mutable network/runtime diagnostics directly once `SystemSnapshot` adoption begins.
- Provisioning/authentication secrets use dedicated services rather than the general controller command queue.
