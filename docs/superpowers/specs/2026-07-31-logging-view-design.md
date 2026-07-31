# Design: Logging View for Pool Controller

**Date:** 2026-07-31
**Status:** Approved (brainstorming)
**Scope:** New "Logs" view in the web UI with RAM ring buffer log capture, REST endpoint, MQTT export, and UI console.

## Motivation

The pool controller currently logs exclusively to the serial interface (253
`Serial.printf/print/println` calls across 21 files). There is no way to inspect
logs from the web UI, which makes remote debugging difficult on a 24/7 device.

This feature adds a web-based logging view: the device captures log lines into a
RAM ring buffer, serves them via an unauthenticated read-only REST endpoint, and
optionally exports warnings/errors to MQTT (Home Assistant). The UI shows the log
as a console with level filters, auto-refresh, and auto-scroll.

## Requirements (from brainstorming)

1. **Content:** Full system log + filtered event view (level/category filters).
2. **Persistence:** RAM ring buffer only (no flash wear). Optional MQTT export for
   warnings/errors + curated events so logs survive reboot in Home Assistant.
3. **Placement:** "📜 Logs" button on the dashboard plus an entry in the More
   bottom-sheet menu (not a bottom tab). The dashboard button is the unauthenticated
   access path — the tab bar and More menu are hidden entirely without login.
4. **Access:** Read-only without login (like dashboard telemetry); clear operation
   requires login.
5. **Updates:** Auto-polling every 2 s while the tab is visible (same pattern as
   dashboard telemetry).

## Approach (chosen: A — central capture + migration)

Full migration of all `Serial.printf/print/println` calls to a central logging API
that continues to write to Serial (behavior unchanged) and additionally appends to
the ring buffer.

## Architecture

### 1. LogCapture module (`src/LogCapture.{hpp,cpp}`)

Replaces the current stub `src/Nodes/Logger.{hpp,cpp}`.

- **Static ring buffer**: fixed size (default 8 KB, configurable via build flag
  `-DLOG_BUFFER_SIZE=<bytes>`). No heap allocation per log line → no fragmentation.
  Estimated ~80–100 bytes/entry → ~80–100 lines in the buffer.
- **Entry format**: `{seq, level, uptime_ms, msg}`.
  - `seq`: monotonically increasing sequence number (uint32) for incremental polling.
  - `level`: enum `Debug, Info, Warning, Critical, Error` (reuse existing
    `Logger::LogLevel` enum names).
  - `uptime_ms`: milliseconds since boot (survives in the entry; absolute time is
    derived in the UI when NTP is synced).
  - `msg`: null-terminated formatted string.
- **API**:
  - `LogCapture::log(Level level, const char *fmt, ...)` — variadic snprintf into a
    static buffer, write to Serial AND append to ring buffer.
  - `LogCapture::begin()` — initialize (called from `setup()`).
  - `LogCapture::getEntries(uint32_t since, size_t maxCount, Level minLevel, ...)`
    — returns entries after `since`, capped, filtered by level (for REST handler).
  - `LogCapture::clear()` — reset ring buffer.
- **Thread safety**: short critical section (portMUX or mutex) — writes come from
  both the WebServer task and the main loop task.
- **Level filtering at capture time**: configurable; DEBUG included by default at
  build, standard capture from INFO up (configurable via build flag if needed).
- Remove the stub `Nodes/Logger` (or fold its enum/flags into `LogCapture`).

### 2. Log capture: migration of 276 Serial calls

- New macros `LOG_DEBUG/LOG_INFO/LOG_WARN/LOG_ERROR(...)` that expand to
  `LogCapture::log(Level::..., __VA_ARGS__)`.
- **Mechanical migration** via ast-grep:
  - `Serial.printf("...", ...)` → `LOG_INFO("...", ...)` (level inferred from
    message content: "WARNING"/"ERROR"/"CRITICAL" prefixes → respective level,
    else INFO).
  - Multi-part `Serial.print` chains: combine into a single `LOG_*` call where
    mechanical; remaining special cases (e.g. `Update.printError(Serial)`,
    library-internal prints) handled manually.
- **Invariant:** serial output stays byte-identical after migration. Verify by
  comparing serial output before/after on representative paths.

### 3. REST API (`WebPortal.cpp`)

- `GET /api/logs?level=INFO&count=200&since=<seq>` — **no auth** (read-only).
  - Response: `{"entries": [{"seq": 42, "level": "INFO", "uptime": 3610, "msg": "..."}], "next_seq": 43}`
  - `level` ∈ {DEBUG, INFO, WARN, ERROR} (server-side filter; WARN includes
    Warning+Critical+Error, ERROR includes Error).
  - `count` default 200, max 500.
