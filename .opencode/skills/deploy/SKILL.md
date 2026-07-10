---
name: deploy
description: "Deployment workflow for the pool-controller ESP32 — build, serial flash, uploadfs, OTA update, release-please semver pipeline, version management, and post-deploy verification. Use when asked to deploy, flash, upload, release, or update firmware. 🇩🇪 Deutsche Trigger: Deployment, Flashen, Firmware einspielen, OTA Update, Release erstellen, Version verwalten, semver, uploadfs, ausrollen."
keywords:
  - deployment
  - flashing
  - firmware upload
  - ota update
  - release
  - semver
  - version management
  - release-please
  - serial flash
  - uploadfs
  - web filesystem
  - littlefs
  - pio run
  - esptool
  - deploy firmware
  - pool-controller deploy
  - firmware release
  - version bump
  - ausrollen
  - flashen
---

# Deploy — Pool Controller

Complete deployment workflow for the pool-controller ESP32 firmware: build,
serial flash, web filesystem upload, OTA update, and semver release management.

## Overview

```text
┌─────────────┐    ┌──────────────┐    ┌──────────────────────┐    ┌─────────────────┐
│  Pre-flight  │ → │  Build       │ → │  Deploy              │ → │  Verify         │
│  - lint      │    │  pio run     │    │                      │    │  - monitor logs │
│  - format    │    │  -e norvi    │    │  Serial:             │    │  - version check│
│  - version   │    │              │    │    flash + uploadfs  │    │  - web UI       │
└─────────────┘    └──────────────┘    │    OTA:               │    └─────────────────┘
                                       │    1. flash firmware  │
                                       │    2. upload web fs   │
                                       │       via /api/fs/    │
                                       │       upload (6 files)│
                                       └──────────────────────┘
```

## Prerequisites

```bash
# PlatformIO environment (project venv)
./venv/bin/pio --version

# Or globally installed
pio --version
```

If `pio` is not in PATH, use the project's virtual environment:

```bash
./venv/bin/pio <command>
```

## Pre-flight Checks

### Version consistency

Before deploying, verify the firmware version across all sources:

| Source           | File                            | Check                                        |
| ---------------- | ------------------------------- | -------------------------------------------- |
| Build flag       | `platformio.ini:18`             | `-D FW_VERSION="x.y.z"`                      |
| Central fallback | `src/Version.h:11`              | `#define FW_VERSION "0.0.0"` (fallback only) |
| Release manifest | `.release-please-manifest.json` | `"x.y.z"`                                    |
| Git tag          | `git tag -l \| sort -V`         | `vx.y.z` exists                              |

> **Source of truth:** `platformio.ini` build flag `FW_VERSION`. Release-please auto-updates it on release.
> `src/Version.h` is a fallback and also auto-updated by release-please.

```bash
# Check current FW_VERSION build flag
grep 'FW_VERSION' platformio.ini

# Check release manifest
cat .release-please-manifest.json

# List tags
git tag --sort=-v:refname | head -5
```

### Lint & Quality (optional)

```bash
# Static analysis
./venv/bin/pio check --environment esp32dev --skip-packages

# Clang-format
find src/ -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | xargs clang-format --dry-run -Werror
```

## Build

```bash
# Build default environment (esp32dev)
./venv/bin/pio run

# Build specific environment
./venv/bin/pio run -e esp32dev

# Clean and rebuild
./venv/bin/pio run --target clean && ./venv/bin/pio run

# Build with verbose output (debug build issues)
./venv/bin/pio run --verbose
```

Expected output:

```text
RAM:   [==        ]  15.1% (used 49596 bytes from 327680 bytes)
Flash: [========  ]  83.7% (used 1096433 bytes from 1310720 bytes)
========================= [SUCCESS] Took X seconds =========================
```

> **Warning:** If flash usage exceeds ~90%, the firmware may not fit. Consider
> memory optimization (see `cpp-memory-opt` skill).

## Deploy — Serial Flash

### 1. Find the serial port

```bash
# Linux
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
ls -la /dev/serial/by-id/ 2>/dev/null

# macOS
ls /dev/cu.usbserial-* /dev/cu.wchusbserial-* 2>/dev/null
```

Common ESP32 USB-to-UART chips:

- **CP2102** (Silicon Labs) → `/dev/ttyUSB0`
- **CH340** → `/dev/ttyUSB0` or `/dev/ttyCH341USB0`

### 2. Flash firmware + Web filesystem

```bash
# Step A: Flash firmware binary
./venv/bin/pio run --target upload --upload-port /dev/ttyUSB0

# Step B: Upload web filesystem (LittleFS) — required for Web UI
./venv/bin/pio run --target uploadfs --upload-port /dev/ttyUSB0
```

