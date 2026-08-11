---
name: platformio-env
description: "PlatformIO environment configuration for the pool-controller — platformio.ini deep dive, build environments, library management, platform/board selection, build flags, optimization, Python venv, and CI integration. 🇩🇪 Deutsche Trigger: PlatformIO Umgebung, platformio.ini Konfiguration, Build-Environment, Library-Management, Build-Flags, Plattform-Konfiguration, ESP32 Board, Compiler-Optimierung, Python venv, CI/CD PlatformIO."
keywords:
  - platformio umgebung
  - platformio environment
  - platformio.ini
  - build environment
  - build konfiguration
  - library management
  - library dependencies
  - build flags
  - compiler flags
  - esp32 board
  - espressif32 platform
  - arduino framework
  - python venv
  - pio cli
  - ci integration
  - platformio check
  - olimex esp32 c6 evb
  - pioarduino
  - platformio core dir
---

# PlatformIO Environment — Pool Controller

PlatformIO environment configuration for the ESP32 pool-controller. This skill
covers the build system, platform configuration, library management, and
environment setup — complementing the `platformio-workflow` skill for
operational commands.

## Olimex ESP32-C6-EVB Notes

- Use the `olimex_esp32_c6_evb` environment for the Olimex board.
- ESP32-C6 Arduino builds require the pinned pioarduino platform; keep a separate `PLATFORMIO_CORE_DIR` for C6/pioarduino builds versus upstream ESP32/NORVI builds.
- Native USB CDC is the serial console path; keep `ARDUINO_USB_CDC_ON_BOOT=1` enabled.
- Local UI wiring lives in `src/Config.hpp`; treat it as the source of truth for TFT, encoder, relay, and sensor pins.

## Architecture Overview

```text
pool-controller/
├── platformio.ini          ← Single source of truth for build config
├── src/                    ← Source code
├── lib/                    ← Private libraries (Vector/)
├── data/                   ← SPIFFS filesystem assets
├── venv/                   ← Python virtual environment for PlatformIO CLI
├── .pio/
│   ├── build/esp32dev/     ← Build artifacts (compiled .o, .elf, .bin)
│   └── libdeps/esp32dev/   ← Downloaded library dependencies
├── test/                   ← Unit tests (not yet created)
└── CPPLINT.cfg             ← Cpplint config for static analysis
```

Python venv at `venv/` provides PlatformIO CLI (`venv/bin/pio`, `venv/bin/piodebuggdb`).

## 1. platformio.ini Deep Dive

### `[platformio]` — Global Settings

```ini
[platformio]
default_envs = esp32dev
```

- `default_envs`: Which environment builds when you run `pio run` without `-e`. Currently only `esp32dev`.
- Other options: `src_dir`, `lib_dir`, `extra_configs` (include additional .ini files).

### `[common]` — Shared Build Flags

```ini
[common]
build_flags =
  '-D FW_VERSION="3.2.0"'    ; Auto-maintained by release-please — do NOT edit manually
  -std=c++17                  ; C++17 standard
  -Wno-deprecated-declarations ; Suppress library deprecation warnings
serial_speed = 115200
```

- `FW_VERSION` is updated by `release-please` via regex search in CI — manual edits will be overwritten
- `serial_speed` is a **custom variable** referenced elsewhere as `${common.serial_speed}`
- Debug build flags (`-g -DDEBUG_PORT=Serial`) are commented out — uncomment for debug builds

### `[common_env_data]` — Shared Library Dependencies

```ini
[common_env_data]
lib_deps =
  DallasTemperature                           ; DS18B20 sensor library
  Wire                                        ; I2C bus
  paulstoffregen/OneWire                      ; OneWire protocol for DS18B20
  Adafruit Unified Sensor                     ; Sensor abstraction layer
  DHT sensor library                          ; ⚠ NOT USED in source — candidate for cleanup
  https://github.com/YuriiSalimov/RelayModule.git#v.1.1.2  ; Relay control (Git pin)
  NTPClient @ 3.2.1                           ; Exact version pin
  TimeZone @ 1.2.4                            ; Exact version pin
  bblanchon/ArduinoJson @ 7.3.0               ; JSON parsing — major version matters
  thomasfredericks/Bounce2                    ; Button debouncing
  knolleary/PubSubClient @ 2.8                ; MQTT client
```

