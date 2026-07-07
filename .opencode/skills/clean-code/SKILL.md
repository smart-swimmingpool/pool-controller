---
name: clean-code
description: "Clean Code checklist for the pool-controller C++ codebase — derived from Codex Review PR #143 findings. Systematic review categories for heap safety, duplication, debris, logging hygiene, boot sequencing, naming, const correctness, and testability. 🇩🇪 Deutsche Trigger: Sauberer Code, Code-Review, Clean Code Checkliste, Heap-Sicherheit, String-Verbot, Duplikate, Toter Code, Logging, Boot-Reihenfolge, const correctness, Testbarkeit, Entkopplung, Lesbarkeit, Wartbarkeit."
keywords:
  - clean code
  - sauberer code
  - code review checkliste
  - code review checklist
  - heap sicherheit
  - heap safety
  - string verbot
  - no string
  - duplikate vermeiden
  - code duplication
  - toter code
  - dead code
  - debris
  - logging hygiene
  - debug output
  - boot sequenz
  - boot sequence
  - degradation
  - const correctness
  - testbarkeit
  - testability
  - entkopplung
  - decoupling
  - lesbarkeit
  - readability
  - wartbarkeit
  - maintainability
  - codestandards
  - coding standards
  - naming conventions
  - early return
  - single responsibility
  - small functions
  - include hygiene
  - pre-merge review
---

# Clean Code — Pool Controller

