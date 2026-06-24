# Testkonzept — Pool Controller Firmware

> **Projekt:** Pool Controller (ESP32, C++17, PlatformIO, Arduino Framework)
> **Stand:** Juni 2026
> **Ziel:** 80 % Line Coverage im C++-Produktivcode (`src/`)

---

## 1. Ausgangslage

### 1.1 Codebasis (Produktivcode in `src/`)

| Kategorie | Dateien | Lines of Code |
|-----------|---------|--------------|
| Regel-Engine | RuleAuto, RuleBoost, RuleManu, RuleTimer, Rule.hpp | ~195 |
| Timer & Zeit | Timer.cpp, TimeClientHelper.cpp | ~353 |
| Temperatursensoren | DallasTemperatureNode, ESP32TemperatureNode | ~299 |
| Relay/Pumpen | RelayModuleNode | ~71 |
| Betriebsmodi | OperationModeNode | ~302 |
| Konfiguration | ConfigManager | ~209 |
| Zustandspersistenz | StateManager | ~94 |
| Web-Interface | WebPortal | ~1068 |
| MQTT | MqttPublisher | ~964 |
| Netzwerk | NetworkManager, WpsProvisioner | ~448 |
| OTA-Update | OtaUpdater | ~554 |
| System-Monitoring | SystemMonitor, DegradationManager | ~245 |
| Pool-Controller | PoolController | ~520 |
| NORVI-Hardware | NorviOledDisplay, NorviButtonHandler, StatusLed | ~1277 |
| Helper | Utils.hpp, Logger.cpp, Version.h, main.cpp | ~65 |
| **Total** | **25 .cpp / ~30 .hpp/.h** | **~6.664** |

### 1.2 Bestehende Test-Infrastruktur

- **Test-Framework:** Eigenbau (kein Google Test, kein Unity) — Makros `ASSERT_TRUE`, `ASSERT_EQ`, `ASSERT_STREQ`
- **Build-System:** CMake (native x86_64), ausgeführt im CI via `native-tests.yml`
- **Compiler-Flags:** `-g -O0 -fprofile-arcs -ftest-coverage -fsanitize=address`
- **Coverage-Tool:** gcov + lcov (CI), Auswertung als PR-Comment
- **Mocking:** Provisorische Mocks in `test/native/mocks/` (Arduino.h, WiFi.h, Preferences.h, etc.)
- **CI-Pipeline:** GitHub Actions (Ubuntu), PlatformIO zum Installieren von Dependencies, CMake-Build, lcov-Report

### 1.3 Aktuelle Testabdeckung

| Modul | Status | Lines | Coverage |
|-------|--------|-------|----------|
| Rule* (Timer/RuleTimer) | ✅ getestet | ~36 | ~80 % |
| MqttPublisher | ✅ getestet (wrapper) | 602 | ~66 % |
| WebPortal | ⚠️ teilweise (wrapper) | 610 | ~10 % |
| OtaUpdater | ⚠️ teilweise | 248 | ~6 % |
| DegradationManager | ❌ kompiliert, kaum getestet | 228 | ~0 % |
| StateManager | ❌ kompiliert, kaum getestet | 94 | ~0 % |
| SystemMonitor | ❌ kaum getestet | 17 | ~0 % |
| ConfigManager | ❌ nur Mock getestet | 209 | ~0 % |
| Timer/Timer.cpp | ❌ nicht kompiliert | 91 | 0 % |
| OperationModeNode | ❌ nicht kompiliert | 302 | 0 % |
| PoolController | ❌ nicht kompiliert | 520 | 0 % |
| Alle anderen `src/*.cpp` | ❌ nicht kompiliert | ~2.700 | 0 % |
| **Gesamt (geschätzt)** | | **~6.664** | **~12 %** |

### 1.4 Lückenanalyse

Um **80 % Line Coverage** zu erreichen, müssen ~5.330 der 6.664 Zeilen abgedeckt werden. Das bedeutet:

1. **Bestehende Tests ausbauen** — insbesondere bei WebPortal (10 % → 80 %), MqttPublisher (66 % → 85 %), OtaUpdater (6 % → 70 %)
2. **Fehlende Module aktivieren** — alle `src/*.cpp` müssen in den Build eingebunden werden
3. **Mocks vervollständigen** — für Hardware-Abhängigkeiten (DallasTemperature, OLED, Buttons)
4. **Neue Test-Suiten** für bisher 0 %-Module