> **Important:** Both steps are required for a complete deployment.
>
> - Step A flashes the compiled firmware (`firmware.bin`)
> - Step B uploads web assets (`data/web/` → LittleFS partition)

### 3. One-liner (if uploadfs is needed)

```bash
./venv/bin/pio run --target upload --upload-port /dev/ttyUSB0 && \
./venv/bin/pio run --target uploadfs --upload-port /dev/ttyUSB0
```

### Serial upload troubleshooting

| Symptom                      | Fix                                                                           |
| ---------------------------- | ----------------------------------------------------------------------------- |
| `Connecting..._____` (stuck) | Hold **BOOT** button on ESP32 during connection, release when flashing starts |
| `Failed to connect`          | Check cable (data, not charge-only), try different USB port                   |
| `Access denied`              | `sudo chmod 666 /dev/ttyUSB0` or add user to `dialout` group                  |
| `No such file or directory`  | USB-to-UART driver missing — install CP210x/CH340 driver                      |
| Upload fails mid-way         | Lower `upload_speed` in `platformio.ini` (e.g. 115200)                        |

## Deploy — OTA Update (Web Upload)

This project uses **web-based OTA** via the device's REST API (`POST /api/update`).
ArduinoOTA (`espota.py`) is **not** supported — there is no ArduinoOTA server running on the device.

> **Requirement:** The device must be online with WiFi configured and reachable on the network.

### 1. Build the firmware

```bash
./venv/bin/pio run -e norvi_ae01_r
# or
./venv/bin/pio run -e esp32dev
```

### 2. Authenticate & upload in one step

```bash
# Step A: Login — POST /api/login with admin password → session cookie saved
curl -c /tmp/ota-cookie.txt \
  -X POST http://<device-ip>/api/login \
  -d "password=<admin-password>"

# Step B: Upload firmware — POST multipart to /api/update with cookie
curl -b /tmp/ota-cookie.txt \
  -F "firmware=@.pio/build/norvi_ae01_r/firmware.bin" \
  http://<device-ip>/api/update
```

Response `HTTP 200 OK` with body `OK` signals success. The device reboots immediately after a successful upload.

> **Important:** The session cookie expires after 600 seconds (10 minutes). Re-login if the upload fails with a 401.

### 3. One-liner (login + upload)

```bash
# Build first, then login + upload in sequence
curl -c /tmp/ota-cookie.txt -s -X POST http://<device-ip>/api/login \
  -d "password=<admin-password>" && \
curl -b /tmp/ota-cookie.txt -s -F "firmware=@.pio/build/norvi_ae01_r/firmware.bin" \
  http://<device-ip>/api/update
```

### 4. OTA + uploadfs (if web files changed)

