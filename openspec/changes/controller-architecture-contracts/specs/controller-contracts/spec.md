## ADDED Requirements

### Requirement: Bounded NTP runtime configuration

The controller contract SHALL carry an owned NTP server value in both `SET_NTP_SERVER` and `ControllerSettingsSnapshot`. The value SHALL hold 1..127 non-NUL bytes plus a terminator without dynamic allocation. Invalid input SHALL be rejected without truncation or mutation of the previous value. Core 1 SHALL own application of accepted updates; the time service SHALL own NTP client lifecycle.

#### Scenario: Callback buffer lifetime ends

- **WHEN** a valid NTP server is copied into a command and the callback buffer changes
- **THEN** the command and subsequent snapshot SHALL retain independent copies of the complete server value

#### Scenario: Malformed or oversized text

- **WHEN** input is null, empty, at least 128 bytes long, or contains an embedded NUL
- **THEN** assignment SHALL fail and preserve the destination
- **AND** queued buffers without a terminator SHALL fail validation

### Requirement: Relative actions survive delayed snapshot publication

The contract SHALL represent pool-pump toggle, solar-pump toggle and mode-cycle actions separately from absolute setters. The owner SHALL evaluate each accepted relative action against its current state in FIFO order. Adapters SHALL NOT derive an absolute target from a cached snapshot or coalesce repeated actions.

#### Scenario: Two queued toggles

- **WHEN** two toggles of the same pump are accepted without an intervening snapshot publication
- **THEN** both SHALL be evaluated and restore the starting relay state if mode policy and safety permit

#### Scenario: Distinct local and Web mode policies

- **WHEN** a toggle is applied
- **THEN** Web's `REQUIRE_MANUAL` SHALL reject outside manual mode
- **AND** NORVI's `KEEP_MODE` SHALL preserve the current mode
- **AND** Olimex's `ENTER_MANUAL` SHALL enter manual mode and toggle as one owner action
- **AND** all policies SHALL remain subject to the application safety policy

#### Scenario: Four queued cycles

- **WHEN** four mode-cycle actions are accepted without an intervening snapshot publication
- **THEN** the owner SHALL apply `auto -> manu -> boost -> timer -> auto` and return to the initial mode

### Requirement: Coherent detected-device inventory

`SensorSnapshot` SHALL represent up to 20 detected devices across all physical buses, including unassigned devices. Each entry SHALL contain the ROM address and a temperature with independent validity. Inventory, logical mappings, measurements, generation and timestamp SHALL be published as one coherent value by the acquisition owner.

#### Scenario: Full inventory with an unreadable device

- **WHEN** 20 unique ROMs are detected and one temperature is invalid
- **THEN** the snapshot SHALL preserve all 20 ROMs and expose the invalid reading separately from presence
- **AND** adapters SHALL be able to offer the unreadable or unassigned ROM for selection without live Dallas access

#### Scenario: Shared or dedicated buses exceed capacity

- **WHEN** enumeration produces duplicate ROMs or more than 20 unique devices
- **THEN** the producer SHALL deduplicate across buses and retain at most 20 entries in deterministic scan order
- **AND** the count SHALL stay within array capacity

#### Scenario: Rescan finds no devices

- **WHEN** the next complete acquisition generation has no detected devices
- **THEN** its zero-count inventory SHALL replace the old inventory rather than retaining stale entries
