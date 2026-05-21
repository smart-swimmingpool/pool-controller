# AGENTS.md

Zweck: Regeln und Standards für einen Coding-Agent, der PlatformIO-basierten
IoT-Firmware für ESP32 entwickelt, refaktoriert, analysiert und stabilisiert.
Fokus: Architektur, Linting, Speicher- und Ressourcenmanagement, Sicherheit,
24/7-Betrieb, Wartbarkeit, Tests, Releases und OTA.

## 1. Arbeitsweise

Der Agent:

- liefert minimal-invasive Änderungen mit Begründung, Impact-Analyse (RAM/Flash/CPU), Risiken, Rollback-Hinweisen.
- verhindert blockierende Patterns im Laufzeit-Code (long `delay()`, busy waits, blockierende Netzwerk-Calls im Main-Loop).
- vermeidet unsichere dynamische Allokationen in Hot-Paths und ungeprüfte Heap-Nutzung.
- folgt einem CI-regelbasierten Prozess (Build + Tests + Lint).

## 2. Zielplattformen & Framework

- Plattform: ESP32 (inkl. Varianten wie S3, C3); Frameworks: Arduino oder ESP-IDF.
- PlatformIO: Single-Source für Build-Konfiguration (`platformio.ini`), Projekt-Environments, Lib-Pins.

## 3. Projektstruktur und Architektur

Empfohlene Schichten (auch wenn das repository aktuell anders aussieht, Ziel ist schrittweise Annäherung):

- `src/app/` Anwendungslogik (Use-Cases, State Machines)
- `src/drivers/` Hardware-Treiber (GPIO, I2C, SPI, ADC), keine Business-Logik
- `src/services/` Netzwerk, MQTT/HTTP, Time, Storage, OTA, Telemetry
- `src/platform/` Board-spezifische Adapter, `#ifdef` nur hier, nicht in App/Services
- `include/` Öffentliche Header, klare Interfaces
- `test/` Unit-/Component-Tests (PlatformIO Unity)

Architekturregeln:

- App kennt Services über Interfaces, keine direkten Implementierungen.
- Drivers ohne Abhängigkeiten zu Services/App-State.
- Scheduler/Timer/State Machines zentral geplant, nicht verteilt über `loop()`.
- Fehler-Resilienz und Fallback-Strategien eingebaut.

## 4. Coding-Standards

- Sprache: C++17 (wenn möglich), keine unkontrollierten Exceptions auf eingebetteten Targets.
- Header: `include-what-you-use`, kein globales `using namespace`.
- Konstanten: `constexpr`, `enum class`.
- Ownership: RAII oder klar definierte Allokations-/Deallokationsverantwortung.
- Schnittstellen: prefer Span-artige Übergaben, keine impliziten Kopien.
- Fehlerbehandlung immer explizit, kein stilles Ignorieren.
- Logging: niemals in ISR, niedrige Frequenz in Loop-Hot-Paths.

## 5. Linting & Format

- Format: `clang-format` repo-weit einheitlich.
- Lint: `clang-tidy` wo möglich, sonst `cppcheck`.
- Statische Checks: Warnungen auf Maximum, keine neuen Warnungen akzeptieren.
- CI: Build (`pio run`), Tests (`pio test`), Lint/Format-Checks grün.

## 6. Commit-Konventionen (Conventional Commits)

Commit Messages müssen dem Conventional Commits-Standard folgen:
`<type>[optional scope]: <description>`
Commit Types umfassen mindestens:

- `feat` für neue Funktionen
- `fix` für Fehlerbehebungen
- `docs` für Änderungen an Dokumentation
- `style` für Formatierung/Code Style
- `refactor` für Code-Umstrukturierungen ohne funktionale Änderung
- `perf` für Performance-Optimierungen
- `test` für Test-Änderungen
- `chore` für Wartung/Tooling/Build-Änderungen

