---
name: web-ui
description: "Web interface development for the pool-controller ESP32 — edit HTML/CSS/JS externally on LittleFS with PROGMEM fallback, REST API, uploadfs workflow, and dashboard architecture. Use when asked to modify the web UI, add dashboard features, change styling, work with REST API endpoints, or update web assets. 🇩🇪 Deutsche Trigger: Web-Interface, Dashboard, CSS, JavaScript, REST-API, LittleFS, uploadfs, Web-Assets, UI-Änderungen, Style, Theme, Frontend."
keywords:
  - web interface
  - dashboard
  - css
  - javascript
  - rest api
  - littlefs
  - uploadfs
  - web assets
  - ui development
  - frontend
  - html
  - glassmorphism
  - ota web update
  - captive portal
  - web-ui
  - theme
  - dark mode
  - pool dashboard
  - telemetry
  - hot reload
---

# Web UI — Pool Controller

Web interface development for the ESP32 pool-controller. The UI lives in two layers: **LittleFS** (editable at runtime) and **PROGMEM** (embedded fallback in the firmware binary).

> **🔍 Code Search**: Use `semble search "LittleFS.open"` or `semble search "kPortalPageHtml"` to find the fallback architecture. See `Agents.md` §7 for full `semble` usage.

---

## Quick Reference

```bash
# Upload web assets to device (no firmware rebuild!)
pio run --target uploadfs

# Build firmware with embedded PROGMEM fallback
pio run

# Build + flash firmware + uploadfs in one go
pio run --target upload && pio run --target uploadfs

# Monitor serial output
pio device monitor -b 115200
```

---

## Architecture

```
Browser ──── GET / ────► WebPortal::handleRoot()
                              │
                    ┌─────────┴─────────┐
                    ▼                    ▼
           LittleFS exist?         PROGMEM Fallback
           /web/index.html         kPortalPageHtml
                    │              + kPremiumStyles
           ┌────────┴───────┐
           ▼                 ▼
    External assets      Inline everything
    /style.css           <style> embedded
    /app.js              <script> embedded
```

### Two-Layer Design

| Layer        | Storage           | Editable at Runtime? | Used When                          |
| ------------ | ----------------- | -------------------- | ---------------------------------- |
| **LittleFS** | Flash (data/web/) | ✅ Yes — `uploadfs`  | First choice                       |
| **PROGMEM**  | Firmware binary   | ❌ Needs recompile   | Fallback if LittleFS files missing |

**Fallback principle**: If `/web/index.html` doesn't exist on LittleFS, the firmware serves the embedded PROGMEM version which contains everything inline (HTML + CSS + JS). This means the device works out of the box with just a firmware flash — no `uploadfs` step required.

---

## Web Asset Files

### `data/web/` Directory

```
data/
└── web/
    ├── index.html    ← Dashboard page (links to /style.css, /app.js)
    ├── style.css     ← Full glassmorphism theme (CSS variables)
    └── app.js        ← All dashboard logic (API calls, DOM updates)
```

### Source in C++ (PROGMEM Fallbacks in `WebPortal.cpp`)

| PROGMEM Symbol      | Content                                   | Line  |
| ------------------- | ----------------------------------------- | ----- |
| `kPremiumStyles[]`  | Complete CSS (~140 lines)                 | ~L28  |
| `kLoginPageHtml[]`  | Login page template (uses `%STYLES%`)     | ~L171 |
| `kPortalPageHtml[]` | Main dashboard template (uses `%STYLES%`) | ~L219 |

The `%STYLES%` placeholder in PROGMEM templates is replaced at runtime with `kPremiumStyles` — this ensures the fallback is always self-contained.

### PROGMEM vs LittleFS Content

The LittleFS `index.html` is functionally identical to `kPortalPageHtml` except:

| Aspect      | PROGMEM Fallback                                 | LittleFS Version                            |
| ----------- | ------------------------------------------------ | ------------------------------------------- |
| CSS         | `<style>%STYLES%</style>` (replaced server-side) | `<link rel="stylesheet" href="/style.css">` |
| JS          | `<script>…</script>` (inline)                    | `<script src="/app.js"></script>`           |
| Editability | ❌ Firmware recompile                            | ✅ Direct file edit + `uploadfs`            |

---

## Development Workflow

### 1. Edit the Assets

Edit files in `data/web/` with normal tooling:

```bash
# CSS — full glassmorphism theme with CSS variables
vim data/web/style.css

# JS — all dashboard logic (functions for telemetry, config, tabs)
vim data/web/app.js

# HTML — page structure
vim data/web/index.html
```

**No firmware rebuild needed** for UI changes. Only the C++ backend (REST handlers in `WebPortal.cpp`) requires a full `pio run`.

### 2. Upload to Device

```bash
# Upload only the web assets (fast — ~5 seconds)
pio run --target uploadfs

# Verify on device: refresh browser
```

### 3. Hot-Reload Cycle

```
1. Edit data/web/app.js        ← in your editor
2. pio run --target uploadfs   ← ~5 seconds
3. Ctrl+R in browser           ← changes live
```

For CSS changes, this is nearly instant. No button-pressing, no serial flashing.

### 4. Production Build (Full Firmware + Assets)

```bash
# Full rebuild with embedded PROGMEM fallback
pio run

# Flash everything
pio run --target upload && pio run --target uploadfs
```

---

## REST API Reference

The web frontend communicates with the ESP32 backend exclusively through these REST endpoints:

| Method | Endpoint             | Auth   | Purpose         | Frontend Call                                                            |
| ------ | -------------------- | ------ | --------------- | ------------------------------------------------------------------------ |
| `GET`  | `/api/status`        | ❌ No  | Live telemetry  | `loadTelemetry()` — every 2s                                             |
| `GET`  | `/api/scan`          | ✅ Yes | WiFi scan       | `scanNetworks()`                                                         |
| `GET`  | `/api/config`        | ✅ Yes | Read all config | `loadConfig()` — on page load                                            |
| `POST` | `/api/config`        | ✅ Yes | Save config     | `saveWiFi()`, `saveMqtt()`, `saveControllerSettings()`, `savePassword()` |
| `POST` | `/api/login`         | -      | Get session     | Login form submit                                                        |
| `GET`  | `/api/logout`        | -      | Clear session   | (not used in UI)                                                         |
| `GET`  | `/api/restart`       | ✅ Yes | Reboot device   | `restartDevice()`                                                        |
| `GET`  | `/api/factory_reset` | ✅ Yes | Wipe + reboot   | `factoryReset()`                                                         |
| `POST` | `/api/update`        | ✅ Yes | OTA firmware    | OTA form submit                                                          |

### Config POST Types

The `/api/config` endpoint uses a `type` parameter:

| Type       | Parameters                                                                            | Persisted               |
| ---------- | ------------------------------------------------------------------------------------- | ----------------------- |
| `wifi`     | `ssid`, `password`                                                                    | LittleFS `/config.json` |
| `mqtt`     | `host`, `port`, `username`, `password`, `tls`                                         | LittleFS `/config.json` |
| `settings` | `mode`, `interval`, `max_pool`, `min_solar`, `hysteresis`, `timezone`, `green`, `red` | LittleFS + NVS          |
| `password` | `password`                                                                            | LittleFS `/config.json` |

### `/api/status` Response

```json
{
  "pool_temp": 27.3,
  "solar_temp": 62.1,
  "ctrl_temp": 35.2,
  "pool_pump": true,
  "solar_pump": true,
  "op_mode": "auto",
  "uptime": 86400,
  "free_heap": 28456,
  "max_alloc": 12288,
  "rssi": -45,
  "wifi_connected": true,
  "mqtt_connected": true,
  "local_ip": "192.168.1.100"
}
```

---

## Adding a New UI Feature

Example: Adding a new "System Info" tab.

### 1. HTML — Add the tab button and content panel

In `data/web/index.html`:

```html
<!-- In the .nav-tabs div: -->
<div class="tab" onclick="switchTab('system-info')">System Info</div>

<!-- New tab content: -->
<div id="tab-system-info" class="tab-content glass-card" style="display: none;">
  <h2>System Information</h2>
  <div id="sysInfoContent">Loading...</div>
</div>
```

### 2. JS — Add the data-loading function

In `data/web/app.js`:

```javascript
async function loadSystemInfo() {
  const res = await fetch("/api/status");
  const data = await res.json();
  document.getElementById("sysInfoContent").innerHTML =
    "Uptime: " + data.uptime + "s<br>" + "Free heap: " + data.free_heap + " B";
}
```

### 3. Upload & verify

```bash
pio run --target uploadfs
# → refresh browser
```

> **Note**: If the new tab needs data not provided by existing API endpoints, you must also add a new endpoint in `WebPortal::setupRoutes()` — this requires a full firmware rebuild.

