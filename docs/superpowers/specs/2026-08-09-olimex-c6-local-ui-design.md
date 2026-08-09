# Design: Olimex ESP32-C6-EVB Local Settings Variant

**Date:** 2026-08-09  
**Status:** Draft for user review  
**Scope:** Add a hardware concept for an Olimex ESP32-C6-EVB based pool-controller variant with a 2.8" ST7789/ILI9341-class SPI TFT and KY-040 rotary encoder for local settings navigation.

## Motivation

The project currently supports a generic ESP32 development board and a NORVI
AE01-R variant. The NORVI path already contains the project's local-control
pattern: onboard IO, display, buttons, and settings persisted through
`ConfigManager`/NVS.

The Olimex ESP32-C6-EVB is a plausible second integrated-controller variant
because it provides relays, opto-isolated inputs, wide-range DC input, and open
hardware documentation. The requested local settings interface should avoid
using scarce ADC pins and should remain robust in a pool equipment environment.

## Hardware Decision

Chosen combination:

```text
Board:      Olimex ESP32-C6-EVB
Display:    2.8" SPI TFT recommended, 2.4" minimum, 320x240
Input:      KY-040 rotary encoder with push button
Settings:   existing ConfigManager / Preferences / NVS
```

Rejected from the initial idea:

- **Analog joystick module**: technically possible, but it consumes two ADC pins,
  needs center/dead-zone calibration, can drift, and is less natural for menu
  navigation.

## Why This Combination Is Sensible

### Olimex ESP32-C6-EVB

The board matches the pool-controller use case well:

- 4 onboard relays for switched loads.
- 4 opto-isolated digital inputs.
- 8-50 V DC board supply option.
- ESP32-C6 with Wi-Fi 6, BLE, and 802.15.4 capability.
- Open-source hardware with documented connectors.

Main risks:

- ESP32-C6 is a single-core RISC-V target, unlike the original ESP32 target.
- PlatformIO/Arduino support must be verified by a real build environment.
- Several GPIOs are already consumed by relays, opto inputs, UEXT, LED, and
  button functions.

### 2.4"/2.8" SPI TFT

The initial 2" display idea is technically usable, but it is not ideal for the
desired combined Pool + Solar overview. To avoid repeating the cramped NORVI
OLED experience, the preferred display size is 2.8". A 2.4" display is the
minimum acceptable size if enclosure space is tight.

The display is appropriate for status pages and a simple settings menu when it
has at least 2.4" diagonal size:

- 320x240 is enough for readable values and menus at 2.4"/2.8".
- SPI display libraries are widely available.
- A full framebuffer should be avoided; use partial drawing or library-managed
  small buffers.

The UI should stay simple: one combined overview screen, short lists, edit
fields, and confirmation dialogs. A heavy LVGL-style animated UI is out of
scope for this variant.

### KY-040 Rotary Encoder

The encoder is preferred over the joystick because it:

- uses digital GPIOs instead of ADC pins,
- has no analog center drift,
- maps naturally to menu navigation,
- allows operation with one knob: rotate, press, long-press,
- preserves ADC pins for possible future sensors.

## User Interaction Model

```text
Rotate clockwise         -> next menu item / increase value
Rotate counter-clockwise -> previous menu item / decrease value
Short press              -> select / confirm
Long press               -> back / leave menu
Idle timeout             -> return to status screen
```

The local UI should expose only safe, essential settings. Full configuration
remains available through the web UI.

Recommended local menu scope:

1. Status overview: water temperature, pump state, mode, Wi-Fi/MQTT state.
2. Operation mode: auto/manual/off as already supported by the controller.
3. Pump/manual actions where safe.
4. Network/MQTT status read-only.
5. Sensor assignment/status where this can reuse existing `ConfigManager` data.

Potentially dangerous operations should use explicit confirmation screens.

## Main Screen Layout

The main screen should show Pool and Solar at the same time. This is the key
reason to prefer a 2.8" display over the originally considered 2" display.

Example structure:

```text
┌────────────────────────────┐
│ POOL                 AUTO  │
│ 27.4 °C                    │
│ Pumpe: AN                  │
├────────────────────────────┤
│ SOLAR                  OK  │
│ 35.1 °C                    │
│ Ventil: SOLAR              │
└────────────────────────────┘
```

Layout rules:

- Pool and Solar each get a clear half-screen block.
- Temperatures are the largest text on the screen.
- Mode/status labels stay short: `AUTO`, `MAN`, `AUS`, `OK`, `ERR`.
- No dense tables and no more than three secondary lines per block.
- Detailed values and settings move to subpages selected with the encoder.

## Local Control Flow

The rotary encoder should behave consistently across the whole local UI:

```text
Status screen:
  rotate        -> switch between overview/detail pages
  short press   -> open main menu
  long press    -> refresh/return to overview

Menu list:
  rotate        -> move selection up/down
  short press   -> enter selected item
  long press    -> go back

Value editing:
  rotate        -> increase/decrease value or change option
  short press   -> save/confirm
  long press    -> cancel without saving

Confirmation screen:
  rotate        -> choose No/Yes
  short press   -> confirm current choice
  long press    -> cancel
```

