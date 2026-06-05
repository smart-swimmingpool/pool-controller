# Architekturverbesserungsplan - Smart Swimming Pool Controller

> **Status**: Draft  
> **Erstellt**: 2024  
> **Version**: 1.0  
> **Autor**: Mistral Vibe Code

---

## 📋 Zusammenfassung

Dieser Plan beschreibt die schrittweise Verbesserung der Softwarearchitektur des
**Smart Swimming Pool Controllers**. Ziel ist es, die Wartbarkeit, Testbarkeit
und Erweiterbarkeit des Systems zu erhöhen, während die bestehende
Funktionalität erhalten bleibt.

**Aktuelle Probleme:**

- Speicherlecks durch manuelles Speichermanagement
- Enge Kopplung zwischen Komponenten (Tight Coupling)
- Duplizierter Code (z. B. Timer-Logik)
- Fehlende Unit Tests
- Globale Abhängigkeiten (z. B. Homie-Logger)
- Keine Persistenz für Konfigurationen
- Plattformabhängiger Code (ESP32 vs. ESP8266)

---

## 🎯 Ziele

| **Ziel**                         | **Priorität** | **Messbarer Erfolg**                            |
| -------------------------------- | ------------- | ----------------------------------------------- |
| Beheben von Speicherlecks        | ⭐⭐⭐⭐⭐    | Keine Memory Leaks in Valgrind/PlatformIO Debug |
| Einführung von Unit Tests        | ⭐⭐⭐⭐⭐    | Testabdeckung > 80% für Kernlogik               |
| Reduzierung von Code-Duplikation | ⭐⭐⭐⭐      | Keine duplizierte Logik in `git grep`           |
| Verbesserung der Testbarkeit     | ⭐⭐⭐⭐      | Mocking von Hardware-Abhängigkeiten möglich     |
| Persistenz für Konfiguration     | ⭐⭐⭐        | Einstellungen überleben Reset                   |
| Plattformunabhängigkeit          | ⭐⭐          | Ein Code für ESP32 und ESP8266                  |

---

## 📅 Meilensteine

### **🟢 Phase 1: Kritische Fehler beheben (1-2 Wochen)**

> **Fokus**: Speicherlecks, Fehlerbehandlung, Grundlegende Tests