**Dependency specification formats**:

| Format              | Example                  | Use Case                              |
| ------------------- | ------------------------ | ------------------------------------- |
| `LibName`           | `DallasTemperature`      | Simplest — takes latest from registry |
| `owner/LibName`     | `paulstoffregen/OneWire` | Registry + specific owner             |
| `LibName @ version` | `NTPClient @ 3.2.1`      | Exact version pinning                 |
| `LibName >=x.y.z`   | `ArduinoJson >=7.0.0`    | Minimum version                       |
| Git URL + `#tag`    | `...#v.1.1.2`            | GitHub dependency at specific tag     |

### `[env:esp32dev]` — Build Environment

```ini
[env:esp32dev]
platform = espressif32 @ 6.9.0    ; ESP32 Arduino platform (includes ESP-IDF 5.x)
board = esp32dev                   ; ESP32 DevKit V1 board definition
framework = arduino                ; Arduino as IoT framework
build_flags = -D SERIAL_SPEED=${common.serial_speed}
build_unflags = -Werror=reorder    ; Remove problematic warning-as-error flag
lib_deps = ${common_env_data.lib_deps}
monitor_speed = ${common.serial_speed}
monitor_filters = esp32_exception_decoder, log2file, time, default
upload_speed = 230400
```

#### Key Fields Explained

| Field             | Current Value                                      | Purpose                                              |
| ----------------- | -------------------------------------------------- | ---------------------------------------------------- |
| `platform`        | `espressif32 @ 6.9.0`                              | ESP32 Arduino core, includes ESP-IDF 5.x, FreeRTOS   |
| `board`           | `esp32dev`                                         | Board definition: flash size, partitioning, CPU freq |
| `framework`       | `arduino`                                          | Arduino wiring framework on top of ESP-IDF           |
| `build_flags`     | `-D SERIAL_SPEED=115200`                           | Passed to compiler (gcc)                             |
| `build_unflags`   | `-Werror=reorder`                                  | Removes a flag that would otherwise be set           |
| `lib_deps`        | inherits from `[common_env_data]`                  | All project dependencies                             |
| `monitor_filters` | `esp32_exception_decoder, log2file, time, default` | Serial output processing filters                     |
| `upload_speed`    | `230400`                                           | Serial flash speed                                   |

### OTA Configuration (Commented)

```ini
;upload_protocol = espota
;upload_port = pool-controller.local
;upload_flags =
;  --timeout=30
;  --port=3232
;  --auth=YOUR_OTA_PASSWORD
```

OTA disabled by default. Enable by uncommenting and setting a strong password.
See `platformio-workflow` skill for OTA commands.

## 2. Build Environments — Adding New Targets

Current: single environment `esp32dev`. Multiple environments allow building for different targets.

**Example: Adding a debug environment**:

```ini
[env:esp32dev-debug]
platform = ${env:esp32dev.platform}
board = ${env:esp32dev.board}
framework = ${env:esp32dev.framework}
build_flags =
  ${env:esp32dev.build_flags}
  -D DEBUG_PORT=Serial
  -g
  -D LOG_LEVEL=4  ; Verbose logging
lib_deps = ${common_env_data.lib_deps}
monitor_filters = ${env:esp32dev.monitor_filters}
upload_speed = ${env:esp32dev.upload_speed}

[env:esp32dev-release]
platform = ${env:esp32dev.platform}
board = ${env:esp32dev.board}
framework = ${env:esp32dev.framework}
build_flags =
  ${env:esp32dev.build_flags}
  -D NDEBUG           ; Disable assertions
  -Os                 ; Optimize for size
  -D LOG_LEVEL=1      ; Minimal logging
lib_deps = ${common_env_data.lib_deps}
```

