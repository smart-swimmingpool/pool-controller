---
title: Security Checklist
summary: Production security hardening checklist for the Pool Controller — default password change, OTA security, MQTT configuration, network isolation, and firmware integrity
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
tags: ["docs", "security", "checklist", "hardening"]
menu:
  docs:
    parent: Pool Controller
    name: Security Checklist
    weight: 160
---

> ⚠️ **WARNING**: Some security measures reference **230V AC mains voltage**
> (relay testing, power supply verification). **Always disconnect power**
> before working on the circuit. If unsure, hire a qualified electrician.

## Overview

This checklist covers security hardening measures for the Pool Controller.
While the controller runs on a local network and operates pumps (not life-critical
systems), following these recommendations prevents unauthorized access and
reduces attack surface.

---

## Critical (Must Do)

### 1. Change Default Password

The factory-default web interface password **must** be changed before
connecting the controller to any network.

- **Default**: `admin`
- **Action**: Web UI → Security & Update → Change Password
- **Requirements**:
  - Minimum 8 characters
  - Mix of uppercase, lowercase, digits, and special characters
  - Not a reused password from other services

### 2. Change OTA Update Password

The OTA update password is separate from the web UI password.

- **Default**: Same as web UI password
- **Action**: Web UI → Security & Update → OTA Password
- **Why**: Prevents unauthorized firmware uploads

### 3. Use MQTT Authentication

- **Action**: Configure MQTT username and password in controller settings
- **Why**: Unauthenticated MQTT allows anyone on the network to publish
  to controller topics
- **Recommended**: Create a dedicated MQTT user for the controller with
  minimal permissions

### 4. Keep MQTT Broker Internal

- **Action**: Bind MQTT broker to local network interface only
- **Why**: A publicly accessible MQTT broker can be discovered and abused
- **Mosquitto configuration**:

  ```ini
  listener 1883 192.168.1.0/24
  allow_anonymous false
  password_file /etc/mosquitto/passwd
  ```

- **Do NOT** expose port 1883 or 8883 to the internet

---

## High (Strongly Recommended)

### 5. Web Interface Only in LAN

- The web interface listens on all interfaces by default
- **Action**: Ensure the controller is on a trusted local network
- **Do NOT** expose port 80/443 of the controller to the internet
- If remote access is needed, use a VPN (WireGuard, Tailscale, OpenVPN)
  or Home Assistant's secure remote access

### 6. Use Official Firmware Only

- **Action**: Only flash firmware from official GitHub Releases
- **Why**: Unofficial firmware could contain backdoors or unsafe modifications
- **Verification**:
  - Download from [official releases](https://github.com/smart-swimmingpool/pool-controller/releases)
  - Check the release tag matches the expected version
  - Build from source yourself if you want full control

### 7. Backup Before OTA Update

- **Action**: Save a screenshot or export of current configuration before
  updating firmware
- **Why**: OTA failures can corrupt the filesystem and require re-flashing
- **Note**: The controller's configuration is stored in NVS and survives
  re-flashing in most cases, but a backup is recommended

---

## Medium (Recommended)

### 8. Test Relays Before Connecting Load

- **Action**: Verify relay operation with a multimeter before connecting
  230V AC pumps
- **Why**: A malfunctioning relay could leave a pump permanently on,
  creating a safety hazard
- **Procedure**: See [Build from Zero → Relay Test](docs/build-from-zero/#step-7-relay-test-no-load)

### 9. Use a Dedicated Network VLAN

For advanced setups:

- Place the ESP32 on an **IoT VLAN** with restricted internet access
- Allow only: MQTT broker, NTP server, Home Assistant
- Block all inbound connections from the internet

### 10. Disable Unused Features

- **Action**: If you don't use OTA, disable it in configuration
- **Action**: If you don't need the web interface, restrict access via
  network firewall rules
- **Why**: Reducing attack surface reduces risk

---

## Network Security Overview

```text
Internet
    │
    │ (blocked)
    ▼
┌──────────────┐     ┌────────────┐     ┌──────────────────┐
│ Home Router  │─────│ Local LAN  │─────│ Pool Controller  │
│ (Firewall)   │     │ 192.168.x.x│     │ ESP32            │
└──────────────┘     └────────────┘     └──────────────────┘
                            │                    │
                            ▼                    ▼
                      ┌──────────────┐     ┌──────────────┐
                      │ MQTT Broker  │     │ Home Assistant│
                      │ (local only) │     │ (local only)  │
                      └──────────────┘     └──────────────┘
```

### Recommended Firewall Rules

| Source     | Destination | Port | Protocol | Action | Purpose              |
| ---------- | ----------- | ---- | -------- | ------ | -------------------- |
| Controller | MQTT Broker | 1883 | TCP      | Allow  | MQTT communication   |
| Controller | NTP Server  | 123  | UDP      | Allow  | Time sync            |
| Controller | Internet    | 80   | TCP      | Block  | No web access needed |
| Controller | Internet    | 443  | TCP      | Block  | No web access needed |
| Internet   | Controller  | any  | any      | Block  | No inbound access    |
| HA Server  | Controller  | 80   | TCP      | Allow  | Web UI / API         |

---

## Credential Management

### Where Credentials Are Stored

| Credential      | Storage | Encrypted?                                 |
| --------------- | ------- | ------------------------------------------ |
| WiFi password   | NVS     | No (ESP32 NVS is not encrypted by default) |
| MQTT password   | NVS     | No                                         |
| Web UI password | NVS     | SHA-256 hashed                             |
| OTA password    | NVS     | SHA-256 hashed                             |

> **Note**: NVS values are stored in plain text on the flash chip.
> Physical access to the device compromises all credentials.
> Enable **Flash Encryption** for production deployments.

### Flash Encryption (ESP32)

For advanced security, enable ESP32 flash encryption:

1. Enable in menuconfig: `Security features → Enable flash encryption`
2. Burn efuse to permanently enable encryption
3. Re-flash firmware
4. All NVS data will be encrypted at rest

> ⚠️ **Warning**: Flash encryption is irreversible on most ESP32 modules.
> Once enabled, you cannot read the flash without the encryption key.

---

## Related Documents

- [Build from Zero](/docs/build-from-zero/) — Complete build guide
- [Electrical Safety](/docs/electrical-safety/) — Electrical safety
- [Safety Model](/docs/safety-model/) — System safety architecture
- [Production Checklist](/docs/production-checklist/) — Pre-deployment checks
- [Troubleshooting](/docs/troubleshooting/) — Common issues and fixes
