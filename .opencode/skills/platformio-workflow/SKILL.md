---
name: platformio-workflow
description: "Build, flash, monitor, and OTA workflow for the pool-controller ESP32 firmware. Use when asked to build, flash, debug serial output, perform OTA updates, or troubleshoot PlatformIO issues. 🇩🇪 Deutsche Trigger: Build, Flashen, Monitor, OTA-Update, Serielle Ausgabe, PlatformIO Problemen, Kompilieren, Upload, ESP32 flashen."
keywords:
  - platformio build
  - esp32 flashen
  - firmware upload
  - ota update
  - serielle ausgabe
  - serial monitor
  - kompilieren
  - build fehler
  - pio run
  - pio device monitor
  - dependency management
  - library dependency
  - olimex build
  - pioarduino
  - native usb cdc
  - olimex_esp32_c6_evb
---

# PlatformIO Workflow — Pool Controller

Build, flash, monitor, and OTA operations for the ESP32 pool-controller.

> **🔍 Code Search**: Use `semble search "pio run"` or
> `semble search "upload_protocol"` to find build-related patterns. See
> `Agents.md` §7 for full `semble` usage.

## Environment

- **Default target**: `esp32dev` (ESP32 DevKit V1)
- **Framework**: Arduino (`espressif32 @ 6.9.0`)
- **C++ Standard**: C++17
- **Serial speed**: 115200 baud
- **Upload speed**: 230400 baud

### Olimex C6 variant

- Build target: `olimex_esp32_c6_evb`
- Use the pinned pioarduino platform and a separate `PLATFORMIO_CORE_DIR` when building C6, so the pioarduino packages do not interfere with upstream ESP32/NORVI builds.
- Native USB CDC is the serial console path on this board; keep `ARDUINO_USB_CDC_ON_BOOT=1` enabled.
- Olimex local UI hardware smoke order: build, boot without peripherals, wire TFT (3.3 V logic, reset tied high), then KY-040 on GPIO 4/5/12.

## Build Commands

```bash
# Build default env (esp32dev)
pio run

# Build specific environment
pio run -e esp32dev

# Clean and rebuild
pio run --target clean && pio run

# Build with verbose output (debug build issues)
pio run --verbose

# Build with platformio check (static analysis)
pio check --environment esp32dev --skip-packages
```

## Flash (Serial)

```bash
# Upload firmware via serial
pio run --target upload

# Upload to specific port
pio run --target upload --upload-port /dev/ttyUSB0

# Upload SPIFFS filesystem data (from data/ directory)
pio run --target uploadfs
```

## Monitor

```bash
# Open serial monitor
pio device monitor

# With specific port and speed
pio device monitor --port /dev/ttyUSB0 --baud 115200

# With filters enabled (as configured in platformio.ini):
# - esp32_exception_decoder: decodes panic backtraces
# - log2file: saves output to file
# - time: adds timestamp
pio device monitor --filter esp32_exception_decoder --filter time
```

## OTA Updates

From `platformio.ini:53-61` (uncomment to enable):

```bash
# Build OTA firmware
pio run -e esp32dev

# Upload via OTA
pio run --target upload --upload-port pool-controller.local
# Or using IP:
pio run --target upload --upload-port 192.168.1.100

# OTA requires:
# - upload_protocol = espota
# - upload_flags = --port=3232 --auth=YOUR_OTA_PASSWORD
```

## Project Structure for PlatformIO

```text
pool-controller/
├── platformio.ini       # Build configuration
├── src/                 # Source files
│   ├── main.cpp         # Entry: setup() + loop()
│   ├── PoolController.* # Core orchestrator
│   ├── Config.*         # Pin definitions, constants
│   ├── ...              # All other modules
│   └── Nodes/           # Sub-node implementations
├── lib/                 # Private libraries
│   └── Vector/          # Custom Vector implementation
├── data/                # LittleFS filesystem data
│   └── web/             # Web UI files (index.html, style.css, app.js)
└── .pio/                # Build artifacts (gitignored)
```

## Serial Debug Output Patterns

When monitoring, look for these key patterns:

```text
✓ Controller setup completed. Free heap: X B     # Normal boot
→ Boot counter: N                                 # Boot counter (see SystemMonitor)
✖ BOOT-LOOP DETECTED                              # Safe mode activated (P8)
✓ Pin configuration validated                     # GPIOs OK
CRITICAL: Free heap X bytes < 8192 bytes           # Memory exhaustion → reboot
WARNING: Low memory detected                       # Memory warning threshold
→ Safe-mode: 5 min stable — boot-loop counter cleared  # Recovery from safe mode
```

## Dependency Management

Dependencies are defined in `platformio.ini` `[common_env_data]` section:

- **lib_deps** uses exact versions (e.g., `ArduinoJson @ 7.3.0`, `NTPClient @ 3.2.1`)
- Git-based deps: `https://github.com/YuriiSalimov/RelayModule.git#v.1.1.2`
- Some libs need `lib_ignore` to resolve conflicts (see `CODING_GUIDELINES.md` §5.1)

To update a dependency:

1. Change the version in `platformio.ini`
2. Run `pio run` to download and build
3. Verify no compilation errors
4. Run `pio check` for regressions

## Troubleshooting

| Issue                                             | Likely Fix                                          |
| ------------------------------------------------- | --------------------------------------------------- |
| Build fails on first clone                        | Run `pio run` to auto-download all dependencies     |
| Serial monitor gibberish                          | Check baud rate (115200) matches `SERIAL_SPEED`     |
| Upload fails                                      | Hold BOOT button on ESP32 during connection         |
| OTA fails                                         | Verify `upload_port` resolves (mDNS or IP)          |
| Missing libraries                                 | `pio update` to refresh library index               |
| ESP32 not detected                                | Install USB-to-UART drivers (CP210x/CH340)          |
| DHT sensor library (in deps but not used in src/) | Potential stale dependency — verify before removing |
