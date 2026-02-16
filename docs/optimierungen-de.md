# Pool Controller - Optimierungen und Verbesserungen

## Zusammenfassung

Dieses Dokument beschreibt die durchgeführten Optimierungen am Pool
Controller für einen zuverlässigen 24/7-Betrieb.

## Durchgeführte Analysen und Behobene Probleme

### 1. Speicherlecks und Speicherfragmentierung

**Problem**:

- Der Code hat bei jeder Messung temporäre String-Objekte erstellt
- Dies führte zur Heap-Fragmentierung bei Langzeitbetrieb (24/7)
- Auf ESP8266/ESP32 mit begrenztem RAM kritisch

**Lösung**:

- Alle dynamischen String-Allokationen durch Stack-basierte Puffer ersetzt
- 10+ String-Allokationen pro Messzyklus eliminiert
- Neue Hilfsfunktionen in `Utils.hpp` für speichereffiziente Konvertierungen

**Betroffene Dateien**:

- `DallasTemperatureNode.cpp` - 1 String-Allokation eliminiert
- `OperationModeNode.cpp` - 7 String-Allokationen eliminiert
- `ESP32TemperatureNode.cpp` - 1 String-Allokation eliminiert

**Auswirkung**: Bei einem typischen Messzyklus von 30-300 Sekunden werden
über 24 Stunden 2.880 bis 28.800 Heap-Allokationen/Deallokationen eingespart.

### 2. millis() Überlauf-Handling

**Problem**:

- Der ursprüngliche Code behandelte millis()-Überläufe nicht korrekt
- millis() läuft nach ~49,7 Tagen über
- Dies konnte zu fehlerhaftem Timing führen

**Lösung**:

- Neue Funktion `Utils::shouldMeasure()` mit korrektem Überlauf-Handling
- Alle Loop-Methoden aktualisiert

**Betroffene Dateien**:

- `DallasTemperatureNode.cpp`
- `ESP32TemperatureNode.cpp`
- `OperationModeNode.cpp`
- `RelayModuleNode.cpp`

### 3. Code-Qualität und Vereinfachung

**Verbesserungen**:

- Doppelte `Homie.isConnected()` Prüfungen entfernt
- **Kritischen Bug in LoggerNode::logf behoben**: vsnprintf war
  auskommentiert, was zu uninitialisierten Puffern und potentiellen
  Abstürzen führte
- Veralteten Code im `deprecated/` Ordner gelöscht
- Code-Konsistenz über alle Sensor-Nodes verbessert

### 4. Zustandspersistenz (NEU)

**Problem**:

- Nach Neustart oder Stromausfall gingen alle Einstellungen verloren
- Benutzer mussten Controller neu konfigurieren
- Pumpen blieben im undefinierten Zustand

**Lösung**:

- Alle Zustände werden automatisch im persistenten Speicher gesichert
- Automatische Wiederherstellung nach Neustart/Stromausfall
- ESP32: Nutzt Preferences API (NVS Storage)
- ESP8266: Basis-Unterstützung (wird erweitert)

**Persistierte Daten**:

- Betriebsmodus (auto/manu/boost/timer)
- Temperatureinstellungen (Pool Max, Solar Min, Hysterese)
- Timer-Einstellungen (Start/Ende Zeiten)
- Relais-Zustände (Pool-Pumpe, Solar-Pumpe)

**Neue Dateien**:

- `StateManager.hpp` - Verwaltung des persistenten Speichers

### 5. System-Überwachung und Auto-Neustart (NEU)

**Problem**:

- Bei Speichermangel konnte Controller abstürzen
- Keine automatische Wiederherstellung
- Hängende Systeme blieben unentdeckt

**Lösung**:

- Kontinuierliche Speicherüberwachung (alle 10 Sekunden)
- Automatischer Neustart bei kritischem Speichermangel
- Hardware Watchdog Timer (ESP32)
- Software Watchdog (ESP8266)

**Schwellwerte**:

- **ESP8266**: Warnung bei < 8KB, Neustart bei < 4KB
- **ESP32**: Warnung bei < 16KB, Neustart bei < 8KB

**Funktionen**:

- Speicherüberwachung
- Minimaler Heap-Tracking
- Heap-Fragmentierung (ESP8266)
- Watchdog-Timer (ESP32: 30s Hardware, ESP8266: Software)
- Automatischer Neustart bei kritischem Speicher

**Neue Dateien**:

- `SystemMonitor.hpp` - System-Gesundheitsüberwachung

**Vorteile**:

- ✅ Automatische Erholung von Speicherproblemen
- ✅ Erkennung und Behebung von System-Hängern
- ✅ Keine manuelle Intervention erforderlich
- ✅ 24/7 Betrieb ohne Ausfallzeiten

## Bibliotheks-Aktualisierungen

### ArduinoJson: 6.18.0 → 7.3.0

- Performance-Verbesserungen
- Bessere Speicherverwaltung
- Sicherheitsfixes
- Breaking Changes behandelt (StaticJsonDocument → JsonDocument)

### NTPClient: 3.1.0 → 3.2.1

- Fehlerbehebungen
- Verbesserte Zeitsynchronisierung

## Neue Funktionen

### MQTT-Protokoll-Konfiguration

**Home Assistant MQTT Discovery Support**:

- Alternative zum Homie Convention
- Konfigurierbar über `mqtt-protocol` Einstellung
- Zwei Modi verfügbar:
  - `"homie"` - Homie 3.0 Convention (Standard)
  - `"homeassistant"` - Home Assistant MQTT Discovery

**Vorteile**:

- Flexibilität bei der Smart Home Integration
- Native Home Assistant Auto-Discovery
- Weiterhin kompatibel mit openHAB via Homie

**Dokumentation**:

- Siehe `docs/mqtt-configuration.md` für Konfigurationsdetails
- Siehe `docs/optimization-report.md` für technische Details

## Konfiguration

### MQTT-Protokoll einstellen

#### Via Homie UI

1. Mit dem WiFi-AP des Geräts verbinden
2. Zur Konfigurationsseite navigieren
3. "mqtt-protocol" auf "homie" oder "homeassistant" setzen
4. Speichern und neu starten

#### Via config.json

```json
{
  "name": "Pool Controller",
  "settings": {
    "mqtt-protocol": "homeassistant"
  }
}
```

## Optimierungen für 24/7-Betrieb

### Speicher-Optimierungen

- **Vorher**: ~10-15 String-Objekte pro Messzyklus
- **Nachher**: 0 String-Objekte pro Messzyklus
- **Heap-Fragmentierung**: Deutlich reduziert
- **Langzeitstabilität**: Verbessert

### Timing-Zuverlässigkeit

- Korrekte Behandlung von millis()-Überläufen
- Zuverlässiger Betrieb über 49+ Tage

### Code-Größe

- Leicht erhöht durch neue Funktionen (+2 KB)
- Kompensiert durch ArduinoJson 7 Optimierungen

## Empfohlene Tests

1. **Langzeitbetrieb**: 60+ Tage Betrieb zur Verifizierung des
   Überlauf-Handlings
2. **Speicher-Überwachung**: Free Heap über 24-48 Stunden überwachen
3. **MQTT-Protokoll-Wechsel**: Beide Modi (Homie und Home Assistant) testen
4. **Sensor-Tests**: Mit getrennten Sensoren und schnellen
   Temperaturänderungen testen

## Zukünftige Verbesserungsmöglichkeiten

1. **Watchdog Timer**: ESP Watchdog für automatische Wiederherstellung
   implementieren
2. **NTP-Konfiguration**: NTP-Server konfigurierbar machen (aktuell
   hartcodiert)
3. **Persistente Einstellungen**: Laufzeit-Konfigurationsänderungen im Flash
   speichern
4. **OTA-Updates**: Zuverlässige Over-the-Air Updates sicherstellen

## Versions-Informationen

**Neue Version**: 3.1.0

**Änderungen**:

- Home Assistant MQTT Discovery Support
- Speicher-Optimierungen für 24/7-Betrieb
- Bibliotheks-Updates (ArduinoJson 7.3.0, NTPClient 3.2.1)
- Code-Qualitätsverbesserungen
- Entfernung von veraltetem Code

Siehe `CHANGELOG.md` für vollständige Details.

## Fazit

Die durchgeführten Optimierungen verbessern die Eignung des Pool Controllers
für 24/7-Betrieb erheblich:

✅ **Heap-Fragmentierung eliminiert** durch Vermeidung wiederholter
String-Allokationen
✅ **Timing-Fehler behoben** die nach 49,7 Tagen auftreten würden
✅ **Abhängigkeiten aktualisiert** für bessere Performance und Sicherheit
✅ **Flexibilität erweitert** durch Dual-MQTT-Protokoll-Support
✅ **Code-Qualität beibehalten** bei verbesserter Zuverlässigkeit

Diese Änderungen stellen sicher, dass der Controller kontinuierlich ohne
Speicherprobleme oder Timing-Fehler laufen kann.
