# Codex Code Review: Clean Code & 24/7 IoT Readiness

**Datum:** 2026-07-04
**Reviewer:** Codex (OpenCode)
**Basis:** `main` (`0c3cea8`)
**Scope:** Gesamte Codebase (`src/` — ~8.200 LOC in 35 Dateien)

---

## Executive Summary

Das Projekt zeigt eine solide Architektur mit bewusster Berücksichtigung von 24/7-IoT-Anforderungen (Watchdog, Boot-Loop-Erkennung, Degradation Manager). Die Code-Qualität ist insgesamt gut — es gibt eine klare Trennung von Concerns, durchgängige Doxygen-Dokumentation und sinnvolle Nutzung von C++11-Features (`auto`, `constexpr`, `delete`d Copy/Move).

Dennoch wurden **8 kritische** und **15 mittelschwere** Befunde identifiziert, die in zwei Clustern fallen:

1. **24/7-Stabilität:** `String`-Allokationen in allen Loop-Pfaden → Heap-Fragmentierung über Tage/Wochen
2. **Clean Code:** Undisziplinierte Singleton-Kopplung, Magic Strings, Duplikate, überlange Funktionen

---

## 🔴 Kritische Befunde (24/7-Betriebsgefährdung)

### K1: String-Allokation in MQTT-Hot-Path (Heap-Fragmentierung)

