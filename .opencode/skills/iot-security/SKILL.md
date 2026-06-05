---
name: iot-security
description: "IoT security checks for the ESP32 pool-controller — Secure Boot, Flash Encryption, TLS, WPS, secrets management, network security, and 24/7 operational security. Use when asked to audit security, fix vulnerabilities, implement secure communication, or harden the device firmware. 🇩🇪 Deutsche Trigger: IoT Sicherheit, Secure Boot, Flash Verschlüsselung, TLS, WPS, Credentials, Passwörter, Netzwerksicherheit, Härtung, Verwundbarkeiten."
keywords:
  - iot sicherheit
  - iot security
  - secure boot
  - flash verschlüsselung
  - flash encryption
  - tls
  - wps
  - credentials
  - passwörter
  - passwords
  - netzwerksicherheit
  - network security
  - härten
  - hardening
  - verwundbarkeit
  - vulnerability
  - eFuse
  - jtag
---

# IoT Security — Pool Controller

Security audit and hardening for the ESP32 pool-controller. This device controls 230V pool pumps — security failures can cause physical damage or create network entry points.

> **🔍 Code Search**: Use `semble search "setInsecure"` or `semble search "password"` to find security-sensitive code. `semble find-related` helps trace credential flow across components. See `Agents.md` §7 for full `semble` usage.

## Threat Model

| Threat                       | Impact                          | Likelihood | Mitigation                           |
| ---------------------------- | ------------------------------- | ---------- | ------------------------------------ |
| Unauthorized MQTT control    | Pump manipulation               | Medium     | Admin password, network segmentation |
| Firmware reverse engineering | IP theft, credential extraction | Low        | Flash Encryption                     |
| Unauthorized OTA update      | Malicious firmware              | Low        | Secure Boot + signed OTA             |
| WiFi credential theft        | Network access                  | Medium     | Encrypted storage, no plaintext logs |
| Physical device access       | Full compromise                 | Low        | JTAG disable, debug UART off         |
| Replay attack on MQTT        | State manipulation              | Low        | TLS + unique client ID               |
| Boot-loop attack             | Denial of service               | Very Low   | Safe mode after 3 boots              |

## 1. Flash Encryption & Secure Boot

Referenced in `Agents.md` §11 — not yet enabled in `platformio.ini`.

**Secure Boot** ensures only signed firmware runs:

```ini
; platformio.ini — add to [env:esp32dev]
board_build.secure = secure
board_build.secure_sign_key = secure_boot_signing_key.pem
```

**Flash Encryption** protects stored data (WiFi creds, MQTT passwords):

```ini
board_build.flash_encrypt = true
```

⚠ **One-time operation**: burning eFuses is irreversible. Generate keys offline:

```bash
# Generate Secure Boot key
espsecure.py generate_signing_key secure_boot_signing_key.pem

# Generate Flash Encryption key
espefuse.py --port /dev/ttyUSB0 burn_key BLOCK_KEY0 flash_encryption_key.bin
```

**Current status**: Both are future work. The device currently stores WiFi passwords in plaintext on LittleFS (`/config.json`, gitignored in `.gitignore:114`).

## 2. TLS for MQTT

**Location**: `NetworkManager.cpp:126-130`

Currently uses `setInsecure()` which skips certificate validation:

```cpp
secureClient_->setInsecure(); // Allows local broker connection without CA verification
```

**Security gap**: No certificate pinning or CA verification. Mitigations:

- **For production**: Use a real CA or self-signed cert with fingerprint:

```cpp
// Instead of setInsecure():
secureClient_->setCACert(rootCACertificate);  // CA cert
// Or for self-signed:
secureClient_->setCertificate(clientCert);
secureClient_->setPrivateKey(clientKey);
```

- **Low-risk mitigation**: Keep on isolated VLAN with firewall rules
- **Minimal mitigation**: Ensure MQTT broker requires authentication even on LAN

## 3. Credential Storage

**Current practice** (`ConfigManager.cpp:80-91`):

- WiFi SSID/password stored in plaintext on LittleFS
- MQTT username/password stored in plaintext on LittleFS
- Admin password stored as SHA256 hash (no salt — see below)
- WPS credentials persisted to SPIFFS (`WpsProvisioner.cpp:126-127`)

**Improvements**:

- Use ESP32 NVS for credential storage with read-protection
- At minimum: `chmod`-equivalent filesystem permissions if supported
- Add salt to password hashing (`Agents.md` §11 recommends against secrets in repo)

**Password hashing issue** (`ConfigManager.cpp:17`):

```cpp
// Hardcoded default hash for "admin" — no salt!
static constexpr const char* kDefaultPasswordHash = "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";
```

The default password hash is for "admin" (SHA256). On first login, user should change it. Consider:

- Forcing password change on first login
- Using `mbedtls_md_hmac` with a per-device salt