---

## 2. Teststrategie

### 2.1 Testpyramide

```
         ┌──────────┐
         │   E2E    │ ← Hardware-in-the-Loop (Integration auf echter HW)
         │  (5 %)   │
        ┌┴──────────┴┐
        │Integration  │ ← Modulübergreifend (MQTT→Rule→Relay)
        │  (15 %)     │
       ┌┴─────────────┴┐
       │  Unit-Tests    │ ← Der Kern: native, isoliert, schnell
       │    (80 %)       │
       └────────────────┘
```

**Fokus:** Native Unit-Tests auf x86_64 (wie bisher) — schnell, deterministisch, mit Coverage-Analyse.

### 2.2 Test-Arten

| Testart | Beschreibung | Anteil | Ziel |
|---------|-------------|--------|------|
| **Unit-Tests (Logic)** | Regel-Engine, Timer-Berechnungen, Degradation, StateManager, Validierung | 50 % | Vollständige Pfadabdeckung |
| **Unit-Tests (I/O)** | MQTT-Payloads, Web-JSON-API, Config-Serialisierung | 20 % | Formate, Randfälle |
| **Unit-Tests (Hardware-Abstraktion)** | Sensorknoten, Relais, OLED — über Mocks | 10 % | Zustandsautomaten |
| **Integrationstests** | OperationModeNode→Rule→Timer, MQTT→OperationMode→Rule | 10 % | Zusammenspiel |
| **Sicherheitstests** | Rate-Limiting, Token, Passwort, Input-Validierung | 5 % | OWASP/IoT |
| **Speicher/Stabilität** | ASan, keine Leaks, Pufferüberläufe | 5 % | Laufzeitqualität |

### 2.3 Test-Framework

**Empfehlung: Bestehendes Custom-Framework beibehalten, aber erweitern.**

Begründung:
- Funktioniert bereits und liefert keine False Positives (53 Suites, 0 Failed)
- Google Test einzuführen brächte Abhängigkeiten und CMake-Änderungen
- Der bisherige Ansatz ist extrem leichtgewichtig und CI-kompatibel

**Empfohlene Erweiterungen:**

```cpp
// Zusätzliche Makros für Integrationstests
#define ASSERT_NEAR(a, b, eps)       // existiert bereits
#define ASSERT_GT(a, b)              // existiert bereits
#define ASSERT_LT(a, b)              // existiert bereits
#define ASSERT_THROWS(expr)          // optional (für die Zukunft)
#define TEST_SUITE(name)             // benannte Gruppe (aktuell manuell)
```

### 2.4 Mocking-Strategie

**Aktuelle Mocks** (vorhanden und nutzbar):
- `Arduino.h` — String, Serial, WiFi, Preferences, LittleFS, millis, delay, GPIO
- `ConfigManager.hpp` und `ConfigManager.cpp` — Settings, WiFi/MQTT/NTP-Konfig
- `NetworkManager.hpp` und `NetworkManager.cpp` — WiFi-Status, MQTT-Connected
- `DallasTemperatureNode.hpp` — Node-Klasse mit setTemperature/getTemperature
- `RelayModuleNode.hpp`, `OperationModeNode.hpp` — Node-Klassen
- `AsyncMqttClient.h` — mit Capture-Mechanismus

**Noch benötigte Mocks** (Priorität):
1. ✅ **DallasTemperature.h** — OneWire-Temperatursensor (vorhanden)
2. ✅ **ESP32TemperatureNode.hpp** — interner Temperatursensor (vorhanden)
3. 🔲 **Adafruit_SSD1306.h** — OLED-Display — *für NorviOledDisplay-Tests*
4. 🔲 **Adafruit_GFX.h** — Grafikbibliothek — *für OLED-Tests*
5. 🔲 **HTTPClient.h** — *für OTA/Web-Client-Tests*
6. 🔲 **time.h/TimeLib.h** — *erweitern für Timer-Tests*
7. 🔲 **Bounce2.h** — Entprellung — *für ButtonHandler-Tests*