**Environment inheritance**: Use `${env:<name>.<field>}` to inherit from another environment and override only what differs.

## 3. Platform & Board Selection

### Current Platform: `espressif32 @ 6.9.0`

This is the ESP32 Arduino platform from PlatformIO registry:

- **Package**: `framework-arduinoespressif32` (Arduino core for ESP32)
- **ESP-IDF**: Ships with ESP-IDF 5.x (important for API compatibility)
- **Toolchain**: `xtensa-esp32-elf-gcc` (gcc-based)

To check available versions:

```bash
pio platform show espressif32
```

### Board Definition: `esp32dev`

The board file defines:

- **Flash size**: 4MB (default)
- **Partition table**: Default ESP32 partition layout
- **CPU frequency**: 240MHz
- **Upload protocol**: esptool (serial) / espota (OTA)

To inspect board config:

```bash
pio run -e esp32dev --target board
```

## 4. Build Flags Reference

### Currently Used

| Flag                           | Effect                                                                       |
| ------------------------------ | ---------------------------------------------------------------------------- |
| `-std=c++17`                   | Enable C++17 features (`if constexpr`, `std::optional`, structured bindings) |
| `-D FW_VERSION="3.2.0"`        | Firmware version macro, accessible as `FW_VERSION` in code                   |
| `-D SERIAL_SPEED=115200`       | Serial baud rate, accessible as `SERIAL_SPEED` macro                         |
| `-Wno-deprecated-declarations` | Suppress warnings for deprecated API usage (library compat)                  |

Define macros in code with:

```cpp
#ifdef FW_VERSION
  Serial.printf("Firmware: %s\n", FW_VERSION);
#endif
```

### Common Additional Flags for ESP32

| Flag                                | Purpose                                             |
| ----------------------------------- | --------------------------------------------------- |
| `-Os`                               | Optimize for binary size (release builds)           |
| `-Og`                               | Optimize for debugging                              |
| `-D LOG_LEVEL=4`                    | Runtime log level control                           |
| `-D NDEBUG`                         | Disable `assert()` statements                       |
| `-DCORE_DEBUG_LEVEL=5`              | ESP-IDF debug output level (0=no output, 5=verbose) |
| `-mno-allow-interrupts-in-sections` | Interrupt safety                                    |
| `-Werror`                           | Treat all warnings as errors (CI)                   |
| `-flto`                             | Link-Time Optimization (reduces binary size)        |
| `-D CONFIG_SECURE_DISABLE_OCD=1`    | Disable JTAG for production                         |

## 5. Library Management

### Where Libraries Are Stored

After running `pio run`, dependencies are downloaded to:

```text
.pio/libdeps/esp32dev/
├── ArduinoJson/
├── Bounce2/
├── DallasTemperature/
├── NTPClient/
├── OneWire/
├── PubSubClient/
├── RelayModule/
├── TimeZone/
├── Wire/
└── ...
```

The `nodemcuv2/` subdirectory also exists (legacy ESP8266 support — pre-v3.2.0).

### Library Conflict Resolution

From `CODING_GUIDELINES.md` §5.1:

```ini
; When two libraries clash (e.g., ESP Async WebServer), use lib_ignore:
lib_ignore = ESP Async WebServer
lib_deps = https://github.com/me-no-dev/ESPAsyncWebServer.git
```

**Current project**: No active conflicts — all lib_deps resolve cleanly.

### Adding a New Library

```bash
# Search PlatformIO registry
pio lib search "library-name"

# Add to platformio.ini under [common_env_data] lib_deps:
#   lib_deps += owner/LibraryName @ x.y.z

# Then build to download and verify:
pio run
```

### Library Version Pinning Strategy

