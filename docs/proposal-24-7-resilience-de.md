# 24/7 Betrieb: Resilienz-Konzept für den Pool Controller

> **Status**: Phase 1+2 implementiert (P1–P6)  
> **Letztes Update**: 2026-05-15  
> **Branch**: `proposal/24-7-resilience`  
> **Basis**: Version 3.1.0  

---

## 1. Einleitung und Zielsetzung

Der Pool Controller muss **24/7 autark** laufen können – ohne manuelles Eingreifen,
selbst bei:

- **Stromausfall**: Controller startet nach Wiederkehr mit sinnvollem Zustand
- **WLAN-Ausfall**: Regelt Pumpen eigenständig weiter (bereits teilweise umgesetzt)
- **MQTT-Broker-Ausfall**: Läuft autonom, puffert Zustandsänderungen
- **Temperatursensor-Ausfall**: Degradiert kontrolliert statt abzustürzen

### Aktueller Reifegrad

Was bereits existiert (v3.1.0):

| Mechanismus | Status |
|---|---|---|
| State Persistence (ESP32 Preferences, ESP8266 EEPROM) | ✅ |
| Relay-State-Wiederherstellung nach Reboot | ✅ |
| `setRunLoopDisconnected(true)` für Offline-Betrieb | ✅ |
| SystemMonitor mit Watchdog (ESP32 HW, ESP8266 SW) | ✅ |
| Speicher-Monitoring mit Auto-Reboot bei Kritisch | ✅ |
| NTP-Zeit-Caching mit millis()-Fallback | ✅ |
| NaN-Validierung für Sensorwerte in Rules | ✅ |
| Pin-Konflikt-Prüfung beim Start | ✅ |
| Timer-Midnight-Crossing-Logik | ✅ |
| **State-Load unabhängig von MQTT (P1)** | **✅ Seit 840f3a8** |
| **ESP8266 EEPROM-Kollisionsfrei (P3)** | **✅ Seit 840f3a8** |
| **Watchdog-Fütterung in Langläufern (P6)** | **✅ Seit 840f3a8** |
| **DegradationManager (P5)** | **✅ Seit 50be817** |
| **NTP Dreistufen-Degradation (P2)** | **✅ Seit c969663** |
| **MQTT State-Refresh bei Reconnect (P4)** | **✅ Seit 6eedebf** |

Dieses Proposal adressiert die **verbleibenden Lücken** für einen wirklich
ausfallsicheren 24/7 Betrieb.

---

## 2. Analyse der verbleibenden Schwachstellen

### 🔴 Kritisch: State-Load nur bei MQTT-Verbindung

**Problem**: `operationModeNode.loadState()` wird in `setupHandler()` aufgerufen,
der wiederum nur feuert, wenn Homie eine MQTT-Verbindung herstellt.

Betroffener Code:
```cpp
// PoolController.cpp:250
auto PoolControllerContext::setupHandler() -> void {
  StateManager::begin();
  SystemMonitor::begin();
  operationModeNode.loadState();  // ← Läuft NICHT ohne MQTT!
  // ...
}

// PoolController.cpp:343
auto PoolControllerContext::setup() -> void {
  Homie.setup();
  initializeController();  // ← Erzeugt Regeln, aber lädt keine States
}
```

**Szenario**: Power-Failure + WLAN-Ausfall → Controller bootet → Homie
verbindet kein MQTT → `setupHandler()` feuert nicht → Mode bleibt auf
HomieSetting-Default ("auto"), nicht der zuvor gespeicherte Modus.
Temperaturen, Timer etc. ebenfalls auf Default.

**Auswirkung**: Bei kombinierter Störung gehen alle Benutzereinstellungen
verloren, bis das WLAN/MQTT wieder steht.

### 🟡 Hoch: Pumpen-Logik bei anhaltendem NTP-Ausfall

**Problem**: `checkPoolPumpTimer()` deaktiviert die Pumpe sofort, wenn
`time.tm_year == -1` (kein gültiges NTP). Nach 24h ohne Sync springt
die Zeit-Schätzung auf "invalid".

```cpp
// RuleAuto.cpp:89
if (time.tm_year == -1) {
  Homie.getLogger() << "⚠ Time sync invalid - timer disabled" << endl;
  return false;  // Pumpe AUS
}
```

**Auswirkung**: Bei längerem WLAN-Ausfall (>24h) stoppt Timer-Modus die
Pumpe komplett, selbst wenn die geschätzte Zeit noch plausibel wäre.
Auto-Modus schaltet nur den Solar-Pump-Teil ab (Pool-Pump läuft weiter),
aber das ist undokumentiert.