---

## 3. Detaillierte Test-Spezifikation

### Phase 1 — Bestehende Tests ausbauen (Sofort)

| Modul | Aktuell | Ziel | Maßnahme |
|-------|---------|------|----------|
| **MqttPublisher** | 66 % / 602 Z. | 85 % | Fehlende Fehlerpfade: `publishDiscovery` bei nicht-verbundenem MQTT, leere Payloads, Timeouts |
| **WebPortal** | 10 % / 610 Z. | 80 % | `apiGetStatus`/`apiGetConfig` direkt testen (öffentliche Helper), `handleRoot`, `handleLogin`, Session-Handling, statische Dateien |
| **OtaUpdater** | 6 % / 248 Z. | 75 % | `begin()`, `loop()`, Fehler bei github-connect, ausgeschöpfter Flash, Abbruch |
| **DegradationManager** | ~0 % / 228 Z. | 90 % | Vollständige Zustandsmaschine (NORMAL→NO_WIFI→NO_SENSOR→CRITICAL), `forceSafeMode`, `unforceSafeMode`, Grenzfälle |
| **StateManager** | ~0 % / 94 Z. | 95 % | Save/Load für String, Float, Int, Bool; NaN- und Bereichsvalidierung; `clear()` |

### Phase 2 — Neue Test-Suiten für Kernmodule

| Modul | Lines | Ziel | Testfälle |
|-------|-------|------|-----------|
| **RuleAuto** | 78 | 90 % | loop mit gültigen/ungültigen Temperaturen, Solar an/aus Schwellwerte, Hysterese |
| **RuleBoost** | 58 | 90 % | 24h-Boost, getEffectiveRuntime, Mode-String |
| **RuleManu** | 16 | 95 % | Manueller Modus, kein Timer-Eingriff |
| **RuleTimer** | 23 | 90 % | Timer mit/ohne customEndMinutes, TimerSetting-Übernahme |
| **Timer.cpp** | 91 | 90 % | `getCurrentDateTime`, `getStartTime/EndTime`, `calculateEffectiveEndMinutes` (NaN, unter/über Threshold, Midnight-Crossing, Cap bei maxRuntime) |
| **OperationModeNode** | 302 | 75 % | Mode-Wechsel, `addRule`/`getRule`, `applyProperty` (alle Properties), Input-Validierung (parseFloat/parseInt), Persistenz (loadState/saveState), HA-Commands |

### Phase 3 — Neue Test-Suiten für Service-Module

| Modul | Lines | Ziel | Testfälle |
|-------|-------|------|-----------|
| **NetworkManager** | 245 | 70 % | WiFi-Status, RSSI, MQTT-Connection-State, Reconnect |
| **WpsProvisioner** | 203 | 60 % | WPS-Tastenabfrage, Provisioning-Ergebnis |
| **SystemMonitor** | 17 | 80 % | getFreeHeap, loop-Watchdog, min/max-Tracking |
| **StatusLed** | 122 | 70 % | LED-Statusanzeige, Blinkmuster |
| **TimeClientHelper** | 262 | 60 % | Zeitabfrage, Timezone-Wechsel, NTP-Sync, Degradation (GREEN→YELLOW→RED) |
| **ConfigManager (real)** | 209 | 80 % | JSON-Speichern/Laden, Reset, adminPasswordHash, OTA-Transition |
| **RelayModuleNode** | 71 | 90 % | Switch-GET/SET, State-Persistenz |

### Phase 4 — Hardware-nahe Module (eingeschränkt)

| Modul | Lines | Ziel | Testansatz |
|-------|-------|------|-----------|
| **NorviOledDisplay** | 1022 | 40 % | Display-Seiten, Text-Ausgabe über Mock (machbar), QR-Code (aufwändig) |
| **NorviButtonHandler** | 155 | 50 % | Tastendruck-Erkennung, Entprellung über Bounce2-Mock |
| **DallasTemperatureNode** | 262 | 50 % | Temperatur-Lesen simulieren, Fehlerfälle (Sensor nicht gefunden, CRC-Fehler) |
| **ESP32TemperatureNode** | 37 | 70 % | Interne Temperatur, Fehlerbehandlung |
| **PoolController** | 520 | 50 % | setup/loop-Zyklus, Subsystem-Initialisierung |
| **Nodes/Logger** | 18 | 80 % | Log-Level, Formatierung |

