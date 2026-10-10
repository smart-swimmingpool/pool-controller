# Design: Dallas bus abstraction

## Target model

```text
SensorTask (Core 0)
  |
  +-- DallasBus A ---- discovered ROMs
  |
  +-- DallasBus B ---- discovered ROMs      (dedicated-bus boards)
  |
  +-- SensorRoleMapping { POOL -> ROM, SOLAR -> ROM }
  |
  +-- SensorSnapshot generation
                         |
                         v
                      Core 1
```

On NORVI, one `DallasBus` instance serves both roles. On dedicated-bus boards, two instances are composed. The logical role model does not know which topology is active.

## Responsibilities

### DallasBus

- own `OneWire` and `DallasTemperature`
- begin, rescan and enumerate ROM addresses
- start non-blocking conversions
- read validated values by ROM
- expose discovery results as value copies

### Sensor role mapping

- map `SensorRole` to a ROM address
- persist/load mappings outside the bus driver
- request mapping changes through the sensor-task command boundary

### Sensor acquisition

- trigger all required bus conversions
- wait/yield once per conversion window
- read pool/solar/controller values
- publish one generation with timestamp and validity flags

## Invariants

- no Core-1 OneWire/Dallas call after sensor task startup
- no role node owns a physical bus
- no adapter reads discovery state from live Dallas objects
- reconnect/rescan behavior preserves the current recovery cadence and reading filter semantics

## Verification

- native tests for topology planning, role mapping and snapshot generation
- regression tests for missing/replaced sensors
- PlatformIO builds for NORVI, esp32dev and Olimex C6
- on-device tests for one shared bus and two dedicated buses