### 🟡 Hoch: ESP8266 EEPROM Kollisionen

**Problem**: Der DJB2-Hash auf nur 15 Slots à 32 Byte ist kollisionsanfällig.
Bei 8 gespeicherten Keys kann es zu Überschreibungen kommen.

```cpp
// StateManager.cpp:28
return EEPROM_DATA_START + (hash % EEPROM_SLOT_COUNT) * EEPROM_SLOT_SIZE;
// 15 Slots für 8+ Keys → Kollisionswahrscheinlichkeit ~85%
```

**Auswirkung**: Settings überschreiben sich gegenseitig → unvorhersehbares
Verhalten nach Reboot auf ESP8266.

### 🟡 Hoch: Startreihenfolge zwischen initializeController und Homie-Node-Setup

**Problem**: `PoolControllerContext::setup()` ruft `Homie.setup()` auf,
gefolgt von `initializeController()`. Die `RelayModuleNode.setup()` und
`OperationModeNode.setup()` (Homie-Node-Setups) werden von Homie in dessen
`setup()` aufgerufen. Allerdings: die `RelayModuleNode`-Statics werden VOR
`PoolControllerContext` als globale Konstruktoren initialisiert.

Die Reihenfolge ist:
1. Globale Konstruktoren: `LN`, `solarTemperatureNode`, `poolTemperatureNode`,
   `ctrlTemperatureNode`, `poolPumpNode`, `solarPumpNode`, `operationModeNode`
2. `PoolControllerContext::setup()` → `Homie.setup()` (Node-Setups laufen)
3. `initializeController()` → erzeugt Rules, setzt Intervalle

**Problem**: `initializeController()` erzeugt Rules mit `new` ohne Owner.
Die Vector-Klasse in `OperationModeNode` besitzt die Pointer, löscht sie
im Destruktor. Das ist fragil – ein vorzeitiger Destruktordurchlauf (z.B.
durch Homie-internen Reconnect) könnte Use-After-Free verursachen.

### 🟠 Mittel: Silent MQTT Publish-Failures

**Problem**: Viele `MqttInterface`-Methoden prüfen `Homie.isConnected()`,
aber wenn die Verbindung WÄHREND des Publishens wegbricht, schlägt die
Operation still.

```cpp
// MqttInterface.hpp:68
inline void publishSwitchState(...) {
  if (isHomeAssistant()) {
    HomeAssistant::DiscoveryPublisher::publishSwitchState(...);  // kein Return-Check
  } else {
    node.setProperty(homieProperty).send(state ? "true" : "false");  // Ergebnis ignoriert
  }
}
```

**Auswirkung**: Status-Updates gehen verloren. Beim Reconnect werden keine
Nachhole-Publishes getriggert → Home Assistant zeigt u.U. veraltete Zustände.

### 🟠 Mittel: Keine Watchdog-Fütterung in Langläufern

**Problem**: Der Watchdog (30s auf ESP32) wird nur in der Hauptschleife
gefüttert. Ein langer `sensor.requestTemperatures()`-Call (bis zu 750ms)
ist noch unkritisch. Aber bei großen MQTT-Publishes oder blockierenden
Operationen kann der Watchdog auslösen.

```cpp
// PoolController.cpp:391
auto PoolControllerContext::loop() -> void {
  SystemMonitor::feedWatchdog();
  SystemMonitor::checkMemory();
  Homie.loop();
}
```

**Empfehlung**: Watchdog-Fütterung in potenziell lange Operationen einbauen.

### 🟠 Mittel: Kein Retry/Backoff für Sensor-Recovery

**Problem**: Wenn ein DallasSensor disconnectet, bleibt `_temperature = NaN`
und wird nicht aktiv neu gesucht. Das nächste `loop()`-Intervall versucht
einen Lesevorgang, aber es gibt kein beschleunigtes Recovery.

```cpp
// DallasTemperatureNode
void loop() {
  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    sensor.requestTemperatures();
    // Wenn fehlgeschlagen: bleibt NaN bis zum nächsten Intervall (30s+)
  }
}
```

**Auswirkung**: Nach Sensordefekt kann es Minuten dauern, bis der Sensor
wieder erkannt wird, weil kein verkürztes Polling-Intervall aktiv ist.

### 🔵 Niedrig: Millis()-Overflow nach ~49 Tagen

