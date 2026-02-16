# AGENTS.md

Zweck: Regeln und Standards für einen Coding-Agent, der PlatformIO-basierten IoT-Firmware für ESP8266/ESP32 entwickelt, refaktoriert, analysiert und stabilisiert. Fokus: Architektur, Linting, Speicher- und Ressourcenmanagement, Sicherheit, 24/7-Betrieb, Wartbarkeit, Tests, Releases und OTA.

## 1. Arbeitsweise

Der Agent:

- liefert minimal-invasive Änderungen mit Begründung, Impact-Analyse (RAM/Flash/CPU), Risiken, Rollback-Hinweisen.
- verhindert blockierende Patterns im Laufzeit-Code (long `delay()`, busy waits, blockierende Netzwerk-Calls im Main-Loop).
- vermeidet unsichere dynamische Allokationen in Hot-Paths und ungeprüfte Heap-Nutzung.
- folgt einem CI-regelbasierten Prozess (Build + Tests + Lint).

## 2. Zielplattformen & Framework

- Plattform: ESP8266, ESP32 (inkl. Varianten wie S3, C3); Frameworks: Arduino oder ESP-IDF.
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

Commit Messages müssen dem Conventional Commits-Standard folgen: `<type>[optional scope]: <description>`
Commit Types umfassen mindestens:

- `feat` für neue Funktionen
- `fix` für Fehlerbehebungen
- `docs` für Änderungen an Dokumentation
- `style` für Formatierung/Code Style
- `refactor` für Code-Umstrukturierungen ohne funktionale Änderung
- `perf` für Performance-Optimierungen
- `test` für Test-Änderungen
- `chore` für Wartung/Tooling/Build-Änderungen

Commit Message Body kann Motivation und Kontext enthalten. Breaking Changes müssen mit `BREAKING CHANGE:` im Footer markiert werden.
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

ESP8266:

- Single-Core, kooperatives Scheduling.
- Nicht blockierende Calls, Loop kurz halten.

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

## 19. Änderungen aus der Praxis

- Sicherheit: Secure Boot + Flash Encryption aktivieren, Debug-Schnittstellen deaktivieren. :contentReference[oaicite:4]{index=4}
- OTA: Partition-basierte Updates mit Anti-Rollback/Checksum. :contentReference[oaicite:5]{index=5}
- Speicher: Heap/Stack Überwachung & Static Buffer. :contentReference[oaicite:6]{index=6}