Commit Message Body kann Motivation und Kontext enthalten. Breaking Changes
müssen mit `BREAKING CHANGE:` im Footer markiert werden.
Commit Messages müssen dem Format entsprechen, damit automatische Changelog-Generierung, Versionierung und CI-Checks funktionieren.:contentReference[oaicite:1]{index=1}

## 7. Build-Konfiguration

- `platformio.ini`: zentrale Flags, Versions-Defines (`FW_NAME`, `FW_VERSION`).
- Build-Artefakte: Debug vs Release:
  - Debug: intensiver Logging, Heap/Stack Checks.
  - Release: optimiert, gedämpftes Logging, Sicherheits-Features aktiv.
- Build-Flags: `-D LOG_LEVEL`, `-D NDEBUG` steuerbar über Environments.

## 8. Speicher & Ressourcen

- Kein unbounded dynamic Heap/Fragmentierung:
  - Statische Puffer wo möglich, wiederverwendbare Ring-Buffers.
  - Vermeide String-Objekte (`String`) in Loops.
- JSON: `StaticJsonDocument` mit statischem Speicher vorab dimensionieren.
- Heap/Stack-Metriken überwachen (`ESP.getFreeHeap()`, `heap_caps_get_free_size`, Task-Stack-High-Watermarks).
- PSRAM gezielt nutzen, nicht blind, mit Metriken.
- Memory-Pools statt häufige Allokationen.

## 9. RTOS & Nebenläufigkeit

ESP32:

- FreeRTOS Tasks mit klarer Verantwortlichkeit.
- Kommunikation über Queues/Semaphores; keine globals ohne Schutz.
- Prioritäten bewusst setzen, Priority Inheritance bei Mutex.
- Task-Stack dimensionieren und überwachen.

## 10. Sicherheit

ESP32 Hardware-Security:

- **Secure Boot**: Boot-Image-Verifikation vor Start, Schlüssel offline erzeugen, eFuse planen. :contentReference[oaicite:0]{index=0}
- **Flash Encryption**: Schutz des Flash-Inhalts (Firmware, Credentials, Zertifikate), Release-Mode vor Produktion. :contentReference[oaicite:1]{index=1}
- Debug Interfaces (JTAG/UART) im Produktions-Build deaktivieren. :contentReference[oaicite:2]{index=2}
- TLS für Netzverbindungen (MQTTS/HTTPS) mit CA/Key-Validation.
- Secrets nicht im repository.

## 11. OTA & Updates

- OTA mit mindestens zwei Partitions-Slots, Anti-Rollback/Checksum/Validity. :contentReference[oaicite:3]{index=3}
- Sicherer OTA: HTTPS, Signaturen, Rollback-Mechanismus.
- Update-Failure Detection (Task Init + Health-Checks vor Markieren aktiv).

## 12. 24/7-Robustheit

- Watchdogs aktiv (Loop/Tasks).
- Netzwerk-Resilienz: Reconnect-Backoff + Jitter, Offline-Betrieb möglich.
- Time via NTP mit Fallback.
- Persistenz: Flash-Writes minimieren, Bundling, Debounce.
- Health-Metrics sammeln: Uptime, Heap/Stack, Reset-Reason, Wifi/MQTT Status.

## 13. Logging & Telemetrie

- Strukturierte Logs (KV-Form).
- Rate Limits für wiederkehrende Events.
- Health Endpoints oder Telemetrie-Reports.

## 14. Konfiguration & Secrets

- Defaults in `config_defaults.h`.
- Runtime-Konfiguration über Filesystem (LittleFS/NVS) validieren.
- Keine hartkodierten Secrets.

## 15. Tests

- Unit-Tests für Parser, Protocol/State, Backoff, Scheduler.
- Native Tests (`platform = native`) bevorzugt für CI.
- Komponententests mit Mocks/Simulations.

## 16. Dependencies

- Minimiert, begründet, Version-Pinned.
- Lizenz-Checks; Updates mit CI-Absicherung.