### Phase 5 — Integrationstests

| Test | Beschreibung |
|------|-------------|
| **MQTT → Rule Cycle** | HA-Command → MqttPublisher → OperationModeNode → Rule → Relay-Status per MQTT |
| **Degradation → Rule** | Sensorausfall → DegradationManager → RuleAuto verhält sich korrekt |
| **Config → Timer** | Config-Änderung → Timer-Berechnung ändert sich |
| **OTA Guard** | OtaUpdater prüft Platz → verweigert bei zu wenig Flash |

---

## 4. Coverage-Zielerrechnung

### 4.1 Ziel: 80 % Line Coverage

```
Ziel: 80 % von 6.664 = 5.331 abgedeckte Zeilen
Aktuell (geschätzt): ~800 Zeilen (12 %)
Fehlend: ~4.531 Zeilen
```

### 4.2 Aufwandsschätzung

| Phase | Module | Neue Tests | Zeilen | Coverage-Beitrag |
|-------|--------|-----------|--------|-----------------|
| P1: Bestehende ausbauen | Mqtt, Web, OTA, Degradation, State | ~800 Z. Testcode | ~2.600 | +39 % → ~51 % |
| P2: Kernmodule | Rules, Timer, OperationMode | ~600 Z. Testcode | ~800 | +12 % → ~63 % |
| P3: Service-Module | Network, WPS, System, Status, Time, Relay | ~500 Z. Testcode | ~900 | +14 % → ~77 % |
| P4: HW-nahe Module | OLED, Buttons, Dallas, ESP, PoolController | ~400 Z. Testcode | ~2.000 | +11 % → ~88 % |
| P5: Integration | Querschnitt | ~200 Z. Testcode | — | Absicherung |

### Realistisches Ziel nach P1+P2+P3: ~77 % Line Coverage

**Ziel von 80 % erreichbar mit:** P1 + P2 + P3 + Teilen von P4 (insb. ConfigManager, RelayModuleNode, DallasTemperatureNode).

### 4.3 Priorisierte Roadmap

```
Sprint 1 (P1):  Bestehende Tests ausbauen          → ~51 %
Sprint 2 (P2):  Rules + Timer + OperationModeNode   → ~63 %
Sprint 3 (P3a): Service: ConfigManager, Network,    → ~72 %
                Relay, SystemMonitor, StatusLed
Sprint 4 (P3b): TimeClientHelper, WpsProvisioner    → ~77 %
Sprint 5 (P4):  HW-nahe + PoolController            → ~85 %
Sprint 6 (P5):  Integrationstests + Coverage-Finetuning
```

---

## 5. Technische Umsetzung

### 5.1 CMake-Integration

Aktuell werden nur wenige `src/*.cpp` in den Test-Build eingebunden. **Ziel:** Alle Module einbinden.

```cmake
# CMakeLists.txt (Soll-Zustand)
set(PRODUCTION_SOURCES
  ${PROJ_ROOT}/src/ConfigManager.cpp
  ${PROJ_ROOT}/src/DallasTemperatureNode.cpp
  ${PROJ_ROOT}/src/DegradationManager.cpp
  ${PROJ_ROOT}/src/ESP32TemperatureNode.cpp
  ${PROJ_ROOT}/src/MqttPublisher.cpp          # via wrapper
  ${PROJ_ROOT}/src/NetworkManager.cpp
  ${PROJ_ROOT}/src/Nodes/Logger.cpp
  ${PROJ_ROOT}/src/OperationModeNode.cpp
  ${PROJ_ROOT}/src/OtaUpdater.cpp
  ${PROJ_ROOT}/src/RelayModuleNode.cpp
  ${PROJ_ROOT}/src/RuleAuto.cpp
  ${PROJ_ROOT}/src/RuleBoost.cpp
  ${PROJ_ROOT}/src/RuleManu.cpp
  ${PROJ_ROOT}/src/RuleTimer.cpp
  ${PROJ_ROOT}/src/StateManager.cpp
  ${PROJ_ROOT}/src/StatusLed.cpp
  ${PROJ_ROOT}/src/SystemMonitor.cpp
  ${PROJ_ROOT}/src/Timer.cpp
  ${PROJ_ROOT}/src/TimeClientHelper.cpp
  ${PROJ_ROOT}/src/WebPortal.cpp              # via wrapper
  ${PROJ_ROOT}/src/WpsProvisioner.cpp
)
```