The `/api/update` endpoint only flashes the firmware binary — LittleFS web assets
(HTML/CSS/JS) are **not** included. After OTA, deploy web assets via the
`/api/fs/upload` endpoint (available since firmware v4.1.1, PR #155).

#### One-liner: firmware + all web assets

```bash
DEVICE=http://pool-controller.local
PASSWORD=admin
COOKIE_JAR=$(mktemp)

# Login
curl -c "$COOKIE_JAR" -s -X POST "$DEVICE/api/login" -d "password=$PASSWORD" > /dev/null

# Flash firmware
curl -b "$COOKIE_JAR" -s -F "firmware=@.pio/build/norvi_ae01_r/firmware.bin" \
  "$DEVICE/api/update" && echo "Firmware flashed, waiting for reboot..."
sleep 20

# Re-login (session was lost on reboot)
curl -c "$COOKIE_JAR" -s -X POST "$DEVICE/api/login" -d "password=$PASSWORD" > /dev/null

# Upload all web assets
for f in index.html app.js style.css sw.js manifest.json icon.svg; do
  curl -b "$COOKIE_JAR" -s -X POST "$DEVICE/api/fs/upload" \
    -F "path=/web/$f" -F "content=@data/web/$f"
done

rm -f "$COOKIE_JAR"
echo "Deployment complete!"
```

#### Manual upload of individual files

```bash
curl -b /tmp/ota-cookie.txt -X POST http://<device-ip>/api/fs/upload \
  -F "path=/web/app.js" \
  -F "content=@data/web/app.js"
```

Response `200 OK` means the file was written to LittleFS. The endpoint:
- Requires authentication (valid session cookie)
- Only allows paths under `/web/` (security)
- Blocks path traversal (`..`)
- Streams multipart uploads — no size limit
- Returns `200 OK` on success (handled by the POST completion handler)

> **Note:** Firmware versions without `/api/fs/upload` (pre-v4.1.1) require
> serial `uploadfs` for web asset deployment. See "Serial Flash" above.

### OTA troubleshooting

| Symptom                              | Fix                                                    |
| ------------------------------------ | ------------------------------------------------------ |
| `HTTP 401` on upload                 | Session expired — re-login (cookie valid 10 min)       |
| `HTTP 429` on login                  | Too many failed attempts — wait for lockout to expire  |
| `curl: (7) Failed to connect`        | Device unreachable — check WiFi, ping the IP/hostname  |
| Upload succeeds but device stays off | Wait ~30s for reboot, then check `/api/status` uptime  |
| Device doesn't boot after OTA        | Serial flash a known-good firmware (see Rollback)      |

## CI/CD Pipeline (GitHub Actions)

The project uses **release-please** for automated semver releases.

### Release workflow

Trigger: Push to `main` branch.

```text
Commit (Conventional Commits) → [release-please] → Release PR
  → Merge PR → [release-please] → GitHub Release + Tag
  → [Build Firmware] → Upload firmware.bin to Release Assets
```

### Conventional Commits

| Prefix              | Version bump      | Example                           |
| ------------------- | ----------------- | --------------------------------- |
| `fix:`              | Patch (x.y.**Z**) | `fix: correct temperature offset` |
| `feat:`             | Minor (x.**Y**.0) | `feat: add heater control`        |
| `feat!:` or `fix!:` | Major (**X**.0.0) | `feat!: drop ESP8266 support`     |
| `chore:`            | No release        | `chore: update dependencies`      |

### Release-please configuration

```json
// release-please-config.json
"extra-files": [
  {
    "type": "generic",
    "path": "platformio.ini",
    "search-regex": "FW_VERSION=\"[^\"]*\""
  },
  {
    "type": "generic",
    "path": "src/Version.h",
    "search-regex": "FW_VERSION \"[^\"]*\""
  }
]
```

Release-please automatically updates `FW_VERSION` in both `platformio.ini` and
`src/Version.h` when a new release is created.

### Manual version bump (without release-please)

```bash
# Update version in platformio.ini
sed -i 's/FW_VERSION="[^"]*"/FW_VERSION="X.Y.Z"/' platformio.ini

# Update version in Version.h
sed -i 's/FW_VERSION "[^"]*"/FW_VERSION "X.Y.Z"/' src/Version.h

# Update release manifest
echo '{".": "X.Y.Z"}' > .release-please-manifest.json

# Create git tag
git tag -a vX.Y.Z -m "Release vX.Y.Z"
```

## Post-deployment Verification

### 1. Serial monitor

```bash
./venv/bin/pio device monitor --port /dev/ttyUSB0 --baud 115200 \
  --filter esp32_exception_decoder --filter time
```

Look for these boot patterns:

```text
✓ Pin configuration validated
✓ Controller setup completed. Free heap: X B     # Normal boot
✓ HA Discovery Device ID set to: pool_controller_...
✓ Web Server running on port 80.
→ Boot counter: N                                 # Boot counter
```

### 2. Version check — Web UI

Open `http://<device-ip>/` in browser → version displayed as `vX.Y.Z` below the title.

### 3. Version check — REST API

```bash
curl http://<device-ip>/api/status | python3 -m json.tool
# → "fw_version": "X.Y.Z"
```

### 4. Version check — Home Assistant

Check device info in Home Assistant → `sw_version` field matches the firmware version.

### 5. Full system diagnostics endpoint

The `/api/status` endpoint returns:

```json
{
  "fw_version": "3.2.0",
  "pool_temp": 25.3,
  "solar_temp": 58.1,
  "ctrl_temp": 32.4,
  "pool_pump": true,
  "solar_pump": false,
  "op_mode": "auto",
  "uptime": 3600,
  "free_heap": 180000,
  "max_alloc": 45000,
  "rssi": -65,
  "wifi_connected": true,
  "mqtt_connected": true,
  "local_ip": "192.168.1.100"
}
```

## Rollback

If a deployed firmware has issues:

1. **Serial flash** the previous known-good `firmware.bin` (download from GitHub Releases)
2. Or rebuild from a specific git tag:

```bash
git checkout v3.1.0
./venv/bin/pio run --target upload --upload-port /dev/ttyUSB0
git checkout main  # return to main branch
```

## Skill Integration

This skill works alongside:

| Skill                  | Purpose                                  |
| ---------------------- | ---------------------------------------- |
| `platformio-workflow`  | General build/monitor/debug commands     |
| `platformio-env`       | Deep platformio.ini configuration        |
| `web-ui`               | Web interface changes (before deploying) |
| `release-please`       | Release pipeline internals (detailed)    |
| `conventional-commits` | Commit format for automatic releases     |
| `cpp-code-quality`     | Linting before deployment                |
| `esp32-reliability`    | 24/7 stability checks                    |
| `cpp-memory-opt`       | Flash/RAM optimization if space is tight |
