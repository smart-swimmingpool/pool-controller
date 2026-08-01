# Implementierungsplan: Logging View (Web-Log-Konsole)

Datum: 2026-07-31
Basis: `docs/superpowers/specs/2026-07-31-logging-view-design.md` (approved, Ansatz A)
Branch: `fix/relay-r4-solar-pump`
Commit-Historie: `db3a970` (Spec-Update) → `2e40262` (research-backed MQTT event entity) → `0783fd8` (Spec-Initial)

---

## Goal

Dem Pool-Controller eine zentrale Log-Capture-Architektur geben:

1. **`LogCapture`** — RAM-Ringbuffer, ersetzt die 253 direkten `Serial.*`-Aufrufe in 21 Dateien.
2. **REST** — unauthentifiziertes `GET /api/logs` (mit `since`-Polling), authentifiziertes `POST /api/logs/clear`.
3. **MQTT** — HA-„event"-Entity-Discovery (research-backed, `event_types`) + Export von WARN/ERROR + kuratierten Events.
4. **UI** — „📜 Logs"-Button auf dem Dashboard (unauthentifizierter Zugriffspfad) + Eintrag im More-Menü; Konsole mit 2s-Polling, Level-Filtern, Pause-on-Scroll, Auth-Gated-Clear.
5. **Docs** — `mqtt-configuration.md`, `software-guide.md`, Home-Assistant-Sektion (Logbook-Automation-Blueprint).