**Wrapper-Ansatz beibehalten:** Für WebPortal und MqttPublisher, die direkt Node-Header
inkludieren (Mock-Layout weicht ab). Andere Module können direkt aus `src/` kompiliert werden.

### 5.2 Umgang mit Hardware-Abhängigkeiten

| Abhängigkeit | Mock-Status | Strategie |
|-------------|-------------|-----------|
| Arduino.h (GPIO, Serial, millis, delay) | ✅ Vorhanden | Keine Änderung nötig |
| Preferences (NVS) | ✅ Vorhanden | In-Memory-Map |
| OneWire / DallasTemperature | 🔲 Erweiterung | Temperaturwerte setzbar, Fehler simulierbar |
| Adafruit SSD1306 / GFX | 🔲 Neu | Print-Mock, keine Grafiklogik nötig |
| AsyncMqttClient | ✅ Vorhanden | Capture-Mechanismus |
| Bounce2 | 🔲 Neu | Button-Event-Simulation |
| WiFiClientSecure | 🔲 Erweiterung | Für OTA-Updater-Tests |

### 5.3 Test-Orchestrierung

```bash
# Build & Run (lokal)
cd test/native && cmake -B build && cmake --build build && ./build/test_runner

# Coverage (lokal — mit gcovr oder per Skript)
cd test/native/build && gcovr --root ../.. --filter 'src/' .
```

### 5.4 CI-Integration

Bereits vorhanden in `.github/workflows/native-tests.yml`:
- `pio pkg install` für Dependencies
- CMake configure + build
- ASan-aktivierter Testlauf
- lcov-Report als PR-Comment

**Erweiterung für Coverage-Gate:**

```yaml
# Coverage-Gate (z.B. als separater Job oder Step)
- name: Coverage Gate
  if: always()
  run: |
    total_cov=$(lcov --summary coverage.info 2>&1 | grep "lines" | awk '{print $2}' | tr -d '%')
    if (( $(echo "$total_cov < 70" | bc -l) )); then
      echo "❌ Coverage $total_cov% is below 70% threshold"
      exit 1
    fi
    echo "✅ Coverage $total_cov% meets threshold"
```

---

## 6. Qualitätskriterien

| Kriterium | Metrik | Grenzwert |
|-----------|--------|-----------|
| **Line Coverage** | lcov/gcov | ≥ 80 % |
| **Branch Coverage** | lcov/gcov | ≥ 60 % |
| **Test-Erfolg** | Alle Tests grün | 0 Failed |
| **ASan** | Address Sanitizer | 0 Leaks/Overflows |
| **Test-Laufzeit** | CI | < 2 Minuten |

### 6.1 Ausnahmen (von Coverage ausgenommen)

Folgende Code-Kategorien dürfen von der 80 %-Regel ausgenommen werden:
- **Hardware-Init-Code** in `main.cpp` (Setup-Code, der nur auf echter HW läuft)
- **QR-Code-Generierung** in NorviOledDisplay (komplexe Fremdbibliothek)
- **Debug-only Blöcke** (via `#ifdef DEBUG`), falls vorhanden
- **OTA-Download-Logik** in OtaUpdater (echter HTTPS-Download auf HW)

### 6.2 Messung der Coverage

Die Coverage wird **ausschließlich auf dem natives Test-Build** gemessen (`test/native/build`). Nur die Dateien aus `src/` zählen für das Ziel.

```bash
# lcov-Befehl (CI)
lcov --capture --directory test/native/build --output-file coverage.info \
  --rc lcov_branch_coverage=1 --ignore-errors unused
lcov --remove coverage.info \
  '/usr/*' '*/test/*' '*/lib/*' '*/mbedtls/*' '.pio/*' \
  --output-file coverage.info \
  --rc lcov_branch_coverage=1 --ignore-errors unused
lcov --list coverage.info --rc lcov_branch_coverage=1
```