This gives users a predictable mental model: rotate changes focus or value,
short press accepts, and long press exits or cancels.

## Architecture

The new variant should not add more board-specific branching directly into
`PoolController.cpp`. Instead, introduce a small local-UI boundary and adapt the
existing NORVI path to it over time.

Proposed structure:

```text
LocalUi
  begin(...)
  loop()
  showStatus(...)
  notifyEvent(...)

Implementations:
  NorviLocalUi          -> existing OLED + button flow
  OlimexTftEncoderUi   -> ST7789 TFT + KY-040 encoder
```

Supporting modules:

```text
OlimexTftDisplay       -> drawing primitives, pages, menu rendering
OlimexEncoderHandler   -> encoder decode, debounce, short/long press events
LocalSettingsMenu      -> shared menu state machine using ConfigManager
```

`ConfigManager` remains the source of truth for persisted settings. The local UI
is only another frontend for reading and changing selected settings.

## Build and Board Variant

Add a dedicated PlatformIO environment rather than overloading `esp32dev`:

```ini
[env:olimex_esp32_c6_evb]
platform = espressif32
board = esp32-c6-devkitc-1 ; or a custom Olimex board definition if needed
framework = arduino
build_flags =
  ${common.build_flags}
  -D OLIMEX_ESP32_C6_EVB
  -D HAS_LOCAL_TFT_UI
```

The exact `board` value is an implementation-time decision. If PlatformIO has no
matching Olimex board definition, use the closest ESP32-C6 board or add a custom
board JSON after verifying flash, upload, and serial settings.

## Initial Pin-Planning Direction

Use the EVB's onboard functions for the controller IO:

```text
Relays:       Olimex onboard relay GPIOs
Opto inputs:  Olimex onboard opto input GPIOs
Display SPI:  UEXT SPI pins where possible
Encoder:      free digital GPIOs, preferably not ADC-only constrained pins
Backlight:    fixed 3.3 V or one PWM-capable GPIO if dimming is required
```

Pin assignment must be validated against the Olimex schematic before
implementation. The concept intentionally avoids assigning final GPIO numbers in
the design, because the implementation should verify connector exposure,
boot-strapping constraints, and PlatformIO board support first.

## Error Handling and Reliability

- Encoder input must be debounced and tolerant of missed transitions.
- Long-press detection should be time-based and non-blocking.
- Display failures must not stop pool control logic.
- Local UI drawing must never block pump/sensor/control loops for long periods.
- Unsafe actions require confirmation.
- If local UI initialization fails, firmware should continue with web/MQTT
  control where possible.

## Testing and Verification

Minimum meaningful verification for implementation:

1. Build the existing `esp32dev`/NORVI environments to prove no regression.
2. Build the new `olimex_esp32_c6_evb` environment.
3. Unit-test or host-test the menu state machine where feasible.
4. Hardware smoke test:
   - TFT initializes and renders a status page.
   - Main page shows Pool and Solar together without cramped text.
   - Encoder rotation creates up/down events.
   - Short press selects.
   - Long press backs out.
   - Existing controller loop continues while navigating menus.
5. Validate relays and opto inputs against the Olimex schematic before connecting
   real pool equipment.

## Alternatives Considered

### A. Olimex + 2.8" TFT + KY-040 Encoder — chosen

Best balance of robustness, pin usage, and menu usability.

### A2. Olimex + 2.4" TFT + KY-040 Encoder — acceptable minimum

Works if enclosure space is limited, but gives less visual breathing room for
the combined Pool + Solar overview.

### B. Olimex + 2" TFT + KY-040 Encoder

Technically works, but it is too close to the cramped NORVI experience when
Pool and Solar must be visible together on the main screen.

### C. Olimex + TFT + Joystick

Works electrically if powered from 3.3 V, but consumes ADC pins and creates more
software/UX complexity.

### D. Olimex + Web UI only

Simplest and lowest-risk implementation. Rejected for this concept because the
goal is explicit local settings control.

### E. Touch TFT

Most comfortable local UI, but higher complexity, more pins, more display work,
and more mechanical/front-panel considerations.

## Out of Scope

- Replacing the web UI.
- Building a complex graphical dashboard with animations.
- Zigbee/Thread feature use on ESP32-C6.
- Final production enclosure design.
- Final certified mains wiring design.
- Direct implementation before board support and pin mapping are verified.

## Open Implementation Questions

1. Which PlatformIO board definition works best for the Olimex ESP32-C6-EVB?
2. Which exact GPIOs are safe and exposed for TFT DC/RST/CS/backlight and encoder
   A/B/SW?
3. Which exact 2.8" SPI TFT module should be used, and is ST7789 or ILI9341 the
   best-supported controller for that module?
4. Should display backlight be fixed-on or PWM-dimmable?
5. Which settings are safe enough for local editing in the first release?
6. Should the NORVI OLED/button implementation be adapted to the same `LocalUi`
   interface immediately, or should the abstraction be introduced only for the
   new Olimex variant first?
