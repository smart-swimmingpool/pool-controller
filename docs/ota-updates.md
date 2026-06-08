---
title: OTA Updates
summary: Over-The-Air firmware update guide for the ESP32 Pool Controller — WebUI upload, GitHub Release auto-updates, PlatformIO serial flashing, and security best practices
date: "2026-06-07"
lastmod: "2026-06-07"
draft: false
toc: true
type: docs
tags: ["docs", "ota", "firmware", "update", "esp32"]
menu:
  docs:
    parent: Pool Controller
    name: OTA Updates
    weight: 55
---

# Over-The-Air (OTA) Updates

## Overview

The Pool Controller supports Over-The-Air (OTA) firmware updates, allowing
you to update the device remotely without physical access to the hardware.

## Features

- **WebUI firmware upload**: Flash a signed `.bin` via the dashboard
- **GitHub Release updates**: Check for new versions and install automatically
- **PlatformIO serial upload**: Flash via USB for development
- **Status feedback**: Progress indication via WebUI + MQTT

## Update Methods

### Method 1: WebUI (Recommended)

The simplest method — no tools needed beyond a web browser:

1. Open the Pool Controller dashboard in your browser
2. Go to **System** tab
3. Choose one of:
   - **Manual Firmware Upload**: Select a `.bin` file and flash
   - **Check for Updates**: Automatically fetches latest release from GitHub
4. Follow the progress bar — device reboots when done

### Method 2: GitHub Release (Automated)

Firmware is built and published automatically on each GitHub release:

1. In the **System** tab, click **Check for Updates**
2. If a newer version is found, **Install Update** appears
3. Click to download and flash the latest firmware directly from GitHub
4. Progress is shown in the UI; device reboots on completion

### Method 3: PlatformIO Serial Upload

For development:

```bash
# Build and upload via USB
pio run -e esp32dev --target upload

# Or with project venv
./venv/bin/pio run -e esp32dev --target upload
```

## Building Firmware for OTA

### Create Firmware Binary

```bash
# Build firmware without uploading
pio run -e esp32dev

# Binary location
.pio/build/esp32dev/firmware.bin
```

## Security Best Practices

### 1. Network Security

- Use WPA2/WPA3 Wi-Fi encryption
- Isolate IoT devices on separate VLAN if possible

### 2. Firmware Verification

- Only flash firmware from trusted sources (official GitHub releases)
- Verify firmware builds before uploading to production
- Test on development device first
- Keep backup of working firmware version

## Troubleshooting

### OTA Upload Fails

**Problem**: Upload fails with timeout error

**Solutions**:

- Verify device is online: `ping pool-controller.local`
- Ensure device has sufficient free memory (>50KB)
- Try increasing timeout in `upload_flags`

### Device Not Found

**Problem**: Device not visible in network ports

**Solutions**:

- Check mDNS is working: `avahi-browse -a` (Linux) or Bonjour (Windows)
- Use IP address instead of mDNS hostname
- Restart device and wait for Wi-Fi connection
- Check device is on same network/subnet

### Upload Successful but Device Not Responding

**Problem**: Upload completes but device doesn't reboot or run new firmware

**Solutions**:

- Check serial console for boot errors
- Verify firmware was built for correct platform (ESP32)
- Ensure firmware size fits in flash memory
- Check for memory issues in serial log
- Perform manual reboot

### Memory Issues During OTA

**Problem**: OTA fails due to insufficient memory

**Solutions**:

- Free memory is critical for OTA
- System monitor will prevent OTA if memory < 8KB
- Reboot device before OTA attempt
- Reduce logging during update

## OTA Architecture

### How It Works

1. **WebUI OR GitHub**: User triggers update via dashboard or automated check
2. **Firmware Download**: Binary fetched from local upload or GitHub release
3. **Flash Writing**: New firmware written to OTA partition (ESP32 Update library)
4. **Verification**: Firmware signature and integrity checked
5. **Reboot**: Automatic restart with new firmware

### Memory Requirements

- **ESP32**: Minimum 100KB free heap for OTA
- **Flash**: Sufficient space for dual boot partitions

### LWIP Configuration

The project uses `PIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY` flag to optimize
network stack memory usage, ensuring reliable OTA updates.

## Monitoring OTA Status

OTA progress is visible in the WebUI progress bar and via serial console.

```bash
# PlatformIO monitor
pio device monitor

# Look for log messages
[OTA] Start
[OTA] Progress: 25%
[OTA] Progress: 50%
[OTA] Progress: 75%
[OTA] Success
```

## Recovery Procedures

### OTA Update Failure Recovery

If OTA update fails and device becomes unresponsive:

1. **Physical Access Recovery**:

   - Connect via USB serial
   - Upload firmware via serial: `pio run -e esp32dev --target upload`

1. **Bootloader Recovery**:

   - ESP32 bootloader allows serial recovery
   - Hold BOOT button during power-on
   - Upload firmware via esptool

1. **Factory Reset**:
   - In WebUI: System tab → **Factory Reset**
   - Or serial command: clear NVS + LittleFS

## Future Enhancements

- [x] Web-based OTA update interface (System tab)
- [x] Automatic update checking from GitHub releases
- [ ] Rollback capability to previous firmware
- [ ] A/B partition updates for safer updates
- [ ] Update scheduling via MQTT commands

## References

- [PlatformIO OTA Guide](https://docs.platformio.org/en/latest/platforms/espressif32.html#over-the-air-ota-update)
- [ESP32 Arduino OTA](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ota.html)
- [GitHub Releases](https://github.com/smart-swimmingpool/pool-controller/releases)

## Support

For OTA-related issues:

- Open issue: <https://github.com/smart-swimmingpool/pool-controller/issues>
- Discussions: <https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions>

---

**Note**: OTA is implemented using the ESP32 Arduino Update library integrated
into the WebPortal. GitHub release builds are handled by the CI pipeline
(`release.yml`).