---

## 7. Risiken und Gegenmaßnahmen

| Risiko | Wahrscheinlichkeit | Auswirkung | Maßnahme |
|--------|-------------------|------------|----------|
| Aufwändige Mocks für OLED/QR-Code | Mittel | Niedrig | OLED-Tests auf Text-Ausgabe beschränken; QR-Code vom Coverage-Ziel ausnehmen |
| Realer ConfigManager (JSON via LittleFS) schwer mockbar | Gering | Mittel | Dateibasierten Mock verwenden (temp file) — oder Integrationstest mit aufgesetztem JSON |
| PoolController::setup/loop zu stark gekoppelt | Hoch | Mittel | Verantwortlichkeiten trennen (Refactoring) oder große Integrationstests schreiben |
| Coverage unter 80 % wegen HW-lastiger Module | Mittel | Mittel | Priorisierte Roadmap (P1-P5) + Coverage-Ausnahmen für kritische Module |
| CI-Coverage-Gate blockiert PRs | Gering | Mittel | Threshold schrittweise erhöhen: 50 % → 65 % → 75 % → 80 % |

---

## 8. Nächste Schritte

1. **Sprint 1 starten:** WebPortal-Tests ausbauen (apiGetStatus/apiGetConfig direkt testbar machen)
2. **Mock-Bibliothek erweitern:** Fehlende Mocks für Phase 3+4
3. **CMakeLists.txt ausbauen:** Alle `src/*.cpp` in den Test-Build einbinden
4. **Test-Suites schreiben:** Nach Priorität (siehe Roadmap)
5. **Coverage-Gap analysieren:** Nach jedem Sprint lcov-Report prüfen
6. **Coverage-Gate in CI aktivieren:** Ab 70 % Mindest-Coverage

---

## Anhang: Datei-Metrik-Tabelle

| Source-Datei | LOC | Aktuell getestet? | Ziel % | Sprint |
|-------------|-----|-------------------|--------|--------|
| ConfigManager.cpp | 209 | ❌ (Mock-only) | 80 | P3a |
| DallasTemperatureNode.cpp | 262 | ❌ | 50 | P4 |
| DegradationManager.cpp | 228 | ⚠️ (0%) | 90 | P1 |
| ESP32TemperatureNode.cpp | 37 | ❌ | 70 | P4 |
| main.cpp | 45 | ❌ | 0* | — |
| MqttPublisher.cpp | 964 | ✅ (66%) | 85 | P1 |
| NetworkManager.cpp | 245 | ❌ (Mock-only) | 70 | P3a |
| Nodes/Logger.cpp | 18 | ❌ | 80 | P3a |
| NorviButtonHandler.cpp | 155 | ❌ | 50 | P4 |
| NorviOledDisplay.cpp | 1022 | ❌ | 40 | P4 |
| OperationModeNode.cpp | 302 | ❌ | 75 | P2 |
| OtaUpdater.cpp | 554 | ⚠️ (6%) | 75 | P1 |
| PoolController.cpp | 520 | ❌ | 50 | P4 |
| RelayModuleNode.cpp | 71 | ❌ | 90 | P3a |
| RuleAuto.cpp | 78 | ❌ | 90 | P2 |
| RuleBoost.cpp | 58 | ❌ | 90 | P2 |
| RuleManu.cpp | 16 | ❌ | 95 | P2 |
| RuleTimer.cpp | 23 | ❌ | 90 | P2 |
| StateManager.cpp | 94 | ⚠️ (0%) | 95 | P1 |
| StatusLed.cpp | 122 | ❌ | 70 | P3a |
| SystemMonitor.cpp | 17 | ⚠️ (0%) | 80 | P3a |
| Timer.cpp | 91 | ❌ | 90 | P2 |
| TimeClientHelper.cpp | 262 | ❌ | 60 | P3b |
| WebPortal.cpp | 1068 | ⚠️ (10%) | 80 | P1 |
| WpsProvisioner.cpp | 203 | ❌ | 60 | P3b |

*\* main.cpp wird vom Coverage-Ziel ausgenommen (HW-Init-Code)*
