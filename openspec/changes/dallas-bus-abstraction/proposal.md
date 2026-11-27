# Change: Separate Dallas buses from logical sensor roles

## Why

`DallasTemperatureNode` currently combines physical OneWire/Dallas bus ownership, discovery, ROM selection, logical pool/solar roles, measurement, recovery and cross-core snapshots. Shared-bus NORVI and dedicated-bus ESP32 variants therefore require branching inside the node itself.

## What changes

- model physical Dallas buses separately from logical `POOL` / `SOLAR` roles
- keep all OneWire/Dallas operations owned by the Core-0 sensor task after startup
- move discovery/rescan and ROM enumeration to the bus abstraction
- keep role-to-ROM mapping as application/configuration data
- publish a complete sensor acquisition generation to Core 1
- support one shared bus and two dedicated buses through composition instead of role-node conditionals

## Dependencies

- #170 for the multicore sensor ownership rule
- #214 for snapshot/value contracts, or an equivalent merged foundation

## Impact

This is a hardware-sensitive refactor and requires on-device verification for both shared-bus NORVI and dedicated-bus ESP32 topologies before merge.