## 17. Release & CI

- Release: Sicherheitsfeatures, Monitoring, Debug ausschalten.
- CI: Lint, Build, Tests, Heap/Stack Reports, Memory-Analyse.

## 18. Antipatterns (verboten)

- Unlimitierte `delay()`, Busy-Wait, blockierende Netzwerk-Calls im Loop.
- Häufige Heap-Allokationen in Hot-Paths.
- String-Objekte in Zyklus-Code.
- Globale state ohne Synchronisation.
- Große Puffer (`> 512 B`) als file-scope `static`, wenn sie nur einmalig
  (z. B. Setup-Pfad) benötigt werden — stattdessen als function-local Variable
  deklarieren, damit der Speicher nur während der Ausführung belegt ist.
- Pointer-Member ohne In-Class-Initialisierung (`= nullptr`); uninitialisierte
  Pointer sind UB und führen zu schwer reproduzierbaren Crashes.
- Statische `String`-Puffer mit `.concat()` ohne vorheriges Reset — führt bei
  mehrfachem Aufruf zu verdoppelten Inhalten.

## 19. ESP-IDF-Versionskompatibilität

Beim Einsatz von ESP-IDF-APIs immer mit `ESP_IDF_VERSION_VAL` absichern:

```cpp
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  // Neue API (ESP-IDF 5.x)
#else
  // Legacy-API (ESP-IDF 4.x)
#endif
```

Bekannte API-Brüche zwischen ESP-IDF 4 und 5:

| API | ESP-IDF 4.x | ESP-IDF 5.x |
|-----|-------------|-------------|
| Task Watchdog Init | `esp_task_wdt_init(uint32_t s, bool panic)` | `esp_task_wdt_init(const esp_task_wdt_config_t *)` |
| Task Watchdog (wenn bereits initialisiert) | — | `esp_task_wdt_reconfigure(const esp_task_wdt_config_t *)` |
| WPS Start | `esp_wifi_wps_start(int)` | `esp_wifi_wps_start()` |

Das Arduino-Framework für ESP32 (espressif32 ≥ 6.x) verwendet ESP-IDF 5.x und
initialisiert den TWDT bereits vor `setup()`. Daher muss beim Anpassen des
WDT-Timeouts `esp_task_wdt_reconfigure()` statt `esp_task_wdt_init()` verwendet
werden.

## 20. JSON-Puffer-Dimensionierung

`ArduinoJson`-Regeln für ESP32-Projekte:

- `StaticJsonDocument<N>` für N anhand des tatsächlichen JSON-Inhalts
  bemessen (ArduinoJson Assistant: <https://arduinojson.org/v6/assistant/>).
- Den Serialisierungspuffer (`char buffer[]`) **mindestens 25 % größer** als
  der erwartete maximale JSON-Output bemessen; bei optionalen Feldern mehr
  Reserve einplanen.
- Niemals `StaticJsonDocument<1024>` in einen `char buffer[512]` serialisieren
  ohne korrekte Truncation-Prüfung (`len >= sizeof(buffer) - 1 → return false`).
- Einmalig genutzte große Dokumente (> 1 KB) als function-local statt
  file-scope `static` deklarieren.

## 21. Änderungen aus der Praxis

- Sicherheit: Secure Boot + Flash Encryption aktivieren, Debug-Schnittstellen deaktivieren.
- OTA: Partition-basierte Updates mit Anti-Rollback/Checksum.
- Speicher: Heap/Stack Überwachung & Static Buffer.
- ESP-IDF v5: WDT-API gebrochen — `esp_task_wdt_reconfigure` statt
  `esp_task_wdt_init` verwenden wenn Arduino-Framework WDT vorinitialisiert hat.
- JSON Discovery-Puffer: Serialisierungspuffer muss ausreichend Margin zum
  tatsächlichen JSON-Output haben, sonst schlägt HA-Discovery lautlos fehl.
