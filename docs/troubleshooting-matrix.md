---
title: Troubleshooting Matrix
summary: Quick-reference troubleshooting matrix for the Pool Controller — common symptoms, root causes, and solutions in a compact table format
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
tags: ["docs", "troubleshooting", "quick-reference", "matrix"]
menu:
  docs:
    parent: Pool Controller
    name: Troubleshooting Matrix
    weight: 121
---

> ⚠️ **WARNING**: Some troubleshooting steps involve working with
> **230V AC mains voltage** (relay testing, pump wiring). **Always disconnect
> power** before working on the circuit. If unsure, hire a qualified electrician.

## Troubleshooting Matrix

| #   | Symptom                                      | Cause                                                                           | Solution                                                                                                      |
| --- | -------------------------------------------- | ------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| 1   | **-127 °C** temperature reading              | DS18B20 not detected on OneWire bus                                             | Check 4.7 kΩ pull-up resistor between DATA and 3.3V; verify wiring (VCC, GND, DATA); check solder joints      |
| 2   | **85 °C** temperature reading                | DS18B20 in parasitic power mode without sufficient power                        | Connect VCC pin to 3.3V (external power, not parasitic)                                                       |
| 3   | HA finds no device                           | MQTT Discovery payload not published or broker unreachable                      | Verify MQTT broker settings (host, port, credentials) in the web UI; check broker connectivity with MQTT Explorer |
| 4   | HA entities show "unavailable"               | MQTT connection lost or broker restarted                                        | Check controller MQTT status in web UI; restart HA MQTT integration; reboot controller                        |
| 5   | Relay behavior inverted                      | Active-low relay module but firmware uses active-low logic (hard-coded)         | The firmware drives relays with active-low logic (`LOW` = on). If your module expects active-high, swap the relay module or add an external transistor inverter |
| 6   | Relay clicks but pump doesn't run            | Incorrect COM/NO wiring or pump fuse blown                                      | Verify COM → mains Live, NO → pump Live; check pump fuse; test pump independently                             |
| 7   | Relay doesn't click                          | GPIO pin damaged, relay module not powered, or optocoupler defective            | Measure GPIO voltage (should toggle between ~0 V and ~3.3 V); verify relay VCC (5V) and GND; **disconnect the IN wire** from the ESP32 GPIO and apply 3.3 V (not 5 V) to the relay input pin to test the module |
| 8   | Safe Mode active                             | Boot-loop detected: 3 consecutive boots < 5 minutes each                        | Read serial log for crash reason (panic, assert, exception); fix the root cause; reboot                       |
| 9   | Random reboots every few hours               | Insufficient power supply or heap exhaustion                                    | Use >1A power supply; check `free_heap_space` (should be >20 KB); monitor serial for crash output             |
| 10  | OTA update fails                             | TLS handshake failure or insufficient flash space                               | Try USB flash as fallback; free space by removing unused data; try smaller firmware binary                    |
| 11  | OTA uploads but device won't boot            | Corrupted or incompatible firmware binary                                       | Flash known-good version via USB; verify firmware matches board type                                          |
| 12  | Web UI not loading                           | ESP32 not connected to network or wrong IP                                      | Check router DHCP list; verify WiFi connection status; try `pool-controller.local` (mDNS)                     |
| 13  | Web UI password not accepted                 | Password changed but browser has cached old session                             | Clear browser cookies and cache; use incognito/private browsing window                                        |
| 14  | MQTT connection refused                      | Wrong broker credentials or broker not reachable                                | Verify broker IP, port, username, and password in MQTT settings; check if broker allows anonymous connections |
| 15  | Temperature readings fluctuating             | Loose connection, interference, or long unshielded cable                        | Check sensor wiring; use shielded twisted-pair cable; verify pull-up resistor value (4.7 kΩ)                  |
| 16  | Configuration lost after reboot              | NVS corruption or configuration not saved properly                              | Re-save configuration via web UI; if persistent, try factory reset and reconfigure                            |
| 17  | ESP32 not entering AP mode                   | Previous WiFi configuration still stored                                        | Perform factory reset via Web UI → System → Factory Reset to clear credentials; AP mode starts automatically when no SSID is configured |
| 18  | WiFi scan finds no networks                  | ESP32 antenna issue, interference, or wrong region                              | Move device closer to router; check antenna connection; verify WiFi region settings                           |
| 19  | mDNS not resolving (`pool-controller.local`) | Network doesn't support mDNS or reflector needed                                | Use IP address directly; install mDNS reflector (avahi-daemon on Linux)                                       |
| 20  | Serial monitor shows gibberish               | Wrong baud rate or serial port                                                  | Use 115200 baud; verify correct serial port (`/dev/ttyUSB0`, `COM3`, etc.)                                    |
| 21  | DS18B20 works initially then fails           | Intermittent connection or cold solder joint                                    | Re-solder sensor connections; add strain relief on sensor cable; check for water ingress                      |
| 22  | Both pumps running at same time unexpectedly | Timer or mode conflict                                                          | Check operation mode (should be Auto, not Manual); verify timer start/end times don't overlap                 |
| 23  | Pump runs when controller is off             | Relay welded/stuck closed or wiring bypasses relay                              | **DISCONNECT POWER IMMEDIATELY**; replace relay module; verify wiring                                         |
| 24  | Home Assistant shows wrong temperature unit  | HA temperature unit configuration mismatch                                      | Check HA → Settings → System → Units; restart HA                                                              |
| 25  | Firmware version shows "unknown"             | Version string not compiled into firmware                                       | Re-compile from clean build (`pio run --target clean && pio run`); verify `version.h` is correct              |