**Problem**: `millis()` auf ESP8266/ESP32 läuft nach ca. 49 Tagen über.
`Utils::shouldMeasure()` handhabt das korrekt per unsigned arithmetic.
Aber es gibt keine Tests dafür und keine Monitoring-Meldung.

---

## 3. Lösungsvorschläge

### P1: State-Load von MQTT entkoppeln 🔴 Kritisch

**Ziel**: Persistierte Zustände werden immer geladen, unabhängig von WiFi/MQTT.

**Lösungsansatz A (empfohlen) — Load in `initializeController()`**:

```cpp
auto PoolControllerContext::setup() -> void {
  Homie.setup();

  // StateManager und SystemMonitor immer initialisieren,
  // nicht nur bei MQTT-Verbindung
  StateManager::begin();
  SystemMonitor::begin();

  initializeController();  // ← loadState() hier rein oder danach
  operationModeNode.loadState();  // Läuft immer, auch ohne WLAN
}
```

Homie-Settings dienen dann nur noch als **Factory Defaults**, die von
persistierten Werten überschrieben werden.

**Lösungsansatz B — Fallback-Kette**:  
`initializeController()` prüft Homie::isConnected() und ruft
`loadState()` auf. Wenn Homie das erste Mal verbindet, wird `loadState()`
erneut aufgerufen (überschreibt ggf. aktuellere Werte).

**Aufwandsabschätzung**: 1–2 Tage  
**Risiko**: Niedrig (StateManager muss vor Homie-Callbacks initialisiert sein)

### P2: Graceful Degradation für NTP-Ausfall 🟡 Hoch

**Ziel**: Bei Zeitunsicherheit nicht einfach alles abschalten, sondern
intelligent degradieren.

**Lösungsansatz — Drei-Stufen-Modell**:

| Stufe | Bedingung | Verhalten |
|---|---|---|
| **Grün** | letzter NTP-Sync < 1h | Normalbetrieb |
| **Gelb** | 1h–24h seit letztem Sync | Timer läuft mit millis()-Schätzung, Warnung via MQTT/Serial |
| **Rot** | >24h seit letztem Sync **oder** Zeit springt unplausibel | Timer-Modus fällt auf Auto-Modus zurück, Pumpe läuft mit Temperaturlogik (falls Sensoren OK) |

```cpp
// Vorschlag für RuleAuto::checkPoolPumpTimer()
bool RuleAuto::checkPoolPumpTimer() {
  tm time = getCurrentDateTime();

  if (time.tm_year == -1) {
    auto degradation = getTimeDegradationLevel();

    if (degradation == TimeDegradation::YELLOW) {
      // millis()-Schätzung verwenden, trotzdem weiterlaufen
      return checkPumpTimerWithEstimate();
    }

    // Rot: Fallback auf Temperatursteuerung oder letzte bekannte Ein/Aus-Zeit
    Homie.getLogger() << "⚠ Time degraded RED - using fallback schedule" << endl;
    return getFallbackPumpState();
  }
  // Normal: präzise Timer-Logik
  return checkPumpTimerExact(time);
}
```

**Aufwandsabschätzung**: 2–3 Tage  
**Risiko**: Mittel (muss sorgfältig getestet werden, da Verhalten bei Zeit-Fallback
kritisch für die Beckenhygiene ist)

### P3: ESP8266 EEPROM-Kollisionen vermeiden 🟡 Hoch

**Lösungsansatz A (empfohlen) — Zusammenlegung in ein strukturiertes Layout**:

```cpp
// Klare Slot-Zuweisung statt Hashing
enum EEPROMSlot : uint16_t {
  SLOT_MAGIC      = 0,     // 4 bytes Magic Number
  SLOT_OPMODE     = 4,     // 32 bytes: Mode String
  SLOT_POOL_TEMP  = 36,    // 4 bytes: float
  SLOT_SOLAR_TEMP = 40,    // 4 bytes: float
  SLOT_HYSTERESIS = 44,    // 4 bytes: float
  SLOT_TIMER_START_H = 48, // 4 bytes: int
  SLOT_TIMER_START_M = 52, // 4 bytes: int
  SLOT_TIMER_END_H   = 56, // 4 bytes: int
  SLOT_TIMER_END_M   = 60, // 4 bytes: int
  SLOT_RELAY_POOL    = 64, // 1 byte: bool
  SLOT_RELAY_SOLAR   = 65, // 1 byte: bool
  SLOT_CHECKSUM      = 66, // 2 bytes: CRC16 über Slots 4-65
  SLOT_END           = 68   // Passt in 512 Bytes EEPROM
};
```

