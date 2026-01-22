# Over-The-Air (OTA) Updates

## Overview

The Pool Controller supports Over-The-Air (OTA) firmware updates, allowing
you to update the device remotely without physical access to the hardware.
This feature is provided by the Homie library and is enabled by default.

## Features

- **Network-based updates**: Upload firmware via WiFi
- **Password-protected**: Secure updates with authentication
- **Low memory footprint**: Optimized for ESP8266/ESP32
- **Automatic discovery**: mDNS support for easy device location
- **Status feedback**: Progress indication via MQTT

## Prerequisites

- Pool Controller connected to WiFi network
- PlatformIO installed (for uploading firmware)
- Device IP address or mDNS hostname
- OTA password (configured in Homie config)

## Update Methods

### Method 1: PlatformIO OTA Upload (Recommended)

#### 1. Configure Upload Settings

Edit `platformio.ini` and uncomment/modify OTA settings:

```ini
[env:nodemcuv2]
; ... existing settings ...
upload_protocol = espota
upload_port = pool-controller.local  ; or IP address like 192.168.1.100
upload_flags =
  --timeout=30
  --port=8266
  --auth=YOUR_OTA_PASSWORD
```

#### 2. Upload Firmware

```bash
# Build and upload via OTA
pio run -e nodemcuv2 --target upload

# Or using platform-specific environment
pio run -e esp32dev --target upload
```

#### 3. Monitor Progress

The upload progress will be shown in the terminal. After completion, the
device will automatically reboot with the new firmware.

### Method 2: Arduino IDE OTA Upload

1. Open Arduino IDE
2. Go to **Tools → Port**
3. Select your device from the network ports list
   (e.g., `pool-controller at 192.168.1.100`)
4. Click Upload button
5. Enter OTA password when prompted

### Method 3: Web Interface (Homie UI)

1. Access Homie web interface at `http://pool-controller.local/`
   or `http://[DEVICE_IP]/`
2. Navigate to **Firmware Update** section
3. Select compiled `.bin` file
4. Click **Upload**
5. Wait for update completion and automatic reboot

## OTA Configuration

### Setting OTA Password

The OTA password is configured through the Homie configuration portal:

1. **Reset device** to enter configuration mode (hold button during boot)
2. Connect to WiFi AP `Homie-XXXXXX`
3. Open browser to `http://192.168.123.1`
4. Set **OTA Password** in the configuration
5. Save and reboot

### Homie Configuration File

OTA settings are stored in the Homie configuration:

```json
{
  "name": "Pool Controller",
  "wifi": {
    "ssid": "YourWiFiSSID",
    "password": "YourWiFiPassword"
  },
  "mqtt": {
    "host": "192.168.1.10",
    "port": 1883
  },
  "ota": {
    "enabled": true
  },
  "device_id": "pool-controller"
}
```

## Building Firmware for OTA

### Create Firmware Binary

```bash
# Build firmware without uploading
pio run -e nodemcuv2

# Binary location
.pio/build/nodemcuv2/firmware.bin
```

### ESP32 Build

```bash
# Build for ESP32
pio run -e esp32dev

# Binary location
.pio/build/esp32dev/firmware.bin
```

## Security Best Practices

### 1. Set Strong OTA Password

- Use minimum 8 characters
- Include uppercase, lowercase, numbers, symbols
- Example: `MyP00l#Update2026`

### 2. Network Security

- Use WPA2/WPA3 WiFi encryption
- Isolate IoT devices on separate VLAN if possible
- Restrict OTA port (8266) at firewall level

### 3. Firmware Verification

- Always verify firmware builds before uploading
- Test on development device first
- Keep backup of working firmware version

## Troubleshooting

### OTA Upload Fails

**Problem**: Upload fails with timeout error

**Solutions**:

- Verify device is online: `ping pool-controller.local`
- Check firewall allows port 8266
- Ensure correct OTA password
- Verify device has sufficient free memory (>50KB)
- Try increasing timeout in `upload_flags`

### Device Not Found

**Problem**: Device not visible in network ports

**Solutions**:

- Check mDNS is working: `avahi-browse -a` (Linux) or Bonjour (Windows)
- Use IP address instead of mDNS hostname
- Restart device and wait for WiFi connection
- Check device is on same network/subnet

### Upload Successful but Device Not Responding

**Problem**: Upload completes but device doesn't reboot or run new firmware

**Solutions**:

- Check serial console for boot errors
- Verify firmware was built for correct platform (ESP8266/ESP32)
- Ensure firmware size fits in flash memory
- Check for memory issues in serial log
- Perform manual reboot

### Memory Issues During OTA

**Problem**: OTA fails due to insufficient memory

**Solutions**:

- Free memory is critical for OTA
- System monitor will prevent OTA if memory < 8KB (ESP32) or 4KB (ESP8266)
- Reboot device before OTA attempt
- Reduce logging during update

## OTA Architecture

### How It Works