---

## By Category

### Sensors

| #   | Symptom              | Likely Cause       | Quick Fix            |
| --- | -------------------- | ------------------ | -------------------- |
| 1   | -127 °C              | No sensor detected | Check 4.7 kΩ pull-up |
| 2   | 85 °C                | Parasitic power    | Add VCC wire         |
| 15  | Fluctuating readings | Interference       | Shielded cable       |
| 21  | Intermittent failure | Bad connection     | Re-solder            |

### MQTT / Home Assistant

| #   | Symptom            | Likely Cause       | Quick Fix              |
| --- | ------------------ | ------------------ | ---------------------- |
| 3   | Not discovered     | Discovery not sent | Check broker settings  |
| 4   | Unavailable        | Connection lost    | Reboot controller      |
| 14  | Connection refused | Wrong credentials  | Verify MQTT settings   |

### Relays

| #   | Symptom            | Likely Cause      | Quick Fix             |
| --- | ------------------ | ----------------- | --------------------- |
| 5   | Inverted behavior  | Wrong polarity    | Use active-low module |
| 6   | Clicks but no pump | Wiring error      | Check COM/NO          |
| 7   | No click           | No power to relay | Check 5V supply       |
| 23  | Pump runs when off | Welded relay      | Replace relay module  |

### Boot / Stability

| #   | Symptom        | Likely Cause      | Quick Fix        |
| --- | -------------- | ----------------- | ---------------- |
| 8   | Safe Mode      | Boot-loop         | Check serial log |
| 9   | Random reboots | Low power or heap | Better PSU       |
| 16  | Config lost    | NVS corruption    | Factory reset    |

### OTA / Firmware

| #   | Symptom              | Likely Cause | Quick Fix          |
| --- | -------------------- | ------------ | ------------------ |
| 10  | OTA fails            | TLS/Space    | USB flash          |
| 11  | Won't boot after OTA | Bad firmware | Reflash known-good |
| 25  | Unknown version      | Build issue  | Clean rebuild      |

---

## Serial Log Quick Reference

Look for these keywords in the serial monitor output:

| Log Message                  | Meaning                                   |
| ---------------------------- | ----------------------------------------- |
| `Safe Mode ACTIVE`           | Boot-loop detected, relays forced OFF     |
| `WDT reset`                  | Hardware watchdog triggered (30s timeout) |
| `heap critical`              | Free heap below 8 KB, reboot imminent     |
| `DS18B20 read failed`        | Temperature sensor communication error    |
| `MQTT connection failed`     | Broker unreachable or credentials wrong   |
| `NTP sync failed`            | Time server not reachable                 |
| `Configuration CRC mismatch` | NVS config corrupted, using defaults      |
| `WiFi disconnected`          | Network connection lost                   |
| `OTA update failed`          | Firmware upload error                     |

---

## See Also

- [Hardware Guide](/docs/hardware-guide/) — Build and wiring instructions
- [Software Guide](/docs/software-guide/) — First-time setup and configuration
- [Security Checklist](/docs/security-checklist/) — Security hardening