---

## Adding a New API Endpoint

To add a new backend endpoint (requires firmware rebuild):

### 1. C++ Handler (`WebPortal.cpp`)

```cpp
void WebPortal::apiGetSystemInfo() {
  JsonDocument doc;
  doc["chip_model"] = ESP.getChipModel();
  doc["flash_size"] = ESP.getFlashChipSize();
  doc["sketch_size"] = ESP.getSketchSize();
  doc["free_sketch"] = ESP.getFreeSketchSpace();

  String json;
  serializeJson(doc, json);
  server_.send(200, "application/json", json);
}
```

### 2. Declaration (`WebPortal.hpp`)

```cpp
static void apiGetSystemInfo();
```

### 3. Route (`WebPortal::setupRoutes()`)

```cpp
server_.on("/api/system_info", HTTP_GET, []() {
  if (!handleAuthentication()) return;
  apiGetSystemInfo();
});
```

### 4. Build & flash

```bash
pio run --target upload        # Firmware update
pio run --target uploadfs      # Web assets
```

---

## Production Deployment

### Bundled Firmware

The PROGMEM fallbacks mean the firmware binary is **self-contained**. Users only need:

```bash
pio run --target upload
```

No `uploadfs` required — the dashboard works instantly from the embedded PROGMEM strings.

### Full Deployment (with external assets)

For users who want the fastest UI (LittleFS files are served directly without runtime string replacement):

```bash
pio run --target upload && pio run --target uploadfs
```

### OTA Update

```bash
pio run --target upload --upload-port <esp-ip> --upload-protocol espota
```

The OTA update only replaces the firmware. If web assets on LittleFS change, a separate `uploadfs` is needed. Consider building a combined OTA image that includes both.

---

## Troubleshooting

### Symptom: Browser shows blank page, no errors

**Cause**: LittleFS `/web/index.html` exists but is corrupted.

**Fix**: Either upload a valid `data/web/` or — if you can't reach the device at all — re-flash the firmware. The PROGMEM fallback is only used when the LittleFS file is **absent**, not when it's corrupted. To force fallback mode, remove the file:

```cpp
// Temporary: in WebPortal::handleRoot(), force fallback
LittleFS.remove("/web/index.html");
```

### Symptom: CSS works in PROGMEM mode but not via LittleFS

**Cause**: The LittleFS `index.html` links to `/style.css` but the file doesn't exist on LittleFS.

**Fix**: Ensure all three files are uploaded:

```bash
pio run --target uploadfs
# Verifies: data/web/index.html, data/web/style.css, data/web/app.js
```

### Symptom: "404 Not Found" for `/app.js`

**Cause**: The LittleFS `index.html` expects `/app.js` but the LittleFS file is missing.

**Fix**: Re-run `uploadfs`. In PROGMEM fallback mode this endpoint is never called (JS is inline).

### Symptom: Changes not showing after uploadfs

**Cause**: Browser caching. Do a hard refresh:

- **Chrome/Edge**: `Ctrl+Shift+R`
- **Firefox**: `Ctrl+Shift+R`
- **Safari**: `Cmd+Option+R`

### Symptom: `uploadfs` command doesn't exist

**Cause**: The `data/` directory wasn't configured in `platformio.ini`.

**Fix**: Ensure your `platformio.ini` has a `board_build.filesystem` directive or that the uploadfs target is available for your board (ESP32 LittleFS is supported by default in PlatformIO).

---

## File Reference

| Path                    | Purpose                                                                   | Edit Frequency |
| ----------------------- | ------------------------------------------------------------------------- | -------------- |
| `data/web/index.html`   | Dashboard HTML structure                                                  | Low            |
| `data/web/style.css`    | Glassmorphism theme (CSS variables)                                       | Medium         |
| `data/web/app.js`       | All dashboard JavaScript logic                                            | High           |
| `src/WebPortal.hpp`     | C++ handler declarations                                                  | Low            |
| `src/WebPortal.cpp`     | C++ handler implementations, PROGMEM fallbacks, route setup               | Medium         |
| `src/ConfigManager.hpp` | Config data structures (`WiFiConfig`, `MqttConfig`, `ControllerSettings`) | Low            |
| `src/ConfigManager.cpp` | Config persistence to LittleFS `/config.json`                             | Low            |
| `src/MqttPublisher.cpp` | MQTT state publishing (notifies HA of changes)                            | Low            |