**Explizite Vorgaben (User-Entscheidungen, in Spec committed):**
- **LittleFS only** — kein PROGMEM-Fallback für die neue View (`WebPortal.cpp:58`: „PROGMEM fallbacks removed").
- **Platzierung**: Dashboard-Button **und** More-Menü-Eintrag — Grund: Tab-Bar + More-Menü sind ohne Login unsichtbar (`app.js:262-263` `tabBar.style.display = isAuthenticated ? '' : 'none'`); der Dashboard-Button ist der einzige unauthentifizierte Einstieg.
- **Kein @librarian-Einsatz** — HA-Recherche ist abgeschlossen und committed (2e40262). Ergebnis: HA-**event**-Component (`platform: event`, `event_types`), kein Sensor/Device-Tracker.

---

## Architektur

```
Serial.*-Calls (21 Dateien, 253 Stellen)
        │  migriert zu
        ▼
LogCapture::log(level, fmt, ...)   ──►  RAM-Ringbuffer (static, kein Heap)
        │  (mit LogToSerial-Flag: Mirror auf Serial, Byte-identisch)
        ├──► WebPortal::apiGetLogs()   GET  /api/logs?since=&count=&level=   (unauthentifiziert)
        │         apiClearLogs()       POST /api/logs/clear                  (handleAuthentication)
        └──► MqttPublisher             homeassistant/event/.../config (Discovery, platform: event)
                                       homeassistant/event/.../state  ({"event_type": ..., "message": ...})
                                       pool-controller/log            (Raw, WARN/ERROR)
```

**Design-Entscheidungen:**
- `LogCapture` ist eine static-Klasse (Codebase-Konvention, vgl. `StateManager`, `DegradationManager`, `SystemMonitor`).
- Ringbuffer: statisches `std::array`-artiges C-Array, **kein Heap** (IoT-Qualitätsgate, `cpp-memory-opt`-Skill: keine String-Klassen, keine Allokation in Hot-Paths).
- Thread-Safety: `portMUX_TYPE`-Guard, nur unter `#ifdef ARDUINO` aktiv — in Native-Tests (kein portMUX-Mock) kompiliert er zu no-op. Aufrufer sind Loop-Task + WebServer-Handler (Loop) + MQTT-Callbacks (WiFi-Task) → Guard notwendig.
- Serial-Mirror bleibt **standardmäßig an** (`Flags::LogToSerial`-Semantik aus Logger-Stub): bestehendes Serial-Debugging (pio monitor) funktioniert unverändert weiter; Verifikations-Invariante „Serial-Ausgabe byte-identisch" wird so erfüllbar.
- `LogLevel`-Enum bleibt namensgleich zum Logger-Stub (`Debug=0, Info, Warning, Critical, Error`) — kein Consumer außer `Logger.cpp` selbst (verifiziert: kein Include von `Logger.hpp` in `src/` oder `test/native/`), Stub wird gelöscht.

---

## Tech Stack

- C++17, PlatformIO/ESP32 (Arduino-Framework), ArduinoJson, WebServer (async-frei, Loop-gepollt).
- Native Tests: CMake (`test/native`), `./build/test_runner` mit ASAN, `test/native/relay_safety` (separater Build).
- CI: `.github/workflows/native-tests.yml` (cmake build → test_runner → relay_safety → lcov → PR-Kommentar).
- Frontend: vanilla HTML/CSS/JS auf LittleFS (`data/web/`), 2s-Polling-Muster analog `loadTelemetry` (`app.js:1005`).

---

## Global Constraints

1. **Kein Heap in LogCapture** — statischer Ringbuffer, feste Entry-Größe, `vsnprintf` in Entry-Buffer.
2. **Serial-Ausgabe bleibt byte-identisch** — `LogCapture::log` formatiert exakt das übergebene Format (inkl. `\n`) und spiegelt es 1:1 auf Serial.
3. **Keine String-Klasse** in LogCapture — `char[]` + `vsnprintf` (Clean-Code/Heap-Regeln).
4. **Unauthentifizierte Endpoints nur lesend** — `GET /api/logs` (Limit/Cap), Schreibzugriff (`clear`) ausschließlich hinter `handleAuthentication()`.
5. **XSS-Sicherheit in der UI** — Log-Messages sind Fremdtext; im Frontend per `textContent`/Escaping rendern, nie per `innerHTML`.
6. **Native Tests müssen grün bleiben** — `test_runner` **und** `relay_safety` (separater Build), ASAN-Optionen wie CI.
7. **Dokumentation EN+DE** — alle Docs-Dateien existieren als `.md` + `.de.md`.
8. **LittleFS-only** — keine PROGMEM-Spiegel für neue Assets.
9. Jeder Task endet mit Build + Test + Commit (Conventional Commits, Scope `logging`).

---

## Tasks

### Task 1 — LogCapture-Kern (Ringbuffer + API)

**Dateien:** `src/LogCapture.hpp` (neu), `src/LogCapture.cpp` (neu), `src/Nodes/Logger.{hpp,cpp}` (löschen)

**Schritte:**
1. `src/LogCapture.hpp` anlegen:

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace PoolController {

enum class LogLevel : std::uint8_t { Debug = 0, Info, Warning, Critical, Error };

struct LogEntry {
  std::uint32_t seq;        // monoton steigend, für since-Polling
  std::uint32_t uptimeMs;   // millis() zum Zeitpunkt des Eintrags
  LogLevel level;
  char message[LOG_MSG_SIZE];
};

class LogCapture final {
public:
  static constexpr std::size_t LOG_BUFFER_ENTRIES = LOG_BUFFER_SIZE / LOG_MSG_SIZE;
  static void begin();
  static void log(LogLevel level, const char *fmt, ...);
  static void logEvent(const char *eventType, const char *fmt, ...);
  static std::size_t getEntries(std::uint32_t sinceSeq, std::size_t maxCount,
                                LogLevel minLevel, LogEntry *out, std::size_t outCapacity);
  static std::uint32_t lastSeq();
  static void clear();
  static const char *levelName(LogLevel level);
  static LogLevel parseLevel(const char *name);   // "info"|"warning"|"error" → Level, sonst Info
  static bool isLogToSerial();
  static void setLogToSerial(bool enabled);
};

}  // namespace PoolController

#define LOG_DEBUG(...)  PoolController::LogCapture::log(PoolController::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)   PoolController::LogCapture::log(PoolController::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...)   PoolController::LogCapture::log(PoolController::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...)  PoolController::LogCapture::log(PoolController::LogLevel::Error, __VA_ARGS__)
```

2. `src/LogCapture.cpp`:
   - Defaults: `LOG_BUFFER_SIZE` = 8192, `LOG_MSG_SIZE` = 96 (platformio.ini `build_flags`: `-DLOG_BUFFER_SIZE=...` überschreibbar — bestehende `build_flags`-Blöcke `platformio.ini:33,56` ergänzen, Default in Header).
   - Statischer Ring: `static LogEntry s_buffer[LOG_BUFFER_ENTRIES];`, `static std::size_t s_head;` (nächster freier Slot), `static std::uint32_t s_seq;`
   - `log()`: `va_list` → `vsnprintf(message, sizeof, fmt, args)` → in Ring schreiben → falls `s_logToSerial`: `Serial.print(entry.message)`.
   - `logEvent(eventType, ...)`: Entry mit `level=Info` und `message` = `"[eventType] <formatted>"` — Marker für den MQTT-Export (Task 4), Serial-Mirror identisch.
   - `getEntries`: vom ältesten Eintrag ≥ `sinceSeq` (Ring durchlaufen, seq-Vergleich) bis `maxCount`, filter `>= minLevel`, Kopie nach `out`.
   - `lastSeq()`: aktueller `s_seq`-Stand (für `next` im REST-Payload).
   - `clear()`: Ring leeren, `s_seq` NICHT zurücksetzen (sonst brechen Polling-Clients), s_head=0.
   - Guard: `portMUX_TYPE s_mux;` nur `#ifdef ARDUINO` (`portMUX_INITIALIZER_UNLOCKED`), in `log/getEntries/clear` `portENTER_CRITICAL(&s_mux)` … `portEXIT_CRITICAL(&s_mux)`. Native Build (kein portMUX) → no-op ohne `#ifdef`-Zweig.
   - `begin()`: `s_head=0; s_seq=0; s_logToSerial=true;` (init Mux).

3. `src/Nodes/Logger.hpp` + `Logger.cpp` löschen (`git rm`). Kein anderer Referenzpunkt existiert (verifiziert).

**Test (TDD — Test zuerst schreiben):**
- `test/native/tests/test_logcapture.cpp` (neu), Funktion `run_logcapture_tests()`:
  - Ring-Wraparound: `LOG_BUFFER_ENTRIES + 5` Einträge loggen → `getEntries(0, 4096, Debug, ...)` liefert letzte `LOG_BUFFER_ENTRIES`, älteste seq > 0.
  - seq-Monotonie: nach N Logs ist `lastSeq() == N`; Einträge haben strikt steigende seq.
  - since-Filter: `getEntries(sinceSeq=N, ...)` liefert nur seq > N.
  - Level-Filter: nur `>= Warning` bei `minLevel=Warning`.
  - `clear()`: danach `getEntries(0, ...) == 0`, `lastSeq()` unverändert.
  - Truncation: Message > `LOG_MSG_SIZE` wird gekappt, kein Overflow (ASAN-frei).
  - Kein Heap: statische Buffer (kein dynamisches Verhalten testbar; ASAN/leak-check im CI).
  - `logEvent`: Message beginnt mit `[` und enthält eventType.
- `test/native/CMakeLists.txt`: `${PROJ_ROOT}/src/LogCapture.cpp` zu `SERVICE_SOURCES` (Zeile 52-59), `tests/test_logcapture.cpp` zu `TEST_SOURCES` (Zeile 69).
- `test/native/tests/test_main.cpp`: `extern int run_logcapture_tests();` (Zeile ~102) + `total += run_logcapture_tests();` (Zeile ~116).

**Verify:**
```bash
cmake -B build -S . && cmake --build build && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./build/test_runner
cmake -B relay_safety/build -S relay_safety && cmake --build relay_safety/build && ./relay_safety/build/test_relay_safety
pio run   # Firmware kompiliert (LogCapture wird in setup() noch nicht benutzt — siehe Task 2)
```

**Commit:** `feat(logging): add LogCapture ring buffer core with native tests`

---

### Task 2 — Integration in PoolController-Setup

**Dateien:** `src/PoolController.cpp` (setup: ~Zeile 268), `platformio.ini` (build_flags 33, 56)

**Schritte:**
1. `#include "LogCapture.hpp"` ergänzen.
2. In `setup()` unmittelbar nach `Serial.begin(...)` (vor `StateManager::begin()` Zeile 268): `LogCapture::begin();` — Ring muss vor allen Logging-Seiten stehen.
3. `platformio.ini`: `-DLOG_BUFFER_SIZE=8192` zu den `build_flags`-Blöcken (Zeile 33, 56) hinzufügen (Default steht im Header; explizit machen für OTA-Konsistenz beider Envs).

**Verify:** `pio run` grün; `test_runner` + `relay_safety` grün (keine LogCapture-Nutzung außer begin → keine Verhaltensänderung).

**Commit:** `feat(logging): initialize LogCapture during boot`

---

### Task 3 — Serial-Migration (21 Dateien, 253 Stellen)

**Level-Inferenz-Regeln (mechanisch):**

| Marker im String | Level |
|---|---|
| `✖`, `ERROR`, `FAIL`, `CRITICAL` | `LOG_ERROR` |
| `⚠`, `WARN`, `WARNING` | `LOG_WARN` |
| `✔`, `✓`, sonst nichts | `LOG_INFO` |
| rein technische Poll-Diagnostik | `LOG_DEBUG` (nur wenn erkennbar Debug-Charakter, z. B. Rohwerte-Dumps in Schleifen) |

**Mechanik:**
- `Serial.printf("...", args)` → `LOG_INFO("...", args)` (bzw. Level nach Tabelle) — Format inkl. `\n` unverändert (Serial-Mirror ist byte-identisch).
- `Serial.println("...")` → `LOG_INFO("...\n")` (println hängt `\n` an; Mirror muss identisch sein).
- `Serial.print("...")` (ohne `\n`) → `LOG_INFO("...")`.
- Mehrteilige Chains (`Serial.print(a); Serial.print(b); Serial.println(c);`) → zu **einer** `LOG_*`-Zeile mit kombiniertem Format mergen (gleiche Bytefolge: `"ab" + c + "\n"`), wo trivial; sonst je Fragment eine Zeile beibehalten.
- `Serial.println()` ohne Argument (Leerzeile) → `LOG_INFO("")`.
- `Update.printError(Serial)` in OtaUpdater → `Serial.print(Update.errorString())`-Ersatz via `LOG_ERROR("%s", Update.errorString())` NUR wo die Updater-API es erlaubt (printError ist library-intern auf Serial verdrahtet — **unverändert lassen**, kommentieren).

**Reihenfolge** (klein → groß, jeder Datei-Task einzeln committen; Call-Zahlen verifiziert):
1. `src/Timer.cpp` (1) · 2. `src/RuleTimer.cpp` (1) · 3. `src/RuleManu.cpp` (1) · 4. `src/TimeClientHelper.cpp` (2) · 5. `src/ESP32TemperatureNode.cpp` (2) ·
   6. `src/StatusLed.cpp` (5) · 7. `src/RelayModuleNode.cpp` (5) · 8. `src/NorviButtonHandler.cpp` (7) · 9. `src/NorviOledDisplay.cpp` (8) ·
   10. `src/WpsProvisioner.cpp` (10) · 11. `src/ConfigManager.cpp` (11) · 12. `src/RuleBoost.cpp` (12) · 13. `src/WebPortal.cpp` (13) ·
   14. `src/NetworkManager.cpp` (15) · 15. `src/DegradationManager.cpp` (15) · 16. `src/RuleAuto.cpp` (16) · 17. `src/DallasTemperatureNode.cpp` (20) ·
   18. `src/OperationModeNode.cpp` (23) · 19. `src/PoolController.cpp` (26) · 20. `src/MqttPublisher.cpp` (27) · 21. `src/OtaUpdater.cpp` (33)

**Sonderfälle:**
- `NorviOledDisplay`/`NorviButtonHandler`: Serial nur als Debug-Pfad — Flags/Guard-Bedingungen (z. B. `if (DEBUG)`) beibehalten, nur die Ausgabe ersetzen.
- `DallasTemperatureNode`: Poll-Schleifen-Logging → `LOG_DEBUG` (würde sonst Ringbuffer in Sekunden fluten).
- `OperationModeNode`: `✖ UNDEFINED Mode` → `LOG_ERROR`, `⚠ NTP time sync failed` → `LOG_WARN`.
- `MqttPublisher`/`NetworkManager`/`OtaUpdater`/`WebPortal`: kuratierte Events (siehe Task 4, Schritt 3) hier gleichzeitig als `logEvent` statt `log` setzen, wo Mode-/Pump-/Wifi-/MQTT-/OTA-Übergänge geloggt werden (kein zweiter Durchlauf).

**Per-Task Verify:**
```bash
pio run && cmake -B build -S . && cmake --build build && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./build/test_runner && cmake -B relay_safety/build -S relay_safety && cmake --build relay_safety/build && ./relay_safety/build/test_relay_safety
```
Invarianz-Check je Datei: `git diff` auf reine Call-Substitution prüfen (keine Logikänderung).

**Commit je Datei:** `refactor(logging): migrate Serial calls in <Datei> to LogCapture` (bis zu 21 Commits; kleine Gruppen á 3-4 Dateien erlaubt, wenn diff klar bleibt).

---

### Task 4 — REST-Endpoint `/api/logs`

**Dateien:** `src/WebPortal.cpp` (setupRoutes:180, apiGetStatus:436 als Muster), `src/WebPortal.hpp`

**Schritte:**
1. `WebPortal.hpp`: `static void apiGetLogs();` + `static void apiClearLogs();` (private, wie apiGetStatus).
2. `setupRoutes()` ergänzen (Muster Zeile 193 / 199-208):
```cpp
// Log view — GET unauthenticated (read-only), clear requires login
server_.on("/api/logs", HTTP_GET, apiGetLogs);
server_.on("/api/logs/clear", HTTP_POST, []() {
  if (!handleAuthentication())
    return;
  apiClearLogs();
});
```
3. `apiGetLogs()`:
   - Query: `since` (seq, default 0), `count` (default 200, Cap 500), `level` (`parseLevel`, default `Info` — damit Debug nicht im Web landet; `level=debug` explizit möglich).
   - JSON: `{"ok":true, "next":<lastSeq()+1>, "entries":[{"seq":…,"t":<uptimeMs>,"level":"info|warning|error","msg":"…"}, …]}`
   - `JsonDocument doc;` + `serializeJson` → `server_.send(200, "application/json", payload)` — exakt wie `apiGetStatus` (Zeile 436ff, Payload-Buffer-Muster).
4. `apiClearLogs()`: `LogCapture::clear();` → `{"ok":true}`.
5. Keine PROGMEM-Anteile (LittleFS-only-Vorgabe).

**Test (TDD):**
- Neues Muster anlehnen an `test_webportal_json.cpp` (WebServer-Mock fängt `send()` via `wsCapture`). Da `apiGetStatus` private ist und der bestehende Test
  inline-JSON baut, wird für `apiGetLogs` derselbe Weg genutzt: **Test-Hook** — `LogCapture` selbst unit-testen (Task 1) + JSON-Serialisierung über einen
  **public static Test-Hook** testen, z. B. `WebPortal::buildLogsJson(uint32_t since, size_t count, LogLevel minLevel, char *buf, size_t bufSize) -> size_t`
  (public for testing, Muster: „Rate limiting helpers (public for testing)" existiert bereits in `WebPortal.hpp`). `apiGetLogs` ruft den Hook auf und sendet.
- Testfälle: since-Filter wirkt, count-Cap, Level-Filter, `next` = lastSeq+1, leeres Ergebnis → `entries: []`.
- Registrierung: `test_webportal_json.cpp` erweitern oder neue Testdatei + `test_main.cpp`-Eintrag.

**Verify:** test_runner + relay_safety grün; `pio run` grün.

**Commit:** `feat(logging): add unauthenticated /api/logs endpoint and authenticated clear`

---

### Task 5 — MQTT-Event-Export

**Dateien:** `src/MqttPublisher.cpp`/`.hpp`, `src/LogCapture.hpp` (falls Event-Zugriff nötig)

**Schritte:**
1. `publishEventDiscovery()` — Muster `publishTextDiscovery` (Zeile 212) / `publishNumberDiscovery` (Zeile ~200), TopicBuilder (`homeassistant/<component>/pool-controller/<object-id>/config`, `cfgTopic.build("event", objectId, "/config")`):
```cpp
void MqttPublisher::publishEventDiscovery(const char *objectId, const char *name, const char *icon) {
  TopicBuilder cfgTopic, stateTopic;
  JsonDocument doc;
  doc["name"] = name;
  doc["unique_id"] = deviceId_ + "_" + objectId;
  doc["state_topic"] = stateTopic.build("event", objectId, "/state");
  doc["availability_topic"] = "homeassistant/sensor/pool-controller/availability";
  doc["platform"] = "event";                    // research-backed: HA event component
  JsonArray eventTypes = doc["event_types"].to<JsonArray>();
  eventTypes.add("LOG_WARN"); eventTypes.add("LOG_ERROR");
  eventTypes.add("MODE_CHANGED"); eventTypes.add("PUMP_ON"); eventTypes.add("PUMP_OFF");
  eventTypes.add("WIFI_CONNECTED"); eventTypes.add("WIFI_DISCONNECTED");
  eventTypes.add("MQTT_CONNECTED"); eventTypes.add("MQTT_DISCONNECTED");
  if (icon) doc["icon"] = icon;
  addDeviceInfo(doc);
  char payloadBuf[1024];
  serializeJson(doc, payloadBuf, sizeof(payloadBuf));
  NetworkManager::publish(cfgTopic.build("event", objectId, "/config"), payloadBuf, true);
}
```
2. In `publishDiscovery()` (Zeile ~430, nach `isMqttConnected()`-Guard): `publishEventDiscovery("logs", "Pool Controller Logs", "mdi:clipboard-text-outline");`
3. **Export-Pumpe** — in `publishStates()` (wird im Loop 2× aufgerufen): neue Einträge seit `s_lastExportedSeq`:
   - `LogCapture::logEvent`-Marker `[EVENT]` bzw. die Event-Marker (Mode/Pump/Wifi/MQTT/OTA — in Task 3 als `logEvent` gesetzt) → `{"event_type": "<TYPE>", "message": "<msg>"}` auf `stateTopic` (`event`-Component) — **nur** bei Änderung (dedup: gleiche seq nicht erneut).
   - WARN/ERROR-Einträge ohne Event-Marker → `{"event_type": "LOG_WARN"|"LOG_ERROR", "message": "..."}` auf denselben `stateTopic`.
   - **Raw-Topic** `pool-controller/log` (via `getBaseTopic()`): WARN/ERROR als JSON-Lines `{"seq":…,"t":…,"level":…,"msg":…}` für externe Tools — mit `s_lastExportedSeq` dedupliziert.
   - `s_lastExportedSeq = LogCapture::lastSeq();` am Ende (Volumen-Kontrolle: keine Info/Debug über MQTT).
4. **Kuratierte Event-Trigger** (setzen als `logEvent` beim Übergang, Referenz aus Task 3):
   - Mode-Wechsel: `OperationModeNode`/`PoolController` (mode set) → `logEvent("MODE_CHANGED", ...)`
   - Pumpe: `PoolController::togglePoolPump/toggleSolarPump` → `PUMP_ON`/`PUMP_OFF`
   - WLAN: `NetworkManager` connect/disconnect (Zeile ~256/268) → `WIFI_CONNECTED`/`WIFI_DISCONNECTED`
   - MQTT: `MqttPublisher::onMqttConnect/onMqttDisconnect` → `MQTT_CONNECTED`/`MQTT_DISCONNECTED`

**Test (TDD):**
- `test_mqttpublisher.cpp` erweitern (Muster `mqttCapture.published`):
  - Nach `begin()`: Discovery-Payload für `homeassistant/event/pool-controller/logs/config` enthält `"platform":"event"` und `event_types` mit `LOG_WARN`.
  - Export-Pumpe: WARN-Entry in LogCapture → nach `publishStates()` ist `{"event_type":"LOG_WARN",...}` auf dem event-state-Topic publiziert; Info-Entry wird NICHT publiziert.
  - Dedup: zweiter `publishStates()`-Aufruf ohne neue Einträge publiziert nichts Neues.
  - `logEvent("MODE_CHANGED", ...)` → `event_type` = `MODE_CHANGED`.
- MqttPublisher ist WRAPPER_SOURCE (wrapper generiert `wrappers/MqttPublisher.cpp` mit `mqttCapture`) — neue Publikationen laufen über `NetworkManager::publish` (Mock, `_mqttConnected=true`).

**Verify:** test_runner + relay_safety grün; `pio run` grün.

**Commit:** `feat(logging): export log events via MQTT event entity with HA discovery`

---

### Task 6 — Web-UI: Dashboard-Button, More-Eintrag, Log-Konsole

**Dateien:** `data/web/index.html`, `data/web/app.js`, `data/web/style.css` (LittleFS-Deployment via `uploadfs`)

**Schritte:**
1. `index.html`:
   - Dashboard-Header (bei Login-Banner-Zeile ~146): `<button id="btnLogs" class="...">📜 Logs</button>` — sichtbar **immer** (unauthentifizierter Einstieg).
   - More-Menü (`moreMenu`, Zeile ~67-83): `<a href="#" data-tab="logs">📜 Logs</a>` vor `wifi`.
   - Neuer `<section id="tab-logs" class="tab">` (Muster `tab-wifi` etc.):
     - Kopf: Filter-Chips `Alle` / `Warnungen` / `Fehler` + `Logs löschen`-Button (`hidden` ohne Login).
     - `<div id="logConsole">` (Scroll-Container) + `<div id="logConsoleEmpty">`.
2. `app.js`:
   - `moreTabs = ['logs', 'wifi', 'mqtt', 'system', 'about']` (Zeile 15) — `logs` wird über das More-Menü erreicht, `barTab`-Mapping greift.
   - `switchTab('logs')` zeigt `#tab-logs` (bestehende Mechanik reicht).
   - `loadLogs()`: `fetch('/api/logs?since=' + lastSeq + '&count=200&level=' + levelFilter)` → Einträge an `#logConsole` anhängen (`textContent`-basiert, keine innerHTML mit Fremdtext!), `lastSeq = data.next`; Auto-Scroll ans Ende **nur wenn** User nicht hochgescrollt hat (`scrollTop + clientHeight >= scrollHeight - 40`); relative Zeit (Uptime-ms → `h:mm:ss`).
   - `setInterval(loadLogs, 2000)` (parallel zu `loadTelemetry`, Zeile ~1005); beim Tab-Wechsel zu `logs` sofort einmal laden.
   - Filter-Chips: Klick setzt `levelFilter` (`info|warning|error`), leert Konsole, `lastSeq=0`, sofort `loadLogs()`.
   - Clear-Button: nur sichtbar wenn `isAuthenticated` (`updateAuthUI()`-Erweiterung, Zeile ~225); Klick → `fetch('/api/logs/clear', {method:'POST'})` → Konsole leeren, `lastSeq=0`.
   - Escape-Hilfe: `function escapeHtml(s)` (ersetze `<`, `>`, `&`, `"`) — verwenden für `msg`.
3. `style.css`: `#logConsole` (monospace, `overflow-y: auto`, max-height), Level-Farben (`.log-debug` grau, `.log-info` default, `.log-warn` amber, `.log-error` rot), Chips, Empty-State.

**Hinweis:** UI-Arbeit wird beim Ausführen an `@designer` gegeben (Layout/Feel) — Copy danach vom Orchestrator geprüft. Design-Absicht (Polling, Pause-on-Scroll, Filter, Auth-Gating) bleibt fix.

**Verify:** `pio run` + `uploadfs`-Deployment auf Gerät (oder lokale Inspektion); manuell: Button ohne Login erreichbar, Konsole füllt sich, Scroll-Pause, Filter, Clear nur mit Login, XSS-Check (Sonderzeichen in Logs rendern sicher).

**Commit:** `feat(logging): add web log console with polling, filters and auth-gated clear`

---

### Task 7 — Dokumentation (EN + DE)

**Dateien:** `docs/mqtt-configuration.md` + `.de.md`, `docs/software-guide.md` + `.de.md`, `docs/home-assistant/_index.md` + `.de.md`

**Inhalt:**
1. `mqtt-configuration.*`: neues Topic `homeassistant/event/pool-controller/logs/config` (Discovery, `platform: event`, `event_types`-Liste), State-Topic, Raw-Topic `pool-controller/log` (JSON-Lines, WARN/ERROR), Beispiel-Payload.
2. `software-guide.*`: REST-API `GET /api/logs` (Parameter `since`/`count`/`level`, Beispiel-Response), `POST /api/logs/clear` (Auth), Web-Log-Konsole (Platzierung: Dashboard-Button + More-Menü, Filter, Auto-Polling).
3. `home-assistant/_index.*`: **Logbook-Automation-Blueprint** (dokumentiert, nicht im Firmware) — YAML-Snippet, das `event`-Entity-Events in das HA-Logbook schreibt (Trigger `event`, Bedingung event_type in Liste, `logbook.log`-Action).

**Verify:** Markdown-Validierung (Dateien existieren EN+DE, Links konsistent), kein Build-Einfluss.

**Commit:** `docs(logging): document log view API, MQTT event entity and HA logbook blueprint`

---

### Task 8 — Gesamt-Verifikation & Review

**Schritte:**
1. Vollständiger CI-Lauf lokal: `pio run` + `test_runner` (ASAN) + `relay_safety`.
2. Serial-Invarianz-Stichprobe: vor/nach Diff der formatierten Strings (kein Zeichenverlust, `\n`-Semantik println→`\n`).
3. Review durch @oracle (Risiko: Ringbuffer-Corner-Cases, Thread-Safety, XSS, MQTT-Volumen) — optional, wenn Task 1-6 ohne Auffälligkeiten.
4. Finale Commit-Historie prüfen (Conventional Commits, Scope `logging`).
5. Deployment-Vorbereitung: `pio run`-Binary für `uploadfs` + OTA (Deploy-Skill folgt separat, nicht Teil dieses Plans).

**Commit:** ggf. `fix(logging): ...` für Review-Fundstücke.

---

## Selbst-Review gegen Spec

| Spec-Anforderung | Plan-Abdeckung |
|---|---|
| Ansatz A: zentrale Capture + Migration | Task 1 + 3 (253 Calls / 21 Dateien, verifizierte Zahlen) |
| RAM-Ringbuffer, kein Heap | Task 1 (statisch, vsnprintf, IoT-Gates) |
| Unauthentifiziertes `/api/logs` (Polling `since`) | Task 4 (since/count/level, Cap 500) |
| Clear nur mit Login | Task 4 (`handleAuthentication`) |
| MQTT-Export WARN/ERROR + kuratierte Events | Task 5 (event-Entity + Raw-Topic, dedup via lastExportedSeq) |
| HA-Discovery research-backed (event-Typ) | Task 5 (committed Ergebnis 2e40262, `platform: event`) |
| UI: Konsole, 2s-Polling, Filter, Pause-on-Scroll | Task 6 (Polling-Muster wie loadTelemetry) |
| Platzierung: Dashboard-Button + More-Menü | Task 6 (Dashboard = unauthentifizierter Einstieg, More für eingeloggte) |
| LittleFS-only, kein PROGMEM | Task 4/6 (keine PROGMEM-Anteile) |
| Docs EN+DE | Task 7 (alle 3 Docs-Paare) |
| Native Tests + relay_safety grün | jeder Task (Verify-Block) |
| Ohne @librarian | eingehalten — Recherche committed, keine externe Recherche nötig |

## Risiken / Offene Punkte

1. **Ringbuffer-Volumen**: Poll-Schleifen-Logging (DallasTemperature) muss auf `LOG_DEBUG` — sonst ist der 8KB-Ring in Sekunden überschrieben und die Web-Konsole zeigt nur Hot-Path-Logs. Task 3 Sonderfälle deckt das ab; Review-Schritt 8.3 validiert.
2. **`Update.printError(Serial)`** bleibt serial-gebunden (library-intern) — Web/MQTT sehen OTA-Fehler nur über den kuratierten `OTA_FAILED`-Event (Task 3, Sonderfälle).
3. **Event-Dedup über Reconnect**: `s_lastExportedSeq` ist RAM-only — nach Reboot werden ältere seqs nicht re-exportiert (gewollt; MQTT-retained nur Discovery, nicht Events).
4. **XSS**: Log-Console rendert nur via `textContent`/`escapeHtml` — Fremdtexte (Mode-Namen, WLAN-SSIDs) sind potenzielle Payloads.