Systematische Clean Code Review-Kategorien abgeleitet aus dem [Codex Review PR #143](https://github.com/smart-swimmingpool/pool-controller/pull/143).
Jede Kategorie beschreibt ein konkretes, prüfbares Muster — keine abstrakten Prinzipien.

Jeder PR durchläuft **alle 8 Kategorien** vor dem Merge.

> **🔍 Code Search**: `semble search "String" src/` → Heap-Verstöße finden.
> `semble search "Preferences.begin" src/` → mögliche NVS-Duplikate.
> `semble search "Serial.print" src/` → Debug-Output in Produktionscode.

---

## K1 — Heap Safety (String-Verbot in Hot Paths)

Heap-Allokationen fragmentieren den ESP32-Speicher und führen nach Tagen/Wochen zum OOM-Crash.
`String`, `std::string` und dynamische `new`-Allokationen sind nur in **Setup-/einmaligem Code** erlaubt.

### Prüfliste

| Check                                              | Beschreibung                                                    | Konsequenz                                                         |
| -------------------------------------------------- | --------------------------------------------------------------- | ------------------------------------------------------------------ |
| `String` im Loop/zyklischen Code                   | String-Konkatenation in `publishStates()`, `loop()`, `check*()` | **Blocked** — char-Buffer verwenden                                |
| `getBaseTopic()` kehrt `String` zurück             | Rückgabe als `String` zwingt Caller zur Heap-Allokation         | **Blocked** — `void getBaseTopic(char*, size_t, ...)`              |
| `serializeJson(doc, payload)` mit `String payload` | ArduinoJson serialisiert in `String` → Heap                     | **Blocked** — `serializeJson(doc, payloadBuf, sizeof(payloadBuf))` |
| `String(poolTemp, 1)` für MQTT-Publish             | temporäre String-Objekte im Hot Path                            | **Blocked** — `snprintf(valBuf, sizeof(valBuf), "%.1f", temp)`     |
| `String` als Funktionsparameter                    | Kopie bei jedem Aufruf                                          | `const String&` oder `const char*`                                 |
| `str + str2` Konkatenation                         | Erzeugt temporäre String-Objekte                                | char-Array + `strlcat`/`snprintf`                                  |

### Beispiel: Gut vs. Schlecht

```cpp
// ❌ BAD — Heap-Allokation im Hot Path
String MqttPublisher::getBaseTopic(const char *component, const char *objectId) {
  return String("homeassistant/") + component + "/pool-controller/" + objectId;
}
// → 3+ temporäre String-Objekte pro Aufruf, ~80ns + Heap-Fragmentierung

// ✅ GOOD — Stack-only
void MqttPublisher::getBaseTopic(char *buf, size_t bufSize, const char *component, const char *objectId) {
  snprintf(buf, bufSize, "homeassistant/%s/pool-controller/%s", component, objectId);
}
```

```cpp
// ❌ BAD — String als MQTT-Payload
String payload;
serializeJson(doc, payload);
NetworkManager::publish(topic, payload.c_str(), true);

// ✅ GOOD — pre-allocated char buffer
char payloadBuf[1024];
serializeJson(doc, payloadBuf, sizeof(payloadBuf));
NetworkManager::publish(topic, payloadBuf, true);
```

### Ausnahmen (akzeptabel)

- `deviceId_` im Konstruktor/`begin()` einmal gesetzt (nicht in Hot Path)
- `OtaUpdater::downloadUrl_` nur bei OTA-Update (selten)
- `ConfigManager` String-Member — nur bei `load()`/`save()` genutzt

---

## K2 — Code Duplication (DRY)

Duplizierte Logik = duplizierte Bugs. Jede Geschäftslogik existiert genau einmal.

### Prüfliste

| Muster                                                             | Fundort                                            | Fix                                        |
| ------------------------------------------------------------------ | -------------------------------------------------- | ------------------------------------------ |
| Identische NVS-Zugriffe (`Preferences::begin("ds18b20")`)          | PoolController.cpp + WebPortal.cpp + ConfigManager | In ConfigManager zentralisieren            |
| Gleiche Discovery-Payload-Struktur                                 | Alle `publish*Discovery()` Methoden                | Template-Methode oder Helper               |
| Wiederholte `getBaseTopic(...) + "/state"`                         | publishStates()                                    | `TopicBuilder`-Helper (anonymes namespace) |
| Gleiche Sensor-Mapping-Konvertierung (hexToString/addressToString) | PoolController.cpp + WebPortal.cpp                 | In ConfigManager oder Utility              |

### Beispiel

```cpp
// ❌ BAD — 3x identischer NVS-Zugriff
// PoolController.cpp:
static void saveSensorAddressMapping(...) { /* Preferences::begin("ds18b20") ... */ }
// WebPortal.cpp:
static void saveSensorMappingNvs(...) { /* Preferences::begin("ds18b20") ... */ }

// ✅ GOOD — einmal in ConfigManager
ConfigManager::saveSensorMapping(solarAddr, poolAddr);
```

---

## K3 — Debris (Dead Code, Comments, Checks)

Toter Code, auskommentierter Code, sinnlose Kommentare und redundante Sicherheitschecks
sammeln sich an und täuschen Wartende.

### Prüfliste

| Muster                       | Beispiel                                                                           | Konsequenz                                  |
| ---------------------------- | ---------------------------------------------------------------------------------- | ------------------------------------------- |
| Auskommentierter Code        | `// int x = 5;`                                                                    | Entfernen                                   |
| Redundanter Sicherheitscheck | <code>\|\| now &lt; lastClockSyncFailTime\_</code> (unsigned wrap bereits korrekt) | Entfernen                                   |
| Sinnentleerte Kommentare     | `// Loop function` über `void loop()`                                              | Entfernen                                   |
| `TODO:`/`FIXME:` ohne Issue  | `// TODO: fix this`                                                                | Issue erstellen oder fixen                  |
| Unused includes              | `#include <esp_now.h>` (ungenutzt)                                                 | Entfernen                                   |
| Unused Variablen             | `uint8_t devCount = getCount();` (nie gelesen)                                     | Entfernen oder `(void)`                     |
| Leere catch-Blöcke           | `catch (...) {}`                                                                   | Logging ergänzen oder entfernen             |
| Tote else-Zweige             | `if (x) return; else { ... }`                                                      | `else` entfernen, `{ ... }` eine Ebene raus |
| Tote Funktionsparameter (YAGNI) | Konstruktor-/Funktions-Parameter, den kein Caller übergibt → nur Defaultwert existiert | Parameter entfernen; wenn nötig via Setter setzen oder hart codieren |

### Beispiel

```cpp
// ❌ BAD — redundant (unsigned long subtract is safe in C++)
if (now - lastClockSyncFailTime_ >= kClockSyncBackoffMs || now < lastClockSyncFailTime_)

// ✅ GOOD — C++ Standard garantiert modulo arithmetic
if (now - lastClockSyncFailTime_ >= kClockSyncBackoffMs)
```

---

## K4 — Logging Hygiene

Serial-Output ist wertvolles Debugging-Tool, aber unkontrollierte Serial-Ausgaben
produzieren Tonnen von Logs, machen wichtige Meldungen unleserbar und bremsen.

### Prüfliste

| Muster                                      | Bewertung                                                 |
| ------------------------------------------- | --------------------------------------------------------- |
| `Serial.println()` im 1s-Loop-Takt          | `#ifdef DEBUG_xxx` guard oder Rate-Limiting               |
| `Serial.printf()` für jeden Funktionsaufruf | Nur mit DEBUG-Präprozessor-Flag                           |
| Produktions-Logs                            | Immer aktiv (Statusänderungen, Fehler, Config-Änderungen) |
| Sensor-Rohdaten                             | Nie aktiv — nur mit DEBUG                                 |
| Funktions-Eintritt/Austritt                 | `#ifdef DEBUG_xxx` guard                                  |
| Persönliche Debug-Ausgaben (`//MM` etc.)    | Vor Commit entfernen                                      |

### Konvention

```cpp
// Im Header (Standard: AUS):
// #define DEBUG_RULE_TIMER   // uncomment for timer debugging

// Im Code:
#ifdef DEBUG_RULE_TIMER
  Serial.println("↕  checkPoolPumpTimer");
#endif
```

---

## K5 — Boot Sequence & Degradation

Ein System, das beim Booten sofort Degradation meldet, bevor es überhaupt eine Messung
hat, ist kein zuverlässiges System: **Alarmmüdigkeit** zerstört den Wert des Degradationssystems.

### Prüfliste

| Muster                                     | Problem                              | Fix                                       |
| ------------------------------------------ | ------------------------------------ | ----------------------------------------- |
| Sensor-Status = `false` vor erster Messung | Falscher CRITICAL-Alarm beim Boot    | `sensorsEverReported_` Flag               |
| Degradation-Level = CRITICAL bei Startup   | Nutzer ignorieren echte Alarme       | Default auf NORMAL bis erste Daten        |
| MQTT-Connect in `begin()`                  | Sendet Discovery bevor System bereit | Discovery erst im ersten `loop()`         |
| Watchdog-Feed nicht in Initialisierung     | Bootloop bei langsamer Hardware      | `feedWatchdog()` in langen Init-Schleifen |

### Beispiel

```cpp
// ❌ BAD — immediate degradation on boot
bool sensorOk = poolSensorOk_ && solarSensorOk_;  // beide false → CRITICAL

// ✅ GOOD — wait for first measurement cycle
bool sensorOk = sensorsEverReported_ ? (poolSensorOk_ && solarSensorOk_) : true;
```

---

## K6 — Naming & Consistency

Gleiche Dinge heißen gleich. Unterschiedliche Dinge heißen unterschiedlich.

### Prüfliste

| Regel                            | Schlecht                                                | Gut                                                       |
| -------------------------------- | ------------------------------------------------------- | --------------------------------------------------------- |
| Gleiche Operation, gleicher Name | `saveSensorAddressMapping()` + `saveSensorMappingNvs()` | `saveSensorMapping()`                                     |
| Boolean-Frage                    | `getSwitch()`                                           | `isSwitchOn()` oder `getSwitchState()`                    |
| Abkürzungen                      | `tmp`, `buf`, `cfg`                                     | `temperature`, `buffer`, `config` (wenn Scope > 5 Zeilen) |
| Prefix-Konsistenz                | `_count` vs `count_` vs `mCount`                        | Projektstandard: `count_` für Member                      |
| Enum-Werte                       | `RED`, `YELLOW`, `GREEN` (degradation)                  | `TimeDegradation::RED` (scoped)                           |
| Einzahl/Mehrzahl                 | `options[]` (Array)                                     | `option` für Einzel-Item                                  |
| Getter/Setter                    | `getMode().c_str()` (String → const char\*)             | `getModeCStr()`                                           |

### Funktionale Namen

| Pattern           | Bedeutung                           |
| ----------------- | ----------------------------------- |
| `is*()`           | Rückgabe `bool` (Frage)             |
| `get*()`          | Rückgabe Wert (keine Seiteneffekte) |
| `set*()`          | Setzt Wert                          |
| `has*()`          | Prüft Existenz                      |
| `publish*()`      | Sendet MQTT                         |
| `begin()`/`end()` | Lebenszyklus                        |

---

## K7 — Const Correctness

Jeder Parameter, der nicht geschrieben wird, ist `const`. Das ist kein Stil — es verhindert
unbeabsichtigte Mutationen und ermöglicht Compiler-Optimierung.

### Prüfliste

| Regel                         | ❌                          | ✅                                |
| ----------------------------- | --------------------------- | --------------------------------- |
| Referenz-Parameter            | `void foo(String &s)`       | `void foo(const String &s)`       |
| Pointer-Parameter (read-only) | `void foo(char *p)`         | `void foo(const char *p)`         |
| Member-Funktion (read-only)   | `int getValue()`            | `int getValue() const`            |
| Lokale Variable (unverändert) | `int val = compute();`      | `const int val = compute();`      |
| Array-Parameter               | `void foo(uint8_t addr[8])` | `void foo(const uint8_t addr[8])` |

### Besondere Fälle für ESP32

```cpp
// PROMFGM/Flash-Strings
// ❌ BAD
void publishSensor(const char *name);
// ✅ GOOD — wo möglich
void publishSensor(const __FlashStringHelper *name);
```

---

## K8 — Testability & Decoupling

Statische Klassen, versteckte Dependencies und Singleton-Zugriffe machen Code
unittestbar. Jede neue Klasse sollte ohne Hardware-Init testbar sein.

### Prüfliste

| Muster                                      | Problem                             | Lösung                                        |
| ------------------------------------------- | ----------------------------------- | --------------------------------------------- |
| `static` Methoden                           | Nicht mockbar                       | Dependency Injection über Interface           |
| Direkter `NetworkManager::publish()`-Aufruf | Kann im Test nicht ersetzt werden   | Interface + `MqttClient`-Parameter            |
| `Preferences`-Direktzugriff                 | Testet NVS statt Logik              | ConfigManager als Abstraktion                 |
| Globaler Zustand                            | Tests beeinflussen sich gegenseitig | Klassenzustand pro Instanz                    |
| `millis()` / `delay()` im Code              | Nicht deterministisch testbar       | TimeSource-Interface                          |
| Freundliche Funktionen in `.cpp`            | Nicht testbar ohne main-Compile     | In Header verschieben oder über Klasse testen |

### Testbarkeits-Hierarchie

```
Am testbarsten:
  Reine Logik ohne HW: Rule, Timer, DegradationManager (→ native test)

Mäßig testbar:
  ConfigManager (→ Preferences mockbar)

Schwer testbar:
  MqttPublisher (→ NetworkManager::publish static)

Kaum testbar:
  PoolController (→ viele statische Dependencies)
```

---

## Quick Reference: Pre-PR Clean Code Review

Vor jedem Merge-Request diese 8 Kategorien durchgehen:

```text
□ K1 — Heap Safety:  Kein String in Hot Paths? char-Buffer überall?
□ K2 — DRY:           Keine Duplikate (NVS, Discovery, Conversions)?
□ K3 — Debris:        Kein toter Code, keine sinnlosen Checks?
□ K4 — Logging:       DEBUG-Output hinter Präprozessor-Guards?
□ K5 — Boot Sequence: Keine Degradation/Failures vor erster Messung?
□ K6 — Naming:        Gleiche Dinge = gleiche Namen?
□ K7 — Const:         Alles const was nicht geschrieben wird?
□ K8 — Testability:   Code testbar? Dependencies sichtbar?
```

### Common Quick Fixes

```bash
# String-Allokationen im MqttPublisher finden
semble search "String stateTopic\|String configTopic\|String payload\|getBaseTopic" src/MqttPublisher.cpp

# NVS-Duplikate finden
semble search 'Preferences\.begin\("' src/

# Serial-Debug in nicht-debug Code finden
semble search "Serial\.\(printf\|println\)" src/ --exclude '*Debug*'

# Dead-Code-Rest: auskommentierte Blöcke
semble search "(?s)//\n// " src/  # mehrzeilige Kommentare prüfen
```

## References

- [PR #143](https://github.com/smart-swimmingpool/pool-controller/pull/143) — Codex Review (Ursprungsdokument)
- [PR #144](https://github.com/smart-swimmingpool/pool-controller/pull/144) — Implementierung aller Fixes
- [cpp-code-quality](./cpp-code-quality/SKILL.md) — Formatting, Linting, CI Gates
- [iot-quality](./iot-quality/SKILL.md) — Anti-Patterns, IoT-spezifische Verbote
- [architecture-refactor](./architecture-refactor/SKILL.md) — Dependency Injection, Layer-Trennung
- [cpp-memory-opt](./cpp-memory-opt/SKILL.md) — Heap/Stack/Buffer-Optimierung für ESP32
