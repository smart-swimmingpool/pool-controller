---
title: Multicore-Architektur
summary: Wie die Firmware beide ESP32-Kerne nutzt — dedizierte I/O-Tasks (Sensoren, Display, MQTT-Telemetrie) auf Kern 0 und eine deterministische Regelschleife auf Kern 1
date: "2026-08-01"
lastmod: "2026-08-01"
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

Der ESP32 hat zwei Xtensa-LX6-Kerne, aber ein Single-Loop-Arduino-Sketch nutzt nur
einen: Der WiFi/BT-Stack läuft auf Kern 0, die Arduino-`loop()` auf Kern 1. Alles
andere — Sensor-Messungen, Display-Update, Regeln, Netzwerk, MQTT — läuft seriell
innerhalb von `loop()`.

Die Firmware wird zu einer **Task-Architektur mit expliziter Kern-Trennung**
umstrukturiert:

| Kern | Rolle | Inhalt |
| ---- | ----- | ------ |
| **Kern 0** (PRO_CPU) | I/O-Kern | SensorTask (DS18B20 + interner Temperatursensor), DisplayTask (OLED-Rendering, nur NORVI), PublishTask (MQTT-Telemetrie-Serialisierung) |
| **Kern 1** (APP_CPU) | Regel-Kern | Arduino-`loop()`: Watchdog, Degradation, Regeln, Relais, Status-LED, asynchrone Netzwerk-Manager, OTA, Frontpanel-Tasterabfrage |

## Warum

Blockierende Arbeit blockierte bisher die gesamte Regelschleife. Die teuerste
Operation ist die DS18B20-Temperaturkonvertierung (`requestTemperatures()`), die bei
12-Bit-Auflösung etwa **750 ms** blockiert. In dieser Zeit warten Watchdog-Feeding,
Regelauswertung und Relais-Ansteuerung.

Das Auslagern dieser I/O-Arbeit in dedizierte Tasks auf Kern 0 bringt drei Vorteile:

1. **Geringe Loop-Latenz** — die Regelschleife bleibt im niedrigen Millisekundenbereich.
2. **Isolation** — ein hängender Sensor-Bus oder ein I2C-Display kann die
   sicherheitskritische Regellogik auf Kern 1 nicht mehr blockieren.
3. **Headroom** — Kapazität für zukünftige Funktionen (mehr Sensoren, Web-UI, Logging).

## Task-Modell

Alle I/O-Tasks werden in `setup()` vom `CoreScheduler` erzeugt und bleiben statisch
(keine dynamische Task-Erzeugung zur Laufzeit, kein Heap-Wachstum).

| Task | Kern | Priorität | Stack | Läuft auf |
| ---- | ---- | --------- | ----- | --------- |
| SensorTask | 0 | 2 | 6 KB | allen Builds |
| PublishTask | 0 | 1 | 4 KB | allen Builds |
| DisplayTask | 0 | 1 | 3 KB | nur NORVI (`#ifdef NORVI_AE01_R`) |

FreeRTOS-Prioritäten gelten nur innerhalb eines Kerns: Die I/O-Tasks geben per
`vTaskDelay` nach und bleiben unterhalb der WiFi-Stack-Tasks auf Kern 0 — sie können
die Regelschleife auf Kern 1 also nie verdrängen.

## Datenfluss

```text
SensorTask (Kern 0) ── lock-free Slots ──▶ Regelschleife (Kern 1): Regeln/Relais/Watchdog
SensorTask ── Status ────────────────────▶ DegradationManager (Kern 1)
Regelschleife ── update() + Render-Anforderung ─▶ DisplayTask (Kern 0, NORVI)
Tasterabfrage bleibt in der Regelschleife (Kern 1) — Callbacks mutieren Loop-Singletons
Regelschleife ── Telemetrie-Queue ────────▶ PublishTask (Kern 0) ──▶ MQTT
Regelschleife ── asynchrones Netzwerk/OTA ── (unverändert, Kern 1)
```

Jeder task-übergreifende Datenpfad ist **Single-Writer**:

- Sensorwerte: lock-free Slots (atomar/ein Wort) — SensorTask schreibt, Regelschleife liest.
- Display-Zustand: `volatile`-Render-Anforderungs-Flag — Regelschleife fordert an,
  DisplayTask rendert (Wortzugriff ist auf dem ESP32 atomar).
- Taster-Eingaben: bleiben in der Regelschleife — die Taster-Callbacks mutieren
  Loop-Singletons (`operationModeNode`, `poolPumpNode`); eine Abfrage auf Kern 0
  würde die Single-Writer-Regel verletzen. Ausgelagert ist nur das OLED-*Rendering*
  (blockierende I2C-Arbeit).
- Telemetrie: SPSC-Ringpuffer mit fester Kapazität — Regelschleife stellt ein,
  PublishTask serialisiert und publiziert.

Die MQTT-*Verbindung* und die Web-/OTA-Manager bleiben in der Regelschleife — sie
sind bereits nicht-blockierend (`AsyncMqttClient`, asynchroner Webserver). Ausgelagert
wird nur die Telemetrie-Serialisierung (JSON-Aufbau, HA-Discovery-Payloads) in den
PublishTask.

## Zuverlässigkeit

- Die Regelschleife füttert den Task-Watchdog weiterhin; I/O-Tasks füttern ihn bei
  langen Wartezeiten (DS18B20-Konvertierung, OTA-Pause).
- `SystemMonitor` meldet die Stack-High-Water-Marks der Tasks, sodass die
  Stack-Größen in Logs und Degradation sichtbar sind.
- Safe-Mode- und Degradations-Semantik bleiben unverändert — Sensorfehler werden dem
  `DegradationManager` über einen thread-sicheren Statuskanal gemeldet.
- Während OTA pausiert der PublishTask das Publizieren, leert seine Queue aber
  weiter.

## Design-Dokument

Das vollständige Design inklusive Thread-Safety-Audit der bestehenden Singletons,
Migrationsphasen, Risiken und Erfolgskriterien liegt unter
[`docs/superpowers/specs/2026-08-01-multicore-task-architecture-design.md`](../superpowers/specs/2026-08-01-multicore-task-architecture-design.md).
