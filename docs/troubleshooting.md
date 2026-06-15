---
title: Troubleshooting
summary: Troubleshooting guide for the Pool Controller — common issues, symptoms, causes, and solutions for hardware, software, MQTT, and sensor problems
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "troubleshooting", "guide", "debug"]
menu:
  docs:
    parent: Pool Controller
    name: Troubleshooting
    weight: 120
---

> ⚠️ **WARNING**: The troubleshooting steps below may involve working with
> **230V AC mains voltage** (relay testing, pump wiring). **Always disconnect
> power** before working on the circuit. If unsure, hire a qualified electrician.

## Quick Reference Matrix

| Symptom                             | Cause                                                       | Solution                                                                                    |
| ----------------------------------- | ----------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| **-127 °C** temperature reading     | DS18B20 not detected                                        | Check 4.7 kΩ pull-up resistor, wiring, and solder joints                                    |
| **HA finds no device**              | MQTT Discovery payload not published                        | Verify `mqtt-protocol = "homeassistant"`, check MQTT broker connectivity, use MQTT Explorer |
| **Relay behavior inverted**         | Active-low module configured as active-high (or vice versa) | Set `relay-invert = true` or `false` in Configuration → Advanced                            |
| **Safe Mode active**                | Boot-loop detected (4 consecutive short boots < 5 min)      | Check serial log for crash reason, fix issue, reboot                                        |
| **OTA update fails**                | TLS issue or insufficient flash space                       | Use USB flashing fallback, check free space on device                                       |
| **Web UI not loading**              | ESP32 not on network or wrong IP                            | Check router DHCP list, verify WiFi connection, try `pool-controller.local`                 |
| **MQTT connection refused**         | Wrong credentials or broker unreachable                     | Verify broker IP, port, username/password in MQTT settings                                  |
| **Temperature reads 85 °C**         | DS18B20 parasitic power issue                               | Add external 3.3V supply to sensor VCC pin                                                  |
| **Relay not clicking**              | GPIO pin damaged or relay module not powered                | Verify relay VCC/GND, check GPIO voltage with multimeter                                    |
| **ESP32 keeps rebooting**           | Power supply insufficient or watchdog timeout               | Use >1A power supply, check serial log for panic messages                                   |
| **WiFi scan finds nothing**         | ESP32 antenna issue or interference                         | Move device closer to router, check antenna connection                                      |
| **mDNS not resolving**              | Network doesn't support mDNS                                | Use IP address directly, install mDNS reflector                                             |
| **Sensor readings fluctuating**     | Loose connection or interference                            | Check wiring, add shielded cable, verify pull-up resistor                                   |
| **Configuration lost after reboot** | NVS corruption or config not saved                          | Re-save configuration, try factory reset if persistent                                      |

---

## Detailed Diagnosis

### 1. Temperature Sensor Issues

#### Symptom: Sensor reads -127 °C

This is the DallasTemperature library default when no sensor is found on the
OneWire bus.

**Checklist:**

1. Verify the 4.7 kΩ pull-up resistor between DATA and 3.3V
2. Check wiring: VCC (red) → 3.3V, GND (black) → GND, DATA (yellow/white) → GPIO
3. Swap the sensor to a known-working GPIO pin
4. Test with a known-working DS18B20
5. Measure voltage on DATA pin: should be ~3.3V idle, pulses during communication

#### Symptom: Sensor reads 85 °C

This indicates the DS18B20 is in **parasitic power mode** but not getting
enough power.

**Solution:** Connect VCC pin (red) to 3.3V instead of relying on parasitic
power.

---

### 2. MQTT / Home Assistant Issues

#### Symptom: Home Assistant doesn't discover the controller

**Checklist:**

1. Verify MQTT broker is running: `mosquitto_sub -h <broker> -t "#" -v`
2. Check controller MQTT settings via web UI (MQTT Settings tab)
3. Test with MQTT Explorer:
   - Subscribe to `pool-controller/#`
   - Look for discovery topics: `homeassistant/sensor/pool-controller-*/config`
4. Restart the MQTT broker
5. Reboot the controller

#### Symptom: Entities show as "unavailable"

1. Check MQTT connection status in controller web UI
2. Verify the broker is reachable from both controller and HA
3. Restart HA MQTT integration

---

### 3. Relay Issues

#### Symptom: Relay clicks but pump doesn't run

1. Check COM/NO wiring — pump should be on the NO (normally open) terminal
2. Verify mains voltage at relay input
3. Check pump connection and fuse

#### Symptom: Relay doesn't click

1. Measure GPIO voltage — should toggle between HIGH and LOW
2. Verify relay module VCC (5V) and GND connections
3. Test relay module independently with 5V between IN and GND

---

### 4. Boot and Stability Issues

#### Symptom: Safe Mode activated

The controller enters Safe Mode after 4 consecutive boots that last less than
5 minutes each. All relays are forced OFF.

**Recovery:**

1. Connect serial monitor: `pio device monitor --baud 115200`
2. Read the boot log — look for panic messages, asserts, or exceptions
3. Common causes:
   - Stack overflow in a task
   - NULL pointer dereference
   - Heap exhaustion
   - Corrupt configuration
4. Fix the issue and reboot
5. The boot counter resets after a successful 5+ minute run

#### Symptom: Random reboots every few hours

1. Check `free_heap_space` sensor — should be above 20 KB
2. Verify power supply (minimum 1A)
3. Check for WiFi disconnection/reconnection loops
4. Monitor serial output for crash messages

---

### 5. OTA Update Issues

#### Symptom: OTA fails with TLS error

1. The web interface uses HTTPS for OTA — TLS handshake may fail on some
   networks
2. Ensure the ESP32 has enough free heap (check dashboard)
3. Try a smaller firmware binary

**Workaround:** Flash via USB:

```bash
pio run --target upload --upload-port /dev/ttyUSB0
```

#### Symptom: OTA uploads but device doesn't boot

1. The new firmware may be corrupted or incompatible
2. Flash a known-good version via USB
3. Check partition table size

---

## Diagnostic Tools

### Serial Monitor

```bash
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

Useful for: boot logs, crash dumps, MQTT connection status.

### MQTT Explorer

Install [MQTT Explorer](http://mqtt-explorer.com/) or use CLI:

```bash
mosquitto_sub -h <broker> -t "pool-controller/#" -v
```

Useful for: verifying MQTT Discovery payloads, checking telemetry data.

### Web UI Dashboard

Access via `http://<device-ip>` — shows live status, temperatures, heap, RSSI.

### Network Scan

```bash
# Find the controller on your network
nmap -sn 192.168.1.0/24
# Or use your router's DHCP client list
```

---

## Getting Help

If the troubleshooting steps above don't resolve your issue:

1. Search the [existing issues](https://github.com/smart-swimmingpool/pool-controller/issues)
2. Check the [discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)
3. Include the following information in your report:
   - Firmware version (from web UI or serial log)
   - Hardware variant (Basic, NORVI AE01-R, Custom)
   - ESP32 board type
   - MQTT broker and version
   - Home Assistant version (if applicable)
   - Serial monitor output (if available)

---

## Related Documents

- [Build from Zero](/docs/build-from-zero/) — Complete build guide
- [MQTT Configuration](/docs/mqtt-configuration/) — MQTT setup details
- [Electrical Safety](/docs/electrical-safety/) — Safety information
- [Home Assistant Integration](/docs/home-assistant/) — HA setup guide
