---
title: Zustandsspeicherung und Systemüberwachung
summary: 24/7-Zuverlässigkeitsfunktionen für den ESP32 Pool Controller — NVS-Zustandsspeicherung, Speicherüberwachung, Watchdog-Timer, Health-API und Fehlerbehebung
date: "2026-06-11"
lastmod: "2026-06-11"
draft: false
toc: true
type: docs
tags: ["docs", "persistence", "monitoring", "watchdog", "reliability"]
menu:
  docs:
    parent: Pool Controller
    name: Zustandsspeicherung
    weight: 60
---

# Zustandsspeicherung und Systemüberwachung

## Übersicht

Der Pool Controller verfügt über eine umfassende Zustandsspeicherung und
Systemzustandsüberwachung für zuverlässigen 24/7-Betrieb.

## Zustandsspeicherung

### Was gespeichert wird

Alle Controller-Zustände werden automatisch im nichtflüchtigen Speicher (NVS)
gespeichert und nach Neustarts oder Stromausfällen wiederhergestellt:

#### Betriebseinstellungen

- **Betriebsmodus**: auto, manual, boost, timer
- **Pool-Maximaltemperatur**: Ziel-Pooltemperatur
- **Solar-Minimaltemperatur**: Minimale Solartemperatur für Aktivierung
- **Temperatur-Hysterese**: Temperaturdifferenz für die Regelung
- **Timer-Einstellungen**: Start- und Endzeiten für den Timer-Modus

#### Relaiszustände

- **Pool-Pumpe**: Ein/Aus-Zustand
- **Solar-Pumpe**: Ein/Aus-Zustand

### Funktionsweise

Verwendet die Preferences-Bibliothek für dauerhaften Speicher in NVS
(Non-Volatile Storage). Jeder Wert wird mit einem typspezifischen Schlüssel
gespeichert.

Relaiszustände werden einzeln pro Relais über einen eigenen Namespace
in NVS gespeichert und speichern den Ein/Aus-Zustand unter dem Schlüssel
`"switch"`.

### Automatische Wiederherstellung

Beim Neustart des Controllers:

1. **State Manager** lädt alle gespeicherten Werte
2. **Konfigurationseinstellungen** aus der LittleFS config.json können den
   gespeicherten Zustand überschreiben
3. **Letzter bekannter Zustand** wird verwendet, wenn keine Konfigurationsüberschreibung existiert

Dadurch wird sichergestellt:

- Nach einem Stromausfall kehren die Pumpen in ihren vorherigen Zustand zurück
- Benutzerkonfigurierte Temperaturen und Timer bleiben erhalten
- Der Betriebsmodus bleibt über Neustarts hinweg erhalten

### Beispielszenario

```text
Benutzer stellt ein:
  - Betriebsmodus: auto
  - Pool max. Temp.: 28,5°C
  - Timer: 10:30 - 17:30

Stromausfall um 14:00

Controller startet neu:
  - Lädt gespeicherten Zustand
  - Stellt Betriebsmodus auto wieder her
  - Stellt Temperaturen und Timer wieder her
  - Setzt Betrieb nahtlos fort
```

## Systemzustandsüberwachung

### Speicherüberwachung

Das System überwacht kontinuierlich den freien Heap-Speicher, um Abstürze
durch Speichererschöpfung zu verhindern.

#### Schwellwerte

**ESP32**:

- **Speicherwarnung (niedrig)**: < 16 KB (16.384 Bytes)
- **Kritischer Speicher**: < 8 KB (8.192 Bytes) → Automatischer Neustart

#### Verhalten

1. **Alle 10 Sekunden**: Speicherprüfung wird durchgeführt
2. **Niedriger Speicher**: Warnung wird an Seriell und MQTT protokolliert
3. **Kritischer Speicher**: Controller startet automatisch neu
4. **Minimum-Tracking**: Zeichnet den niedrigsten Speicherstand seit Boot auf

### Watchdog-Timer

Verhindert Systemhänger und stellt die Wiederherstellung nach Softwarefehlern sicher.

#### ESP32

- **Hardware-Watchdog**: 30-Sekunden-Timeout
- **Automatischer Panic**: Neustart, wenn Watchdog nicht gefüttert wird
- **Wird in der Hauptschleife gefüttert**: Jeder Zyklus

### Health-Status-API

SystemMonitor bietet Methoden zur Überprüfung des Systemzustands:

```cpp
// Aktuellen freien Heap abrufen
uint32_t heap = SystemMonitor::getFreeHeap();

// Minimalen Heap seit Boot abrufen
uint32_t minHeap = SystemMonitor::getMinFreeHeap();

// Prüfen, ob das System gesund ist
bool healthy = SystemMonitor::isHealthy();

// Betriebszeit in Sekunden abrufen
uint32_t uptime = SystemMonitor::getUptimeSeconds();

uint8_t fragmentation = SystemMonitor::getHeapFragmentation();
```

## Konfiguration

### Funktionen aktivieren

Zustandsspeicherung und Systemüberwachung sind **automatisch aktiviert**. Keine Konfiguration erforderlich.