## 4. Web Portal Security

**Location**: `WebPortal.hpp`

- **Session timeout**: 15 minutes (`kSessionTimeoutMs = 15 * 60 * 1000`)
- **Session token**: Generated server-side
- **Password verification**: SHA256 hash comparison (`ConfigManager.cpp:182-183`)
- **AP mode**: Open WiFi named "Pool-Controller-Setup" (`NetworkManager.cpp:101`)

**Risks**:

- AP mode has no encryption — anyone within range can connect
- Session timeout is reasonable, but no rate-limiting on login attempts
- No HTTPS on the web portal (ESP32 can support it with server cert)

**Audit check**: Verify `WebPortal.cpp` doesn't expose credentials in HTML/JS responses.

## 5. WPS Provisioning Security

**Location**: `WpsProvisioner.cpp`

- Triggered by holding GPIO0 for 2 seconds
- WPS PIN mode: disabled (only PBC — Push Button Config)
- Session timeout: 120 seconds
- Credentials persisted to SPIFFS after successful WPS

**Risks**:

- WPS PBC is vulnerable to physical attack (someone with 2s GPIO0 access can extract WiFi credentials)
- Credentials stored in SPIFFS in plaintext at `/homie/config.json`
- After WPS, the device reconnects with new credentials

## 6. OTA Update Security

**Location**: `platformio.ini:53-61`

OTA is configured but **commented out** by default:

```ini
;upload_protocol = espota
;upload_flags =
;  --auth=YOUR_OTA_PASSWORD
```

**When enabling OTA**:

- Always set a strong `--auth` password
- Consider using HTTPS OTA (`esp_https_ota`) instead of plain ESP-OTA
- Verify firmware signature before applying (Secure Boot)
- Use dual-partition OTA (factory + OTA) for rollback

## 7. Debug Interface Security

**From `Agents.md` §11**:

> Debug Interfaces (JTAG/UART) im Produktions-Build deaktivieren.

For ESP32:

```ini
; In platformio.ini production env:
build_flags =
  -D CONFIG_SECURE_DISABLE_OCD=1    ; Disable JTAG
  -D CONFIG_CONSOLE_UART_NONE=1     ; Disable serial console (or use custom UART)
```

**Current state**: Serial console is enabled at 115200 baud, which provides full system access. For production, consider disabling or restricting serial output.

## 8. Secure Coding Patterns for This Project

### Memory Safety

- No `new`/`delete` in hot-paths (existing rule in `Agents.md`)
- `unique_ptr` for rule ownership (`OperationModeNode.hpp:84`) ✓
- RAII for all resources (`PoolController.hpp:11-21`) ✓

### Input Validation

- WiFi SSID/MQTT config validated at entry points? Check `WebPortal.cpp` handlers
- MQTT message payload size: PubSubClient has internal buffer limits
- JSON deserialization uses `size_t` limits (`ConfigManager.cpp:69`)

### Rate Limiting

- No login rate-limiting on web portal
- MQTT reconnect backoff: 5s fixed interval — add jitter for production
- Sensor polling: already rate-limited by `TEMP_READ_INTERVAL`

## 9. Security Audit Checklist

- [ ] Secure Boot keys generated and stored offline
- [ ] Flash Encryption eFuses burned for production
- [ ] MQTT TLS uses CA verification, not `setInsecure()`
- [ ] WiFi credentials encrypted at rest (at minimum: restricted file permissions)
- [ ] Admin password uses salted hash (SHA256 + unique salt per device)
- [ ] Default password changed on first login
- [ ] OTA password set and strong
- [ ] JTAG/UART debug interfaces disabled in production build
- [ ] Web portal has rate-limiting on login
- [ ] No secrets in serial output logs
- [ ] .gitignore covers all credential files (currently: `data/homie/config.json` ✓)
- [ ] NetworkManager AP mode has at minimum a WPA2 password option
- [ ] MQTT LWT (Last Will) published on disconnect (`NetworkManager.cpp:147` ✓)
- [ ] Boot-loop detection prevents repeated crash-exposure of secrets (✓ P8)

## 10. Network Segmentation Recommendation

```
[Pool Controller] ──WiFi──┐
                           ▼
                    ┌──────────────┐
                    │  IoT VLAN    │
                    │  10.0.x.0/24 │
                    ├──────────────┤
                    │ MQTT Broker  │
                    │ (auth + TLS) │
                    └──────┬───────┘
                           │ firewall rule: only MQTT:8883
                           ▼
                    ┌──────────────┐
                    │  Main LAN    │
                    │  Home Ass.   │
                    └──────────────┘
```

The pool controller should be on an isolated IoT VLAN with:

- **Outbound**: MQTT broker only (port 8883 TLS)
- **Inbound**: Nothing (no SSH, no HTTP from main LAN)
- **DHCP**: Static lease for monitoring