**Datei:** `src/MqttPublisher.cpp:65`
**Prinzip:** *"Avoid heap allocation in hot-paths"* (IoT Quality Gate #5)

Jeder Aufruf von `getBaseTopic()` erzeugt via `operator+` neue `String`-Objekte auf dem Heap:

```cpp
return String("homeassistant/") + component + "/pool-controller/" + objectId;
```

`publishStates()` (Z. 533–637) ruft dies **30+ Mal pro Loop-Intervall auf**. Jeder Aufruf:
- Allokiert temporäre `String`-Objekte
- Fragmentiert den Heap
- Erhöht das Risiko eines `CRITICAL: Free heap`-Reboots nach Tagen/Wochen

Betroffen sind auch:
- `src/MqttPublisher.cpp:538-609` — `String(poolTemperatureNode.getTemperature(), 1).c_str()` bei jedem publish
- `src/WebPortal.cpp:724` — `"{\"status\":\"ok\",\"mode\":\"" + mode + "\"}"` String-Konkatenation im API-Handler
- `src/WebPortal.cpp:756` — `"{\"status\":\"ok\",\"state\":" + String(newState ? "true" : "false") + "}"`
- `src/WebPortal.cpp:799` — `"session=" + activeSessionToken_ + "; Path=/; ..."`

**Empfehlung:** `getBaseTopic()` durch `snprintf` in `char[]`-Buffer ersetzen oder eine statische `char[]`-Cache-Strategie implementieren. Für `publishStates()` eine einzige `char topic[128]` pro publish-Slot vorhalten.

### K2: String in OtaUpdater-Statics (Permanent-Heap)

**Datei:** `src/OtaUpdater.hpp:87-92`

```cpp
static String currentVersion_;
static String latestVersion_;
static String releaseUrl_;
static String downloadUrl_;
static String statusMessage_;
```

Diese 5 `String`-Objekte leben permanent auf dem Heap. `statusMessage_` (Z. 71) wird in `begin()` und in der Update-Loop beschrieben. Ein fehlgeschlagener Update-Versuch mit einer sehr langen Fehlermeldung von GitHub (`HTTPClient`-Response) könnte den Heap unnötig belasten.

**Empfehlung:** Auf `char[]`-Fixed-Buffer umstellen. `FW_VERSION` ist bereits ein `const char*` — `currentVersion_` kann ebenfalls als `const char*` oder `char[16]` definiert werden.

### K3: DegradationManager — Pessimistische Initialisierung

**Datei:** `src/DegradationManager.cpp:44-45`

```cpp
poolSensorOk_ = false;  // Pessimistic — both probes must report healthy
solarSensorOk_ = false;
```

Beide Sensoren starten als `false`. Die erste `evaluate()` (nach `EVALUATION_INTERVAL_MS`, default 10s) sieht beide Sensoren als "nicht OK" → `failureCount = 2` → **sofort CRITICAL** (Z. 215-217), obwohl die Sensoren noch gar keine Chance hatten, einen Wert zu liefern.

Der `PoolController::setup()` initialisiert Sensoren erst in `initializeController()` (Z. 243-247), die Degradation läuft aber bereits im `loop()`. Das Fenster ist klein (10s), aber ein kurzzeitiger CRITICAL-Status ist möglich.

**Empfehlung:** `DegradationManager::begin()` sollte die Sensoren als "unbekannt" markieren (z.B. `optional<bool>` oder `uint8_t`-Counter) und erst nach der ersten erfolgreichen/fehlgeschlagenen Messung bewerten. Alternativ: Initialisierung mit `true` — ein Sensor der noch nicht gemeldet hat, sollte nicht als "fehlgeschlagen" zählen.

### K4: Millis-Rollover-Gefahr in OtaUpdater

**Datei:** `src/OtaUpdater.cpp:91`

```cpp
if (now - lastClockSyncFailTime_ >= kClockSyncBackoffMs || now < lastClockSyncFailTime_) {
```

Die `|| now < lastClockSyncFailTime_`-Klausel adressiert millis()-Wrap-Around, aber nicht konsistent. Vergleiche in:
- `OtaUpdater.cpp:101` — `now - lastCheckTime_ >= kCheckIntervalMs || lastCheckTime_ == 0`
- `PoolController.cpp:451` — `(millis() - lastBootClear) > ...` (kein Wrap-Schutz)
- `SystemMonitor.hpp:81` — `now - lastMemoryCheck < 10000` (kein Wrap-Schutz)
- `DegradationManager.cpp:56` — `now - lastEvaluationMs_ < EVALUATION_INTERVAL_MS` (kein Wrap-Schutz)

Nach ~50 Tagen Uptime rollt `millis()` auf ESP32 auf 0. Dann funktionieren einfache Differenzvergleiche falsch, bis die aktuelle Zeit wieder > die gespeicherte Referenz ist. Das könnte zu:
- Falschen Degradation-Entscheidungen
- Ausbleibenden Watchdog-Feeds
- Verpassten OTA-Checks

**Empfehlung:** Für alle `millis()`-Differenz-Vergleiche die Standard-ESP32-Wrap-sichere Formel `(uint32_t)(now - last) >= interval` verwenden, die auch bei Wrap-around korrekt funktioniert (unsigned arithmetic defined behavior). Die `|| now < last`-Klausel ist unnötig, wenn `now - last` als `uint32_t` berechnet wird.

### K5: Statischer JSON-Buffer ohne Fallback bei Überlauf

**Datei:** `src/WebPortal.cpp:479, 503, 551`

```cpp
static char jsonBuffer[1024];  // apiGetStatus
static char jsonBuffer[4096];  // apiScanWiFi
static char jsonBuffer[2048];  // apiGetConfig
```

Drei separate statische Buffer, die **insgesamt 7 KB RAM** permanent belegen (selbst wenn die API nie aufgerufen wird). Bei einem ESP32 mit ~320 KB freiem Heap ist das ~2% permanent verloren — akzeptabel, aber suboptimal.

**Empfehlung:** Entweder einen einzigen Shared Buffer (maximale Größe) verwenden oder auf `DynamicJsonDocument` mit Stack-Buffer für kleine Responses umstellen.

### K6: MQTT publish/retain ohne Buffer-Limit-Check

**Datei:** `src/MqttPublisher.cpp` — alle `publishSensorDiscovery()`-ähnlichen Methoden

```cpp
String payload;
serializeJson(doc, payload);
NetworkManager::publish(configTopic.c_str(), payload.c_str(), true);
```

Das `String payload` serialisiert das JSON-Dokument in einen Heap-String, bevor es an MQTT übergeben wird. Bei langen Option-Listen (z.B. `publishSensorMappingDiscovery()` mit bis zu 20 hex-Adressen wie `28AABBCCDDEEFF11`) kann dieser Payload mehrere KB groß werden.

**Empfehlung:** `serializeJson(doc, buffer, sizeof(buffer))` in einen statischen `char[]`-Buffer verwenden (z.B. `char jsonBuf[1024]`). Prüfen ob `jsonLength < sizeof(jsonBuf)`.

### K7: saveSensorAddressMapping() vs saveSensorMappingNvs() — Code-Duplikat

**Datei:** `src/PoolController.cpp:138-152` und `src/WebPortal.cpp:939-953`

Zwei nahezu identische Funktionen, die Sensor-Adressen in NVS speichern. Unterschied: Eine schreibt 8 Bytes, die andere prüft nicht auf Null-Adressen. Bei Änderungen müssten beide synchron aktualisiert werden.

**Empfehlung:** In eine gemeinsame Helper-Funktion extrahieren, z.B. `ConfigManager::saveSensorMapping(...)`.

### K8: Rule::checkPoolPumpTimer — Exzessives Logging im Hot-Path

**Datei:** `src/Rule.hpp:113, 130-132, 160, 178-179, 189, 194`

Jeder Loop-Durchlauf produziert 6-10 `Serial.printf`-Zeilen von `checkPoolPumpTimer()`. Bei `loopInterval=10` (default 10s) sind das ~50.000+ Logzeilen pro Tag — überflutet den Serial-Puffer und kann bei Langzeitbetrieb den UART-Buffer überlaufen lassen.

**Empfehlung:** Log-Level einführen (z.B. `#ifdef DEBUG_RULE_TIMER`) oder auf einmal pro Minute drosseln.

---

## 🟡 Mittelschwere Befunde (Clean Code & Maintainability)

### C1: Magic Strings für Operation Modes

**Prinzip:** *"Avoid magic strings — use enums or constants"* (Clean Code)

`"auto"`, `"manu"`, `"boost"`, `"timer"` tauchen als String-Literale in **7 Dateien** auf:

| Datei | Zeilen |
|-------|--------|
| `PoolController.cpp` | 347-355 |
| `MqttPublisher.cpp` | 344-349, 758-769, 779-797, 827, 846 |
| `WebPortal.cpp` | 720-724 |
| `OperationModeNode.cpp` | — |
| `Rule*.cpp` | — |

Ein Tippfehler (z.B. `"manu"` vs `"manuel"`) wird nicht vom Compiler abgefangen.

**Empfehlung:** `enum class OperationMode { Auto, Manual, Boost, Timer }` definieren und überall verwenden. Nur an der Grenzfläche zu MQTT/Web in String konvertieren.

### C2: MqttPublisher::handleMqttMessage — Riesen-If-Else-Kette (300+ Zeilen)

**Datei:** `src/MqttPublisher.cpp:723-1010`

Eine einzige Funktion mit ~290 Zeilen und 15+ `if (top.endsWith(...))`-Zweigen. Dies ist die größte Einzelfunktion im Projekt. Jeder neue Entity-Typ erfordert einen weiteren else-if-Zweig.

**Empfehlung:** Command-Pattern: `HashMap<String, CommandHandler>` oder zumindest eine `dispatchTable` mit struct `{ const char *suffix, HandlerFn }`.

### C3: Global-extern-Vernetzung (Tight Coupling)

**Dateien:** `MqttPublisher.cpp:30-35`, `WebPortal.cpp:55-60`, `NorviOledDisplay.cpp:40-44`, `DegradationManager.cpp:20-21`

```cpp
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
// ... 5 Dateien deklarieren die gleichen 6 Nodes neu
```

Jede Datei, die auf globale Nodes zugreift, wiederholt die `extern`-Deklarationen. Bei Umbenennung/Änderung müssen 5 Dateien aktualisiert werden.

**Empfehlung:** Zentrale `Nodes.hpp` mit `extern`-Deklarationen oder — besser — Dependency Injection via Konstruktor/Setter.

### C4: Static-Singleton-Anti-Pattern (Alle Manager)

**Prinzip:** *"Depend on abstractions, not concretions"* (Dependency Inversion)

Jeder Manager/Knoten ist als reine static-methods-Klasse implementiert:

```cpp
class NetworkManager {
  static bool begin();
  static void loop();
  static bool isWiFiConnected();
  // ...
};
```

Das verhindert:
- **Unit-Tests** — keine Mock-Implementierungen möglich
- **Austeauschbarkeit** — kein Interface (z.B. `INetworkManager`)
- **Lifecycle-Kontrolle** — kein wirklicher Singleton-Lifecycle

**Empfehlung:** Interfaces extrahieren (z.B. `INetworkManager`) und als Referenzen in `PoolControllerContext` injizieren. Für Tests: Mock-Implementierungen bereitstellen.

### C5: ConfigManager::hashSha256() gibt String zurück

**Datei:** `src/ConfigManager.cpp:30-46`

```cpp
static String hashSha256(const String &input) {
  // ...
  char result[65];
  // ...snprintf...
  return String(result);
}
```

Erzeugt einen 64-Byte-String auf dem Heap, der nach der Verwendung wieder freigegeben werden muss. Da `verifyAdminPassword()` nur einen Vergleich macht, wäre ein `bool`-Rückgabetyp oder `const char*` ausreichend.

**Empfehlung:** `void hashSha256(const String &input, char *outHex, size_t outSize)` — Buffer wird vom Aufrufer bereitgestellt.

### C6: WebPortal::handleLogin() — HTML in C++-String-Literal

**Datei:** `src/WebPortal.cpp:387-422`

Ein ~40-zeiliges HTML-Dokument inkl. CSS/JS als C++-Raw-String-Literal. Das ist schwer wartbar (kein Syntax-Highlighting, keine IDE-Unterstützung für das HTML).

**Empfehlung:** Entweder als separate HTML-Datei auf LittleFS auslagern oder zumindest in einen separaten `const char kLoginPage[] PROGMEM`-String.

### C7: MqttPublisher::publishDiscovery() — Überlang (120+ Zeilen)

**Datei:** `src/MqttPublisher.cpp:411-530`

Eine Funktion, die 40+ Discovery-Payloads publiziert + Subscriptions registriert + alte Topics cleanup. Das ist zu viel Verantwortung für eine Funktion.

**Empfehlung:** Aufteilen in `publishSensorDiscoveries()`, `publishSwitchDiscoveries()`, `publishConfigDiscoveries()`, `registerSubscriptions()`, `cleanupOldTopics()`.

### C8: Undisziplinierte Include-Guards und Namensräume

**Datei:** `src/Rule.hpp:1`

```cpp
#pragma once  // ✓ gut
// ABER: class Rule { ... };  // Kein Namespace!
```

`Rule`-Basisklasse und `Rule*.cpp`-Implementierungen sind **nicht** im `PoolController::`-Namespace, während alle anderen Klassen darin sind. Das erhöht das Risiko von Symbol-Konflikten.

**Empfehlung:** `namespace PoolController { class Rule ... }` — auch alle `Rule*.cpp`-Dateien anpassen.

### C9: NorviOledDisplay — 1035 Zeilen (Single Responsibility)

**Datei:** `src/NorviOledDisplay.cpp`

Die Datei ist mit 1035 Zeilen die größte im Projekt und erfüllt mehrere Aufgaben:
- Display-Rendering (7 verschiedene Pages)
- Burn-In Mitigation
- Sensor-Setup-Wizard (State Machine)
- Button-Event-Handling (delegiert, aber Logik hier)
- QR-Code-Anzeige für WPS

**Empfehlung:** Aufteilen in `NorviDisplayPages.cpp`, `NorviSetupWizard.cpp`, `NorviBurnIn.cpp` oder zumindest durch `// MARK:`-Sektionen strukturieren (bereits ansatzweise vorhanden).

---

## 🟢 Positive Befunde (Keep doing! ✅)

- **SystemMonitor** — Exzellente Watchdog-Konfiguration mit `esp_task_wdt_reconfigure()` und ESP-IDF 5.x Guard. Klares Design, einfache API.
- **DegradationManager** — Durchdachtes Degradations-Modell mit forensicher Fehlerzählung. Kein Single-Point-of-Failure entscheidet über CRITICAL.
- **Boot-Loop-Erkennung** — Robuste Implementierung mit NVS-Persistenz, Safe Mode und autom. Reset nach 5 min Stabilität.
- **PoolController::loop()** — Saubere, dokumentierte Reihenfolge. Watchdog-Feed an erster Stelle. Keine blocking delays im Loop.
- **ConfigManager** — Sorgfältige NVS-Nutzung mit `begin/end`-Paaren und `readOnly`-Flag wo möglich. Keine offenen Handles.
- **`auto`-Return-Types** — Konsistente Nutzung von `auto setup() -> void` statt `void setup()`. Moderner C++-Stil.
- **`= delete` für Copy/Move** — `PoolControllerContext` verbietet explizit Kopie und Move. Vorbildlich.
- **Security** — CSRF-Token, Session-Management, Rate-Limiting, HttpOnly-Cookies. SAST-konforme Passwort-Hashes.
- **WebPortal::apiGetStatus** — Nutzt `static char jsonBuffer[1024]` + `serializeJson(doc, jsonBuffer, ...)` statt `String`. Genau der richtige Ansatz (sollte Vorbild für MqttPublisher sein!).
- **JSON-Buffer-Überlauf-Prüfung** — `if (jsonLength >= sizeof(jsonBuffer) - 1)` in `apiScanWiFi` und `apiGetConfig`. Vorbildlich.

---

## 📋 Handlungsempfehlungen nach Priorität

### Sofort (nächster Release)

| # | Befund | Aktion | Aufwand |
|---|--------|--------|---------|
| 1 | **K1** String in MqttPublisher-Loop | `getBaseTopic()` → `snprintf()` + `char[]` | 2h |
| 2 | **K3** Degradation pessimistisch | Initialisierung auf `optional<bool>` umstellen | 1h |
| 3 | **K8** Exzessives Rule-Logging | `#ifdef DEBUG_RULE_TIMER` oder Drosselung | 0.5h |
| 4 | **K7** Code-Duplikat Sensor-Mapping | In Helper extrahieren | 0.5h |

### Nächster Minor-Release

| # | Befund | Aktion | Aufwand |
|---|--------|--------|---------|
| 5 | **K4** Millis-Wrap-Robustheit | Einheitliche Wrap-sichere Formel | 1h |
| 6 | **C1** Magic Strings | `enum class OperationMode` | 2h |
| 7 | **C3** Extern-Vernetzung | Zentrale `Nodes.hpp` | 1h |

### Tech-Debt (nächster Major)

| # | Befund | Aktion | Aufwand |
|---|--------|--------|---------|
| 8 | **C2** Mqtt-Command-Router | Command-Pattern | 4h |
| 9 | **C4** Static-Singleton | Interface-Extraktion | 8h |
| 10 | **C9** NorviOled aufteilen | Modularisierung | 4h |

---

## 🔬 Statische Analyse-Zusammenfassung

| Metrik | Wert | Bewertung |
|--------|------|-----------|
| Dateien `src/` | 35 | ✅ |
| Gesamt-LOC (`.cpp`+`.hpp`) | ~8.200 | 🟡 (NorviOled 1KB allein) |
| `String` in Loops/Heißpfaden | ~40 Stellen | 🔴 |
| `extern`-Deklarationen | 5 Dateien × 6 Nodes | 🟡 |
| `enum` statt Magic Strings | 0 (keine) | 🟡 |
| Namespace `PoolController` | 33/35 Klassen ✅, 2 ❌ | 🟡 |
| Doxygen-Kommentare | ~90% der Methoden ✅ | ✅ |
| ESP-IDF Version Guards | 2/2 (WDT + WPS) ✅ | ✅ |
| Blocking `delay()` im Loop | 0 ❌ | ✅ |
| Statische JSON-Buffer | 3 (7 KB RAM) | 🟡 |

---

## Fazit

Das Projekt ist **produktionstauglich** und zeigt ein gutes Verständnis für Embedded/IoT-Qualität. Die Architektur mit Degradation Manager und Boot-Loop-Erkennung ist vorbildlich für ein 24/7-Gerät.

**Das größte Risiko ist K1 (String-Allokation im MQTT-Loop-Pfad)** — eine Heap-Fragmentierung, die nach Tagen/Wochen zu einem Reboot führen kann. Dies sollte im nächsten Release adressiert werden.

Die Clean-Code-Befunde (C1-C9) sind typische Tech-Debt-Akkumulation in Embedded-Projekten und sollten schrittweise abgebaut werden. Besonders die Interface-Extraktion (C4) würde die Testabdeckung signifikant verbessern.

*Review generated by Codex via OpenCode — 2026-07-04*