**Vorteile**:
- Keine Kollisionen
- CRC-Schutz gegen korrupte Daten
- Einfach erweiterbar (genug Platz)
- Schneller als Hashing

**Lösungsansatz B — Einfach**: `EEPROM_SLOT_COUNT` auf 32 erhöhen (mehr
Platz, keine Kollisionen, aber geringere Redundanz).

**Aufwandsabschätzung**: 1–2 Tage  
**Risiko**: Mittel (bricht Kompatibilität zu existierenden EEPROM-Inhalten)

### P4: MQTT-Publish-Retry & Reconnect-Refresh 🟠 Mittel

**Ziel**: Keine verlorenen Status-Updates bei temporären MQTT-Ausfällen.

**Lösungsansatz — Dirty-Flag-Queue**:

```cpp
class MqttPublishQueue {
  struct PendingPublish {
    char topic[64];
    char payload[32];
    bool retained;
    uint32_t retryCount;
  };

  static constexpr uint8_t MAX_QUEUE = 10;
  PendingPublish queue_[MAX_QUEUE];
  uint8_t head_, tail_;

public:
  bool publish(const char* topic, const char* payload, bool retained);
  void onReconnect();  // Sendet alle queued Messages
};

// Im Homie onReadyToOperate-Callback:
void PoolControllerContext::setupHandler() {
  // ...
  MqttPublishQueue::onReconnect();  // Nachhole alle verpassten Updates
}
```

**Alternative** (einfacher): Nach MQTT-Reconnect alle relevanten States
neu publishen (Mode, Temperaturen, Relays). Das ist bereits teilweise
in `OperationModeNode::loop()` implementiert, aber nur im Intervall.

**Aufwandsabschätzung**: 2–3 Tage  
**Risiko**: Niedrig (zusätzlicher Code, der nur auf Reconnect feuert)

### P5: Explizite Degradations-Strategie 🟠 Mittel

**Ziel**: Klar definierte Betriebszustände mit dokumentiertem Verhalten.

```cpp
enum class DegradationLevel {
  NORMAL,        // Alles OK
  NO_WIFI,       // WLAN weg, laufend, keine MQTT-Updates
  NO_TIME,       // Zeit unsicher, Timer-Fallback aktiv
  NO_SENSOR,     // Sensor defekt, Pumpe mit Vorsichtswerten
  CRITICAL,      // Mehrere Ausfälle → Safe Mode
};

class DegradationManager {
  static DegradationLevel currentLevel_;
  static DegradationLevel previousLevel_;

public:
  static void evaluate();   // Läuft jede Schleife
  static void onEnter(DegradationLevel level);   // Logging + MQTT
  static void onExit(DegradationLevel level);    // Recovery
  static bool isSafe();     // Safe Mode aktiv?
};
```

**Verhalten nach Degradationsstufe**:

| Level | Pool-Pumpe | Solar-Pumpe | Logging |
|---|---|---|---|
| NORMAL | Regelkonform | Regelkonform | MQTT + Serial |
| NO_WIFI | Regelkonform | Regelkonform | Serial |
| NO_TIME | Auto-Modus | Regelkonform | Serial + Warnung |
| NO_SENSOR | Letzter guter Wert (max 1h) | AUS | Serial + Alarm |
| CRITICAL | Manuell (letzter State) | AUS | Serial + Dauerwarnung |

**Aufwandsabschätzung**: 2–3 Tage  
**Risiko**: Mittel (neue Komponente, Überschneidung mit SystemMonitor)

### P6: Watchdog-Fütterung in Langläufern 🟠 Mittel

```cpp
// DallasTemperatureNode.cpp
void DallasTemperatureNode::loop() {
  SystemMonitor::feedWatchdog();  // ← vor blockierendem Call

  if (Utils::shouldMeasure(_lastMeasurement, _measurementInterval)) {
    sensor.requestTemperatures();  // max 750ms
    SystemMonitor::feedWatchdog(); // ← nach blockierendem Call

    _temperature = sensor.getTempCByIndex(0);
  }
}
```

**Aufwandsabschätzung**: 0.5 Tage  
**Risiko**: Keines (Watchdog-Reset ist idempotent)

### P7: Beschleunigtes Sensor-Recovery 🟠 Mittel

