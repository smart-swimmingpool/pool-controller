---
name: config-nvs
description: "NVS/Preferences configuration storage for the pool-controller — ConfigManager API, key naming, NVS vs LittleFS trade-offs, OTA-safe persistence, and migration. 🇩🇪 Deutsche Trigger: Konfiguration, NVS, Preferences, ConfigManager, Konfiguration speichern, Einstellungen persistieren, OTA-sicher, Factory Reset, Werksreset."
keywords:
  - configuration
  - konfiguration
  - nvs
  - preferences
  - configmanager
  - konfiguration speichern
  - einstellungen
  - settings
  - persistenz
  - persistence
  - ota-sicher
  - ota safe
  - factory reset
  - werksreset
  - wifi credentials
  - mqtt config
  - ntp config
---

# NVS Configuration Storage — Pool Controller

The pool-controller stores all configuration data in ESP32 **NVS (Non-Volatile Storage)** via the Arduino `Preferences` library. This replaces the earlier LittleFS JSON-based `/config.json` approach.

## Why NVS over LittleFS/JSON?

| Feature | NVS / Preferences | LittleFS / JSON |
|---------|------------------|----------------|
| **OTA survival** | ✅ Native — separate flash partition | ⚠️ Needs manual backup/restore |
| **Wear leveling** | ✅ Built into ESP-IDF NVS driver | ⚠️ File-based, no wear leveling |
| **Atomic writes** | ✅ Power-fail safe | ❌ Can corrupt on power loss |
| **Boot speed** | ✅ Instant — no mount/parse | ❌ Needs FS mount + JSON deserialize |
| **API** | `putString(key, val)` / `getString(key, default)` | Open file → parse JSON → access field |
| **Flash overhead** | ~0 extra (uses existing partition) | Requires LittleFS partition |

## Architecture

```
ConfigManager (static class)
│
├── begin()   → opens NVS namespace "config", calls load()
├── load()    → reads all keys from NVS into memory structs
├── save()    → writes all memory structs to NVS atomically
└── reset()   → clears entire NVS namespace, resets to defaults
│
├── getWiFi()      → WiFiConfig&   { ssid, password }
├── getMqtt()      → MqttConfig&   { host, port, username, password, useTls }
├── getNtp()       → NtpConfig&    { server, timezone }
├── getSettings()  → ControllerSettings& { loopInterval, tempMaxPool, … }
│
├── setAdminPassword() / verifyAdminPassword()
└── isConfigured() / setConfigured()
```

> **⚠️ Save pattern**: After modifying any field via the getters above, call `ConfigManager::save()` explicitly:
> ```cpp
> ConfigManager::getSettings().opMode = "timer";
> ConfigManager::save();
> ```

## NVS Key Map

All config values live in a single NVS namespace `"config"`:

| Key | Type | Struct Field | Default |
|-----|------|-------------|---------|
| `wifi_ssid` | String | `WiFiConfig::ssid` | `""` |
| `wifi_pass` | String | `WiFiConfig::password` | `""` |
| `mqtt_host` | String | `MqttConfig::host` | `""` |
| `mqtt_port` | uint32_t | `MqttConfig::port` | `1883` |
| `mqtt_user` | String | `MqttConfig::username` | `""` |
| `mqtt_pass` | String | `MqttConfig::password` | `""` |
| `mqtt_tls` | bool | `MqttConfig::useTls` | `false` |
| `ntp_server` | String | `NtpConfig::server` | `"pool.ntp.org"` |
| `ntp_tz` | int32_t | `NtpConfig::timezone` | `0` |
| `set_interval` | int32_t | `ControllerSettings::loopInterval` | `10` |
| `set_maxpool` | double | `ControllerSettings::tempMaxPool` | `28.5` |
| `set_minsolar` | double | `ControllerSettings::tempMinSolar` | `55.0` |
| `set_hyst` | double | `ControllerSettings::tempHysteresis` | `1.0` |
| `set_opmode` | String | `ControllerSettings::opMode` | `"auto"` |
| `set_tzidx` | int16_t | `ControllerSettings::timezoneIndex` | `0` |
| `set_green` | uint8_t | `ControllerSettings::timeLossGreenHours` | `1` |
| `set_red` | uint8_t | `ControllerSettings::timeLossRedHours` | `24` |
| `adm_pass` | String | `adminPasswordHash_` | SHA256 of `"admin"` |
| `cfg_configured` | bool | `configured_` | `false` |

> **🔍 Code Search:** the exact key names are defined as `constexpr` at the top of `src/ConfigManager.cpp`.

## OTA Safety

NVS data **survives all firmware update methods** automatically:

- **OTA (Over-the-Air)**: ✅ NVS partition is never touched by OTA
- **USB serial flash**: ✅ Safe as long as *Erase All Flash Before Sketch Upload* is **Disabled** (default in PlatformIO)
- **PlatformIO `pio run --target erase`**: ❌ Will wipe NVS — use with caution

The old OTA backup/restore logic (`backupConfig()` / `restoreConfig()`) and the `/config.json.ota` file have been **removed**. They are no longer needed.

## Boot Version Tracking

`ConfigManager::logOtaTransition()` uses a separate NVS namespace `"ota-version"` with a single key `"fw_version"` to detect version transitions across OTA updates. It logs:

- **First boot ever** — no previous version found
- **OTA update detected** — version changed (logs old → new)
- **Normal boot** — same version as before

```cpp
// Called once in PoolControllerContext::setup() after ConfigManager::begin()
ConfigManager::logOtaTransition();
```

## Factory Reset

`ConfigManager::reset()` clears the entire `"config"` NVS namespace and restores factory defaults:

```cpp
ConfigManager::reset();
ConfigManager::save();  // persist the cleared state
```

The `WebPortal::apiFactoryReset()` calls this and reboots the ESP32.

## Adding a New Config Field

1. Add the field to the appropriate struct in `ConfigManager.hpp`
2. Add a `constexpr` key name at the top of `ConfigManager.cpp`
3. Add a `prefs.getTYPE(key, default)` call in `load()`
4. Add a `prefs.putTYPE(key, value)` call in `save()`
5. Add a default initialization in the struct definition (for `reset()`)

Example — adding a new `heaterTimeout` setting:

```cpp
// In ConfigManager.hpp:
struct ControllerSettings {
  // ... existing fields ...
  int heaterTimeout = 30;  // minutes
};

// In ConfigManager.cpp — key:
static constexpr const char *kSetHeaterTimeout = "set_heat_to";

// In ConfigManager::load():
settings_.heaterTimeout = prefs.getInt(kSetHeaterTimeout, 30);

// In ConfigManager::save():
prefs.putInt(kSetHeaterTimeout, settings_.heaterTimeout);
```

## Related Files

| File | Role |
|------|------|
| `src/ConfigManager.hpp` | Struct definitions + public API |
| `src/ConfigManager.cpp` | NVS read/write + key map |
| `src/PoolController.cpp` | Calls `ConfigManager::begin()` + `logOtaTransition()` |
| `src/WebPortal.cpp` | REST API reads/saves via ConfigManager |
| `src/MqttPublisher.cpp` | MQTT handlers apply config changes via ConfigManager |
| `src/NetworkManager.cpp` | Reads WiFi + MQTT config on reconnect |
| `src/WpsProvisioner.cpp` | Writes WiFi credentials via ConfigManager |