| **Task**                                                                 | **Aufwand**                                              | **Verantwortlich** | **Status** | **Abhängigkeiten** |
| ------------------------------------------------------------------------ | -------------------------------------------------------- | ------------------ | ---------- | ------------------ |
| [#101](https://github.com/smart-swimmingpool/pool-controller/issues/101) | Speicherlecks in `OperationModeNode` beheben             | 2 Tage             | ⬜         | Keine              |
| [#102](https://github.com/smart-swimmingpool/pool-controller/issues/102) | Null-Checks in `OperationModeNode::getRule()` hinzufügen | 1 Tag              | ⬜         | Keine              |
| [#103](https://github.com/smart-swimmingpool/pool-controller/issues/103) | Plattformunabhängige Pin-Definitionen                    | 2 Tage             | ⬜         | Keine              |
| [#104](https://github.com/smart-swimmingpool/pool-controller/issues/104) | Grundlegende Unit-Test-Infrastruktur aufsetzen           | 3 Tage             | ⬜         | Keine              |

**Ergebnis**: Stabilere Codebasis ohne kritische Fehler.

---

### **🟡 Phase 2: Architektur verbessern (2-3 Wochen)**

> **Fokus**: Dependency Injection, Code-Duplikation entfernen, Interfaces

| **Task**                                                                 | **Aufwand**                                                  | **Verantwortlich** | **Status** | **Abhängigkeiten** |
| ------------------------------------------------------------------------ | ------------------------------------------------------------ | ------------------ | ---------- | ------------------ |
| [#201](https://github.com/smart-swimmingpool/pool-controller/issues/201) | `IRelayController`-Interface einführen                       | 2 Tage             | ⬜         | Phase 1            |
| [#202](https://github.com/smart-swimmingpool/pool-controller/issues/202) | Regeln auf `IRelayController` umstellen                      | 3 Tage             | ⬜         | #201               |
| [#203](https://github.com/smart-swimmingpool/pool-controller/issues/203) | `checkPoolPumpTimer()` in gemeinsame Basisklasse verschieben | 1 Tag              | ⬜         | Phase 1            |
| [#204](https://github.com/smart-swimmingpool/pool-controller/issues/204) | Logger-Interface injizieren (statt `Homie.getLogger()`)      | 2 Tage             | ⬜         | Phase 1            |
| [#205](https://github.com/smart-swimmingpool/pool-controller/issues/205) | `ITemperatureSensor`-Interface einführen                     | 2 Tage             | ⬜         | Phase 1            |

**Ergebnis**: Entkoppelte Komponenten, bessere Testbarkeit.

---

### **🟠 Phase 3: Persistenz & Konfiguration (1 Woche)**

> **Fokus**: Speichern von Einstellungen, Konfigurierbarkeit

| **Task**                                                                 | **Aufwand**                                      | **Verantwortlich** | **Status** | **Abhängigkeiten** |
| ------------------------------------------------------------------------ | ------------------------------------------------ | ------------------ | ---------- | ------------------ |
| [#301](https://github.com/smart-swimmingpool/pool-controller/issues/301) | Timer-Einstellungen mit `HomieSetting` speichern | 2 Tage             | ⬜         | Phase 1            |
| [#302](https://github.com/smart-swimmingpool/pool-controller/issues/302) | NTP-Server konfigurierbar machen                 | 1 Tag              | ⬜         | Phase 1            |
| [#303](https://github.com/smart-swimmingpool/pool-controller/issues/303) | Temperaturschwellen als `HomieSetting`           | 1 Tag              | ⬜         | Phase 1            |

**Ergebnis**: Konfigurationen überleben Reset, Benutzerfreundlichkeit ↑

---

### **🔵 Phase 4: Tests & Qualitätssicherung (2-3 Wochen)**

> **Fokus**: Testabdeckung erhöhen, CI/CD verbessern

| **Task**                                                                 | **Aufwand**                        | **Verantwortlich** | **Status** | **Abhängigkeiten** |
| ------------------------------------------------------------------------ | ---------------------------------- | ------------------ | ---------- | ------------------ |
| [#401](https://github.com/smart-swimmingpool/pool-controller/issues/401) | Unit Tests für `RuleAuto`          | 2 Tage             | ⬜         | Phase 2            |
| [#402](https://github.com/smart-swimmingpool/pool-controller/issues/402) | Unit Tests für `RuleTimer`         | 2 Tage             | ⬜         | Phase 2            |
| [#403](https://github.com/smart-swimmingpool/pool-controller/issues/403) | Unit Tests für `OperationModeNode` | 3 Tage             | ⬜         | Phase 2            |
| [#404](https://github.com/smart-swimmingpool/pool-controller/issues/404) | Unit Tests für `Timer`-Logik       | 1 Tag              | ⬜         | Phase 2            |
| [#405](https://github.com/smart-swimmingpool/pool-controller/issues/405) | CI/CD Pipeline für Tests erweitern | 2 Tage             | ⬜         | Phase 1            |

**Ergebnis**: Testabdeckung > 80%, Regressionsschutz ✅

---

### **⚪ Phase 5: Fortgeschrittene Verbesserungen (Optional, 2-4 Wochen)**

> **Fokus**: Architektur-Patterns, Event-Driven Design

| **Task**                                                                 | **Aufwand**                         | **Verantwortlich** | **Status** | **Abhängigkeiten** |
| ------------------------------------------------------------------------ | ----------------------------------- | ------------------ | ---------- | ------------------ |
| [#501](https://github.com/smart-swimmingpool/pool-controller/issues/501) | State-Pattern für Betriebsmodi      | 3 Tage             | ⬜         | Phase 2            |
| [#502](https://github.com/smart-swimmingpool/pool-controller/issues/502) | Event-Bus für Temperaturänderungen  | 4 Tage             | ⬜         | Phase 2            |
| [#503](https://github.com/smart-swimmingpool/pool-controller/issues/503) | Factory-Pattern für Node-Erstellung | 2 Tage             | ⬜         | Phase 2            |
| [#504](https://github.com/smart-swimmingpool/pool-controller/issues/504) | Dokumentation aktualisieren         | 2 Tage             | ⬜         | Alle Phasen        |

**Ergebnis**: Moderne, wartbare Architektur 🚀

---

## 📂 Dateistruktur (Ziel)

```text
pool-controller/
├── src/
│   ├── core/                  # Kernlogik (plattformunabhängig)
│   │   ├── rules/             # Regel-Implementierungen
│   │   │   ├── Rule.hpp       # Basisklasse
│   │   │   ├── RuleAuto.hpp   # Auto-Modus
│   │   │   ├── RuleTimer.hpp  # Timer-Modus
│   │   │   └── ...
│   │   ├── services/          # Dienste (Timer, Logger, etc.)
│   │   │   ├── TimerService.hpp
│   │   │   └── ILogger.hpp
│   │   └── interfaces/        # Interfaces für DI
│   │       ├── IRelayController.hpp
│   │       └── ITemperatureSensor.hpp
│   │
│   ├── nodes/                 # Homie-Nodes
│   │   ├── OperationModeNode.hpp
│   │   ├── RelayModuleNode.hpp
│   │   └── ...
│   │
│   ├── platform/              # Plattformspezifischer Code
│   │   ├── esp32/
│   │   │   └── PlatformConfig.hpp
│   │   └── esp8266/
│   │       └── PlatformConfig.hpp
│   │
│   └── main.cpp               # Haupteinstiegspunkt
│
├── test/                      # Unit Tests
│   ├── rules/
│   │   ├── test_RuleAuto.cpp
│   │   └── ...
│   ├── services/
│   │   └── test_TimerService.cpp
│   └── mocks/                 # Mock-Implementierungen
│       ├── MockRelayController.hpp
│       └── ...
│
├── docs/
│   ├── architecture.md        # Architektur-Dokumentation
│   └── this file              # Verbesserungsplan
│
└── platformio.ini            # Build-Konfiguration
```

---

## 🔧 Technische Richtlinien

### **1. Coding Standards**

- **Namen**: `camelCase` für Variablen/Funktionen, `PascalCase` für Klassen
- **Header**: Jede Datei beginnt mit Copyright-Hinweis und kurzer Beschreibung
- **Kommentare**: Doxygen-Style für öffentliche Methoden
- **Logging**: Verwende `LN.log()` statt `Homie.getLogger()`

### **2. Dependency Injection**

- **Regel**: Keine globalen Instanzen in Klassen
- **Ausnahme**: Singletons wie `Homie` (aber über Interfaces zugreifen)
- **Beispiel**:

  ```cpp
  // ❌ Schlechter Stil
  class RuleAuto {
      void loop() { Homie.getLogger() << "..." << endl; }
  };

  // ✅ Guter Stil
  class RuleAuto {
      RuleAuto(ILogger& logger) : _logger(logger) {}
      void loop() { _logger.log("..."); }
  private:
      ILogger& _logger;
  };
  ```

### **3. Speichermanagement**

- **Regel**: Immer `std::unique_ptr` oder `std::shared_ptr` für dynamische Objekte
- **Ausnahme**: Keine (Raw Pointer nur für nicht-ownende Referenzen)
- **Beispiel**:

  ```cpp
  // ❌ Schlechter Stil
  Rule* rule = new RuleAuto(...);

  // ✅ Guter Stil
  auto rule = std::make_unique<RuleAuto>(...);
  ```

### **4. Fehlerbehandlung**

- **Regel**: Immer `nullptr`-Checks bei Zeigern
- **Regel**: Verwende `assert()` für interne Konsistenzprüfungen
- **Beispiel**:

  ```cpp
  Rule* rule = getRule();
  if (!rule) {
      _logger.log("Error: No rule found", LoggerNode::ERROR);
      return;
  }
  ```

### **5. Testing**

- **Framework**: PlatformIO Unit Testing Framework
- **Mocking**: Handgeschriebene Mocks oder [FakeIt](https://github.com/eranpe/FakeIt)
- **Abdeckung**: Mindestens 80% für Kernlogik (Rules, Timer, etc.)

---

## 📊 Erfolgsmetriken

| **Metrik**            | **Aktuell** | **Ziel**         | **Messmethode**             |
| --------------------- | ----------- | ---------------- | --------------------------- |
| Code-Duplikation      | Hoch        | 0%               | `git grep` / SonarQube      |
| Testabdeckung         | 0%          | >80%             | PlatformIO Test Coverage    |
| Cyclomatic Complexity | Hoch        | <10 pro Funktion | SonarQube                   |
| Speicherlecks         | Ja          | Nein             | Valgrind / PlatformIO Debug |
| Build-Zeit            | ?           | <2 Min           | `time pio run`              |
| Binärgröße            | ?           | <500KB           | `pio run -t size`           |

---

## 🚀 Nächste Schritte

1. **Issue-Tracker vorbereiten**: Issues für alle Tasks in diesem Plan erstellen
2. **Branch-Strategie festlegen**:
   - `main`: Stabiler Code
   - `develop`: Integrationsbranch
   - `feature/*`: Feature-Branches
3. **CI/CD anpassen**: Tests in GitHub Actions/PlatformIO CI integrieren
4. **Code Review**: Alle Änderungen müssen über Pull Requests mit Review

---

## 📚 Referenzen

- [Homie for ESP8266/ESP32](https://homieiot.github.io/)
- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/plus/unit-testing.html)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [SOLID Principles](https://en.wikipedia.org/wiki/SOLID)

---

## 📝 Changelog

| **Version** | **Datum** | **Änderungen** | **Autor**         |
| ----------- | --------- | -------------- | ----------------- |
| 1.0         | 2024      | Initialer Plan | Mistral Vibe Code |

---

## 💬 Feedback

Fragen oder Anregungen zu diesem Plan? Eröffne ein
[Issue](https://github.com/smart-swimmingpool/pool-controller/issues) oder
starte eine
[Diskussion](https://github.com/smart-swimmingpool/pool-controller/discussions).