```cpp
void DallasTemperatureNode::loop() {
  // Wenn Sensor im Fehler: verkürztes Intervall für schnelleres Recovery
  uint32_t effectiveInterval = isnan(_temperature)
    ? RECOVERY_INTERVAL  // z.B. 5 Sekunden statt 60
    : _measurementInterval;

  if (Utils::shouldMeasure(_lastMeasurement, effectiveInterval)) {
    // ... Lesevorgang
    if (isnan(_temperature)) {
      _lastMeasurement = millis();  // Nächster Versuch in RECOVERY_INTERVAL
    }
  }
}
```

**Aufwandsabschätzung**: 1 Tag  
**Risiko**: Niedrig

### P8: Startsicherheit durch Boot-Counter 🟠 Mittel

**Ziel**: Erkenne Boot-Loops (Stromausfall-Serie oder Crash nach Kurzbetrieb).

```cpp
void detectBootLoop() {
  int bootCount = StateManager::loadInt("bootCount", 0);
  uint32_t uptime = SystemMonitor::getUptimeSeconds();

  if (bootCount == 0) {
    // Erster Boot seit Reset
    StateManager::saveInt("bootCount", 1);
    StateManager::saveInt("lastBootUptime", uptime);  // wird 0 sein
    return;
  }

  uint32_t lastUptime = StateManager::loadInt("lastBootUptime", 0);

  if (lastUptime < 300 && bootCount > 2) {
    // 3+ kurze Boots → Boot-Loop!
    enterSafeMode();
  }

  StateManager::saveInt("bootCount", bootCount + 1);
  StateManager::saveInt("lastBootUptime", uptime);
}
```

**Safe Mode**: Alle Relays AUS, nur Basisfunktionen aktiv, MQTT-Warnung
"Safe Mode: Boot-Loop detected".

**Aufwandsabschätzung**: 1 Tag  
**Risiko**: Niedrig

### P9: Konfigurierbare Notlauf-Zeiten 🟠 Mittel

**Ziel**: Benutzer konfigurieren, was bei Zeitverlust passieren soll.

Als `HomieSetting` oder via MQTT:

```cpp
// Neue Einstellungen
HomieSetting<const char*> fallbackModeSetting_{"fallback-mode",
  "Betriebsart bei Zeitverlust (auto/manu/off)"};
HomieSetting<bool> keepPumpOnTimeLossSetting_{"keep-pump-on-time-loss",
  "Pumpe bei Zeitverlust weiterlaufen lassen (true/false)"};
HomieSetting<long> timeLossMaxHoursSetting_{"time-loss-max-hours",
  "Maximale Stunden ohne Zeitsync, bevor Fallback aktiv wird (1-72)"};
```

**Aufwandsabschätzung**: 1 Tag  
**Risiko**: Niedrig

---

## 4. Priorisierte Roadmap

### Phase 1: Kritische Stabilität ✅ Implementiert (840f3a8)

| Priority | Proposal | Aufwand | Status |
|---|---|---|---|
| 🔴 P1 | State-Load von MQTT entkoppeln | 1–2 Tage | ✅ |
| 🟡 P3 | ESP8266 EEPROM-Kollisionen beheben | 1–2 Tage | ✅ |
| 🟠 P6 | Watchdog-Fütterung in Langläufern | 0.5 Tage | ✅ |

### Phase 2: Graceful Degradation ✅ Implementiert (50be817–6eedebf)

| Priority | Proposal | Aufwand | Status |
|---|---|---|---|
| 🟡 P2 | Graceful Degradation für NTP-Ausfall | 2–3 Tage | ✅ |
| 🟠 P4 | MQTT-Publish-Retry & Reconnect-Refresh | 2–3 Tage | ✅ |
| 🟠 P5 | Explizite Degradations-Strategie | 2–3 Tage | ✅ |

### Phase 3: Proaktive Resilienz (Folgewochen)

| Priority | Proposal | Aufwand |
|---|---|---|
| 🟠 P7 | Beschleunigtes Sensor-Recovery | 1 Tag |
| 🟠 P8 | Boot-Loop Erkennung + Safe Mode | 1 Tag |
| 🟠 P9 | Konfigurierbare Notlauf-Zeiten | 1 Tag |

---

## 5. Tests & Verifikation

Für jedes Proposal sind Tests erforderlich:

| Proposal | Test-Szenario | Status |
|---|---|---|
| P1 | Boot ohne WLAN → States geladen ✓. Boot ohne WLAN nach Modus-Wechsel → neuer Modus aktiv | ✅ |
| P2 | NTP blockiert (1h, 6h, 24h, 48h) → korrekte Degradation. Timer fällt auf Auto zurück | ❌ Noch zu testen |
| P3 | Alle 8 Keys schreiben → wieder auslesen → korrekte Werte. CRC-Korruption → Default-Werte | ❌ Noch zu testen |
| P4 | MQTT-Ausfall 5 Min → 10 State-Änderungen → nach Reconnect alle korrekt publisht | ❌ Noch zu testen |
| P5 | Simulierter Sensor-Ausfall → Degradation auf NO_SENSOR → korrektes Pumpen-Verhalten | ❌ Noch zu testen |
| P6 | 500ms blocking call → Watchdog nicht ausgelöst | ❌ Noch zu testen |
| P7 | Sensor disconnect → Recovery in <10s statt >30s |
| P8 | 5 Kurzboots in Folge → Safe Mode aktiv. Ein langer Boot → Boot-Counter zurückgesetzt |
| P9 | Fallback-Mode per MQTT auf "manu" → Pumpe läuft bei Zeitverlust per letztem Befehl |

---

## 6. Auswirkungen auf bestehende Architektur

### Neue Dateien

```
src/
├── DegradationManager.hpp   (P5)
├── DegradationManager.cpp   (P5)
```

### Geänderte Dateien

| Datei | Änderungen |
|---|---|
| `PoolController.cpp` | State-Load in `setup()`, DegradationManager-Loop, MQTT State-Refresh bei Reconnect |
| `StateManager.cpp` | ESP8266 EEPROM neues Layout + CRC16 (P3) |
| `TimeClientHelper.hpp/cpp` | TimeDegradation Drei-Stufen-Modell (P2) |
| `Timer.cpp` | tm_year=-1 nur bei RED (P2) |
| `DallasTemperatureNode.cpp` | Watchdog-Feed vor/nach requestTemperatures (P6) |
| `RuleAuto.cpp` | Pumpen-Fallback bei RED (P2) |
| `RuleTimer.cpp` | Pumpen-Fallback bei RED (P2) |
| `SystemMonitor.hpp` | Degradation-Integration (P5) |

### Rückwärtskompatibilität

- P3 (EEPROM-Layout) bricht Kompatibilität zu existierenden ESP8266-Installationen.
  Migration: Einmalig alter Magic → neues Layout, oder: erstes Boot nach Update
  initialisiert neu.
- Alle anderen Proposals sind additive Änderungen ohne Bruch.

---

## 7. Zusammenfassung

Das System ist für den 24/7 Betrieb **grundsätzlich gut aufgestellt** (s.
die bereits implementierten Features), hat aber einige kritische Lücken:

1. **🔴 P1** ist der dringendste Fix: Ohne ihn gehen bei kombiniertem
   Strom+WLAN-Ausfall alle Einstellungen verloren.
2. **🟡 P2 + P3** adressieren die beiden häufigsten Dauerausfall-Szenarien
   (WLAN-Totalausfall >24h, ESP8266-Speicher-Korruption).
3. **🟠 P4–P9** machen das System von "läuft meistens" zu "läuft
   zuverlässig unter allen Bedingungen".

> **Empfehlung**: P1, P3 und P6 in einem ersten Schritt umsetzen, dann
> P2+P5 als zweiten Schritt, den Rest nach Bedarf.

---

## 8. Anhang: Aktuelle Architektur (Referenz)

Für das Verständnis der Proposals hier die relevante Startreihenfolge
und Datenflüsse im aktuellen Code:

```
ESP boot
  → setup()
    → Homie.setup()
      → Homie-intern: Node-Setups (RelayModuleNode, OperationModeNode)
      → [wenn MQTT connected] setupHandler()
        → StateManager::begin()
        → SystemMonitor::begin()
        → operationModeNode.loadState()     ← KRITISCH: nur hier!
    → initializeController()
      → Rules erzeugen
      → Intervalle setzen
  → loop()
    → SystemMonitor::feedWatchdog()
    → SystemMonitor::checkMemory()
    → Homie.loop()
      → OperationModeNode::loop()
        → Rule::loop() (temperaturabhängige Pumpensteuerung)
```

```
Datenfluss bei WiFi-Ausfall:

Sensor liest → Temperaturwert aktualisiert → Rule evaluiert →
Relay setSwitch() → GPIO schaltet → State persistiert in NVS/EEPROM

→ MQTT-Publish wird wegen !Homie.isConnected() übersprungen
→ Kein Retry beim Reconnect
```

---

*Ende des Proposals. Bei Fragen oder Änderungswünschen bitte melden.*