### Schwellwerte anpassen

Zum Anpassen der Speicherschwellwerte `src/SystemMonitor.hpp` ändern:

```cpp
// Speicherschwelle (niedrig) — nur Warnung
static constexpr uint32_t LOW_MEMORY_THRESHOLD = 8192;  // 8 KB

// Kritische Speicherschwelle (automatischer Neustart)
static constexpr uint32_t CRITICAL_MEMORY_THRESHOLD = 4096;  // 4 KB
```

### Auto-Neustart deaktivieren

Wenn Sie niedrigen Speicher manuell behandeln möchten (nicht empfohlen für
24/7-Betrieb):

Den Auto-Neustart-Abschnitt in `src/SystemMonitor.hpp` auskommentieren:

```cpp
// Kritischer Speicher - sofort neu starten
if (freeHeap < criticalThreshold) {
    Serial.printf("CRITICAL: Free heap %d bytes < %d bytes. Rebooting...\n",
                freeHeap, criticalThreshold);
    // Serial.flush();
    // delay(1000);
    // ESP.restart();  // Diese Zeile auskommentieren, um Auto-Neustart zu deaktivieren
}
```

## Überwachung und Logs

### Serielle Ausgabe

**Normaler Betrieb**:

```text
✓ State loaded from persistent storage
State persistence and system monitoring initialized
Free heap: 28,456 bytes
```

**Speicherwarnung (niedrig)**:

```text
WARNING: Low memory detected. Free heap: 7,892 bytes (min: 7,456)
```

**Kritischer Speicher** (vor Neustart):

```text
CRITICAL: Free heap 3,842 bytes < 4,096 bytes. Rebooting...
```

### MQTT-Logs

Der Systemstatus wird über LoggerNode an das MQTT-Topic veröffentlicht:

```text
pool-controller/log
```

Beispielnachrichten:

- `"State persistence and system monitoring initialized"`
- `"WARNING: Low memory detected. Free heap: 7892 bytes (min: 7456)"`

## Vorteile für den 24/7-Betrieb

### Zuverlässigkeit

- ✅ Übersteht Stromausfälle
- ✅ Erholt sich automatisch von Speicherproblemen
- ✅ Erkennt und behebt Systemhänger
- ✅ Kein manueller Eingriff erforderlich

### Benutzererfahrung

- ✅ Einstellungen bleiben über Neustarts erhalten
- ✅ Keine Neukonfiguration nach Stromausfall
- ✅ Nahtlose Betriebskontinuität
- ✅ Vorhersagbares Verhalten

### Wartung

- ✅ Erkennung von Speicherlecks
- ✅ Automatische Problembehebung
- ✅ Health-Status-Überwachung
- ✅ Diagnoseinformationen verfügbar

## Fehlerbehebung

### Zustände werden nicht gespeichert

1. Serielle Ausgabe auf "State loaded from persistent storage" prüfen
2. Prüfen, ob die NVS-Partition verfügbar ist
3. Auf Preferences-Fehler in den Logs prüfen

### Häufige Neustarts

Wenn der Controller häufig neu startet:

1. **Speichernutzung prüfen**: Logs auf Speicherwarnungen durchsuchen
2. **Speicherleck identifizieren**: Muster erkennen, wann Neustarts auftreten
3. **Speichernutzung reduzieren**:

   - Messintervalle erhöhen
   - MQTT-Nachrichtenhäufigkeit reduzieren
   - Funktionen nach Möglichkeit deaktivieren

4. **Schwellwert senken**: Kritischen Schwellwert vorübergehend senken, um Neustarts zu verhindern

   (der Controller startet dann trotzdem neu, aber die Betriebszeit kann während der Fehlersuche verlängert werden)

### Watchdog-Timeouts

Wenn der Watchdog auslöst (ESP32):

1. **Lange blockierende Operationen**: Auf Verzögerungen oder lange Operationen im Code prüfen
2. **Timeout erhöhen**: Timeout in `SystemMonitor::begin()` anpassen
3. **Häufiger füttern**: `SystemMonitor::feedWatchdog()` in langen Operationen einfügen

## Technische Details

### Speichernutzung

**ESP32 NVS**:

- Betriebsmodus: ~10 Bytes
- Gleitkommawerte (3): 12 Bytes
- Integer-Werte (4): 16 Bytes
- Gesamt: ~40 Bytes

### Leistungsauswirkung

- **Zustand speichern**: < 10ms (nur bei Änderungen)
- **Zustand laden**: < 20ms (einmal beim Boot)
- **Speicherprüfung**: < 1ms (alle 10 Sekunden)
- **Watchdog füttern**: < 0,1ms (jeder Schleifendurchlauf)

**Gesamtauswirkung**: Vernachlässigbar (< 0,1 % CPU-Auslastung)

## Zukünftige Erweiterungen

- 🔜 **Konfigurierbare Schwellwerte**: MQTT-basierte Schwellwertkonfiguration
- 🔜 **Health-Dashboard**: Web-UI für Zustandsüberwachung

---

**Version**: 3.3.0
**Status**: Produktionsbereit
**Plattform**: ESP32