1. **Homie OTA Server**: Runs on port 8266 (ESP8266) or 3232 (ESP32)
2. **mDNS Advertisement**: Device broadcasts `_arduino._tcp` service
3. **Authentication**: Password challenge before accepting firmware
4. **Flash Writing**: New firmware written to OTA partition
5. **Verification**: Boot partition updated to new firmware
6. **Reboot**: Automatic restart with new firmware

### Memory Requirements

- **ESP8266**: Minimum 50KB free heap for OTA
- **ESP32**: Minimum 100KB free heap for OTA
- **Flash**: Sufficient space for dual boot partitions

### LWIP Configuration

The project uses `PIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY` flag to optimize
network stack memory usage, ensuring reliable OTA updates even on ESP8266
with limited RAM.

## Monitoring OTA Status

### Via MQTT

OTA progress is published to MQTT topics:

```text
homie/pool-controller/$state = "ota"       # During OTA update
homie/pool-controller/$state = "ready"     # After successful update
```

### Via Serial Console

Connect to serial port to monitor OTA progress:

```bash
# PlatformIO monitor
pio device monitor -e nodemcuv2

# Look for log messages
[OTA] Start
[OTA] Progress: 25%
[OTA] Progress: 50%
[OTA] Progress: 75%
[OTA] Success
```

## Automation Examples

### Automated OTA Updates Script

```bash
#!/bin/bash
# ota-update.sh

DEVICE_IP="192.168.1.100"
OTA_PASSWORD="MyP00l#Update2026"
FIRMWARE=".pio/build/nodemcuv2/firmware.bin"

# Build firmware
echo "Building firmware..."
pio run -e nodemcuv2

# Upload via OTA
echo "Uploading to $DEVICE_IP..."
python ~/.platformio/packages/framework-arduinoespressif8266/tools/espota.py \
  -i $DEVICE_IP \
  -p 8266 \
  -a $OTA_PASSWORD \
  -f $FIRMWARE

echo "Update complete!"
```

### GitHub Actions CI/CD (Example)

```yaml
name: Build and Deploy OTA

on:
  push:
    tags:
      - 'v*'

jobs:
  build-and-deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      
      - name: Install PlatformIO
        run: pip install platformio
      
      - name: Build Firmware
        run: pio run -e nodemcuv2
      
      - name: Upload via OTA
        env:
          DEVICE_IP: ${{ secrets.DEVICE_IP }}
          OTA_PASSWORD: ${{ secrets.OTA_PASSWORD }}
        run: |
          python ~/.platformio/packages/framework-arduinoespressif8266/tools/espota.py \
            -i $DEVICE_IP \
            -p 8266 \
            -a $OTA_PASSWORD \
            -f .pio/build/nodemcuv2/firmware.bin
```

## Version Management

### Firmware Versioning

Update version in `src/PoolController.cpp`:

```cpp
const char* FIRMWARE_VERSION = "3.1.0";
```

### MQTT Version Publishing

Version is published automatically:

```text
homie/pool-controller/$fw/version = "3.1.0"
homie/pool-controller/$fw/name = "pool-controller"
```

## Best Practices

### 1. Test Before Production

- Always test new firmware on development device
- Verify all features work after OTA update
- Check MQTT connectivity and state restoration

### 2. Staged Rollout

- Update one device first
- Monitor for 24 hours
- Roll out to remaining devices if stable

### 3. Backup Current Firmware

```bash
# Backup current firmware before update
pio run -e nodemcuv2
cp .pio/build/nodemcuv2/firmware.bin \
   backups/firmware-v3.1.0-$(date +%Y%m%d).bin
```

### 4. Schedule Updates

- Perform OTA during low-activity periods
- Avoid updates during critical pool operation
- Consider scheduled maintenance windows

## Recovery Procedures

### OTA Update Failure Recovery

If OTA update fails and device becomes unresponsive:

1. **Physical Access Recovery**:
   - Connect via USB serial
   - Upload firmware via serial: `pio run -e nodemcuv2 --target upload`

2. **Bootloader Recovery**:
   - ESP8266/ESP32 bootloader allows serial recovery
   - Hold BOOT button during power-on
   - Upload firmware via esptool

3. **Factory Reset**:
   - Clear EEPROM/NVS
   - Reset Homie configuration
   - Reconfigure via Homie AP

## Future Enhancements

- [ ] Web-based OTA update interface
- [ ] Automatic update checking from GitHub releases
- [ ] Rollback capability to previous firmware
- [ ] A/B partition updates for safer updates
- [ ] Update scheduling via MQTT commands

## References

- [Homie OTA Documentation](https://homieiot.github.io/homie-esp8266/docs/develop/others/ota-configuration-updates/)
- [PlatformIO OTA Guide](https://docs.platformio.org/en/latest/platforms/espressif8266.html#over-the-air-ota-update)
- [Arduino OTA Documentation](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html)
- [ESP8266 OTA Updates](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)

## Support

For OTA-related issues:

- Open issue: <https://github.com/smart-swimmingpool/pool-controller/issues>
- Discussions: <https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions>

---

**Note**: OTA functionality is enabled by default through the Homie library.
No code changes required in the application - just configure upload settings
in `platformio.ini` and use PlatformIO OTA upload feature.