- `POST /api/logs/clear` — **auth required** (clears ring buffer; for debugging).

### 4. MQTT export (`MqttPublisher`)

Follow existing HA Discovery patterns (`publishTextDiscovery`, `getBaseTopic`).
Research-backed format (official `event.mqtt` docs + HA Core sources): the HA
**MQTT event entity** is the idiomatic representation for discrete device
events; a text/sensor entity would only show a "last line" without history.

- **Raw topic** `pool/log` (JSON lines, QoS 0, non-retained) — for external
  tools. Payload: `{"level":"WARN","uptime":3610,"msg":"..."}`.
- **HA event entity "Pool Controller Event"** via MQTT Discovery
  (`platform: event`) on topic `pool/event`:
  - Discovery topic `homeassistant/event/<node_id>/pool_controller_event/config`
    (retained), payload `{"platform":"event","name":"Pool Controller Event",
    "unique_id":"pool_controller_event","state_topic":"pool/event",
    "event_types":["mode_changed","pump_on","pump_off","wifi_connected",
    "wifi_disconnected","mqtt_connected","mqtt_disconnected","ota_started",
    "ota_success","ota_failed","factory_reset","warning","error"],
    "entity_category":"diagnostic","device":{...same block as sensors...}}`.
  - State payload (JSON, **must** contain `event_type` ∈ `event_types`):
    `{"event_type":"pump_on","message":"Pump turned on","level":"INFO","uptime":3610}`
    — extra keys (`message`, `level`, `uptime`) become entity attributes.
    Events are stateless; replayed retained messages are discarded by HA.
  - Separate `pool/event` topic (instead of reusing `pool/log`) keeps the
    event_type always present without a fragile `value_template` on the raw log
    stream. The `value_template` is intentionally omitted.
- **Volume control:** only WARN/ERROR + curated events (mode changes, pump
  toggles, WiFi/MQTT connect/disconnect, OTA, factory reset) are published to
  MQTT — not every INFO line.
- **Logbook (documented, not firmware):** event entities do *not* create
  message-bearing logbook entries (`event_type: LOGBOOK_ENTRY` is a known
  misconception). Users who want pool events in the HA logbook get a documented
  automation blueprint: MQTT trigger on `pool/event` → `logbook.log` service
  with `name`, `message: "{{ trigger.payload_json.event_type }}: {{ ... }}"`
  and `entity_id`. This stays in docs, never in firmware.
- **ESPHome precedent (documented):** ESPHome forwards device logs into the HA
  core log (default level WARNING, no MQTT equivalent of log_stream); our event
  entity is the MQTT-only analog.

### 5. Web UI (`data/web/index.html`, `data/web/app.js`, `data/web/style.css`)

- **More menu**: new entry `📜 Logs` → `switchTab('logs')`.
- **Tab** `#tab-logs`:
  - Console view: monospace, level colors (Info blue, Warn yellow, Error red),
    auto-scroll, pause on manual scroll-up.
  - Filter chips: All / Info / Warn / Error (frontend filter on loaded entries).
  - Auto-polling every 2 s via `/api/logs` with `since` while tab visible; stop
    when hidden (same pattern as `loadTelemetry`).
  - Timestamps: relative (`+1h 02m 33s` since boot); absolute time when NTP synced
    (compare with `/api/status` `local_time`).
  - "Clear" button — only visible when authenticated.
- **LittleFS only**: web assets are served from LittleFS exclusively —
  `WebPortal.cpp:58` documents "PROGMEM fallbacks removed". No PROGMEM mirror
  for the new view (previous plan draft's fallback is obsolete by decision).

## Error handling

- Ring buffer overflow: oldest entries overwritten (ring semantics); API always
  returns entries + `next_seq` so the client can resume.
- Reboot: logs lost (RAM-only by design); MQTT export compensates partially.
- WebServer task vs loop task: single critical section guards append.
- JSON serialization: use existing `JsonDocument` pattern; cap response size.

## Testing

- Unit tests for ring buffer: wrap-around, `since` pagination, level filter
  (existing test setup, native build).
- Migration verification: firmware builds; serial output on representative paths
  (boot, mode change, MQTT connect) is byte-identical before/after.
- Manual: `pio run --target uploadfs` + browser; verify tab in LittleFS mode
  (the only mode — no PROGMEM fallback exists).
- MQTT: mosquitto subscribe `pool/log`, HA sensor appears via discovery.

## Out of scope

- Persistent flash logging (rejected: flash wear on 24/7 device).
- Real-time streaming (SSE/WebSocket) — 2 s polling is sufficient.
- Log download/export to file in UI.
- Per-module log level configuration at runtime.