- **Stable, slow-moving libs** (Wire, OneWire): No version pin (always latest)
- **Active-development libs** (ArduinoJson, PubSubClient, NTPClient): Exact version `@ x.y.z`
- **Git-hosted libs** (RelayModule): Tag-pinned via `#v.1.1.2`
- **Adafruit ecosystem**: Follows Adafruit's own release cycle

## 6. Python Virtual Environment

PlatformIO is installed in `venv/`:

```bash
venv/bin/pio --version
venv/bin/piodebuggdb
```

### Managing the Venv

```bash
# Activate venv
source venv/bin/activate

# Update PlatformIO
pip install --upgrade platformio

# Install additional tools
pip install platformio platformio-debug

# Deactivate
deactivate
```

### Global PlatformIO State

```text
~/.platformio/
├── platforms/        # Installed platform packages (espressif32, etc.)
├── packages/         # Toolchains, frameworks (toolchain-xtensa-esp32, etc.)
├── boards/           # Board definitions
└── lib/              # Global library cache
```

Clear/update cached state:

```bash
pio update        # Update package indexes
pio platform update  # Update installed platforms
pio lib update    # Update library indexes
```

## 7. PlatformIO Check (Static Analysis)

```bash
# Run static analysis on current env
pio check --environment esp32dev

# Skip library code (only check project source)
pio check --environment esp32dev --skip-packages

# Check specific files
pio check --files src/PoolController.cpp

# Flags defined in platformio.ini can configure check severity
```

**Note**: `pio check` runs cppcheck under the hood. It catches:

- Memory leaks
- Uninitialized variables
- Buffer overflows
- Dead code
- Style violations (controlled by `CPPLINT.cfg`)

## 8. CI/CD Integration

### GitHub Actions

Two workflows reference PlatformIO:

**`.github/workflows/plaform.io.yml`**:

```yaml
- name: Run PlatformIO Check
  run: platformio check --environment ${{ matrix.environment }} --skip-packages
- name: Build PlatformIO Project
  run: platformio run --environment ${{ matrix.environment }}
```

**`.github/workflows/linter.yml`**: Super-Linter runs cpplint separately.

### GitLab CI (`.gitlab-ci.yml`)

Mirror of the same build process for self-hosted runners.

### Travis CI (`.travis-ci.yml`)

Legacy — may no longer be active.

## 9. Troubleshooting Environment Issues

| Problem                          | Likely Cause                   | Fix                                                       |
| -------------------------------- | ------------------------------ | --------------------------------------------------------- |
| `Unknown board: esp32dev`        | Platform package not installed | `pio platform install espressif32`                        |
| `Could not find the library XYZ` | Library index stale            | `pio lib update && pio run`                               |
| `fatal error: esp_task_wdt.h`    | ESP-IDF version mismatch       | Check `espressif32` platform version                      |
| `undefined reference to ...`     | Missing library in lib_deps    | Add required library                                      |
| Build succeeds, OTA fails        | Wrong upload protocol          | Set `upload_protocol = espota`                            |
| `pio: command not found`         | PlatformIO not in PATH         | Use `venv/bin/pio` or activate venv                       |
| Python version conflict          | Venv out of date               | `python3 -m venv venv && venv/bin/pip install platformio` |
| `monitor_filters` not working    | Outdated PlatformIO            | `venv/bin/pip install --upgrade platformio`               |

## 10. Environment Quick Reference

```bash
# Platform & toolchain info
pio system info                     # PlatformIO version + system details
pio platform list                   # Installed platforms
pio platform show espressif32       # Platform details + available versions

# Library management
pio lib list                        # Installed libraries
pio lib show ArduinoJson            # Library details
pio lib search "DS18B20"           # Search registry

# Build info
pio run -e esp32dev --target size    # Flash/RAM usage
pio run -e esp32dev --target compiledb  # Generate compile_commands.json
pio run -e esp32dev --verbose        # Full compiler commands

# Clean
pio run --target clean               # Remove build artifacts
pio run --target fullclean           # Full clean (including .pio)
```
