---
title: Multicore-Architektur
summary: Wie die Firmware blockierende Sensor-I/O auf ESP32-Kern 0 isoliert und veränderlichen Controller-Zustand auf Kern 1 serialisiert
date: "2026-08-01"
lastmod: "2026-10-03"
draft: false
toc: true
type: docs
featured: false
tags: ["docs", "controller", "architektur", "multicore", "tasks"]
menu:
  docs:
    parent: Pool Controller
    name: Multicore-Architektur
    weight: 33
---

## Überblick

Die Firmware verwendet ein explizites Ownership-Modell, statt jede I/O-Operation auf den zweiten Kern
zu verschieben. Das sicherheitsrelevante Controller-Modell bleibt Single-Writer in der Arduino-Loop,
während die blockierende DS18B20-Konvertierung auf Kern 0 isoliert wird.

| Kern | Rolle | Inhalt |
| ---- | ----- | ------ |
| **Kern 0** (PRO_CPU) | Sensor-I/O | `SensorTask`: DS18B20-Konvertierung/-Auslesen und interne ESP32-Temperatur |
| **Kern 1** (APP_CPU) | Regelung und zustandsbehaftete I/O | Arduino-`loop()`: Watchdog, Degradation, Regeln, Relais, Status-LED, Netzwerk/OTA, MQTT-Serialisierung, Frontpanel-UI und OLED-Rendering |

## Warum diese Grenze

Eine 12-Bit-DS18B20-Konvertierung kann ungefähr 750 ms dauern. In der Arduino-Loop würde sie
Regelauswertung, Watchdog-Verarbeitung und Relaisentscheidungen blockieren. `SensorTask` entfernt genau
diese blockierende Operation aus der Regelschleife.

MQTT-Serialisierung und OLED-Rendering bleiben bewusst auf Kern 1. Beide lesen veränderlichen
Controller-Zustand wie Betriebsmodus, Regeln, Relais und Konfiguration. Ein paralleles Lesen auf Kern 0
würde Datenrennen etwa um Arduino-`String` und Regelzustände erzeugen. Die Ausführung auf dem besitzenden
Task ist einfacher und sicherer als Locks über den gesamten Objektgraphen.

## Task-Modell

`CoreScheduler` erzeugt genau einen erforderlichen Worker-Task:

| Task | Kern | Priorität | Stack | Zweck |
| ---- | ---- | --------- | ----- | ----- |
| `SensorTask` | 0 | 2 | 6 KB | DS18B20- und interne Temperaturmessung |

Kann `SensorTask` nicht erzeugt werden, startet der Controller neu. Ein dauerhafter Fehler wird durch die
bestehende Boot-Loop-Erkennung und den Safe-Mode behandelt.

MQTT-Anforderungen verwenden weiterhin `TelemetryQueue`. Die Queue wird jedoch aus
`PoolController::loop()` über `CoreScheduler::logStackWatermarks()` geleert. Dadurch bleiben Reihenfolge
und Entkopplung erhalten, während `MqttPublisher` auf demselben Core-1-Task läuft, dem auch der mutable
Controller-Zustand gehört.

Beim NORVI-Display gilt dieselbe Regel: Taster, UI-Zustandsübergänge und OLED-Rendering werden durch
`DisplayCoordinator` auf Kern 1 serialisiert.

## Sensor-Ownership

Nach dem Start ist `SensorTask` alleiniger Besitzer aller `OneWire`- und `DallasTemperature`-Operationen.
Code auf Kern 1 scannt oder liest den Bus nicht direkt.

`DallasTemperatureNode` stellt deshalb zwei Core-übergreifende Kanäle bereit:

- Messwerte werden über `SensorSlots` publiziert.
- Sensor-Erkennungsdaten und ausgewählte ROM-Adressen werden in einen kleinen Cache kopiert, der durch
  einen kurzen atomaren Lock geschützt ist. Web- und MQTT-Code lesen nur diesen Cache.

Änderungen der Sensor-Zuordnung werden zur Laufzeit als pending Konfiguration gespeichert. `SensorTask`
wendet sie zu Beginn eines späteren Messzyklus an. WebPortal- oder MQTT-Aufrufe führen damit keine
OneWire-Transaktion auf Kern 1 aus.

## DS18B20-Messzyklus

`DallasTemperature` wird mit `setWaitForConversion(false)` konfiguriert. Ein Messzyklus besteht aus:

1. Konvertierung auf dem benötigten Bus beziehungsweise den Bussen starten.
2. `SensorTask` für 800 ms freigeben.
3. Fertige Messwerte lesen.
4. Ergebnis in `SensorSlots` publizieren und Discovery-Cache aktualisieren.

Damit entfällt die frühere doppelte Wartezeit, bei der `requestTemperatures()` bereits intern blockierte
und der Worker anschließend nochmals 800 ms wartete.

Fehlt mindestens einer der Dallas-Sensoren oder ist sein Wert ungültig, verwendet der gemeinsame Zyklus
den kürzeren Recovery-Intervall von 5 Sekunden. Sobald beide Sensoren gültig sind, gilt wieder das
konfigurierte `loopInterval`.

## Datenfluss

```text
SensorTask (Kern 0) ── SensorSlots ──────▶ Regelschleife (Kern 1): Regeln / Relais
SensorTask (Kern 0) ── Discovery-Cache ──▶ WebPortal / MQTT / UI (Kern 1)
Regelschleife (Kern 1) ─ TelemetryQueue ─▶ MQTT-Serialisierung (Kern 1)
DisplayCoordinator: Eingabe + Zustand + Rendering ───────────▶ Kern 1
```

Die entscheidende Regel ist Ownership: Hardware-Buszustand gehört `SensorTask`; veränderlicher
Controller-Zustand gehört der Regelschleife. Core-übergreifende Kommunikation verwendet begrenzte
Snapshots statt gemeinsam veränderter Objekte.

## Zuverlässigkeit

- Sensorwert und Found-Status werden konsistent über `SensorSlots` übertragen.
- `DegradationManager` verwendet atomare Core-übergreifende Status-Flags.
- `SensorTask` registriert sich beim Task-Watchdog und füttert ihn.
- Stack-High-Water-Logging bleibt für den Worker verfügbar.
- OTA-Verarbeitung und MQTT-Zustandsänderungen bleiben auf Kern 1 serialisiert.

## Design-Dokument

Das ursprüngliche Design und der Migrationsplan bleiben unter
[`docs/superpowers/specs/2026-08-01-multicore-task-architecture-design.md`](../superpowers/specs/2026-08-01-multicore-task-architecture-design.md)
erhalten. Nach dem Concurrency-Review wurde die endgültige Core-Grenze bewusst enger gefasst.
