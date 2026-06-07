---
title: "Temperaturabhängige Filterlaufzeit"
description: "Dynamische Verlängerung der Pumpenlaufzeit basierend auf der Wassertemperatur"
weight: 50
---

# Temperaturabhängige Filterlaufzeit

**Optimierung der Pool-Umwälzung für bessere Wasserqualität bei minimalem Stromverbrauch.**

## 1. Motivation

Die Poolpumpe ist der größte Stromverbraucher im Schwimmbad. Gleichzeitig bestimmt sie maßgeblich die Wasserqualität. Die aktuell nötige Umwälzmenge hängt stark von der Wassertemperatur ab:

| Wassertemperatur | Risiko | Empfohlene Umwälzung |
|---|---|---|
| **> 26 °C** | Hohes Algenwachstum, Trübung | Maximal (8–12 h/Tag) |
| **20–26 °C** | Normalbetrieb | Standard (4–8 h/Tag) |
| **< 20 °C** | Kaum Algenwachstum | Minimal (1–4 h/Tag) |

Ein starrer Timer arbeitet entweder **überdimensioniert** (Stromverschwendung bei kühlem Wasser) oder **unterdimensioniert** (schlechte Wasserqualität bei Hitze).

**Ziel:** Die tägliche Filterlaufzeit automatisch an die gemessene Pooltemperatur anpassen.

## 2. Konzept

Die im Timer konfigurierte Laufzeit wird dynamisch verlängert, wenn die Wassertemperatur einen Schwellwert überschreitet. Die Verlängerung ist proportional zur Temperaturdifferenz.

### Berechnungsformel

```
effektiveLaufzeit = basisLaufzeit + max(0, (poolTemp - tempCircThreshold) × tempCircFactor)
effektiveLaufzeit = min(effektiveLaufzeit, tempCircMaxRuntime)
```

### Parameter

| Parameter | Typ | Default | Bereich | Beschreibung |
|---|---|---|---|---|
| `tempCircThreshold` | double | 24,0 °C | 0–40 °C | Ab dieser Temperatur wird die Laufzeit verlängert |
| `tempCircFactor` | uint16_t | 30 min/°C | 0–120 min/°C | Verlängerung **pro °C** über dem Schwellwert |
| `tempCircMaxRuntime` | uint16_t | 720 min | 60–1440 min | Absolute Obergrenze der Gesamtlaufzeit |

### Berechnete Felder (nicht konfigurierbar)

| Feld | Beschreibung |
|---|---|
| `effectiveRuntime` | Berechnete Laufzeit in Minuten (für Status/Logging) |
| `effectiveEndTime` | Resultierende Ausschaltzeit (Uhrzeit) |

## 3. Beispiele

### Sommer (Pool 30 °C)

```
Timer:         10:00 – 18:00  (Basis = 480 min)
Schwellwert:   24 °C
Differenz:     6 °C
Verlängerung:  6 × 30 = 180 min
Effektiv:      480 + 180 = 660 min
Ausschaltzeit: 10:00 + 660 min = 21:00 Uhr
```

→ Die Pumpe läuft 3 Stunden länger. Bei Hitze wird mehr umgewälzt, Algen haben keine Chance.

### Übergang (Pool 22 °C)

```
Timer:         10:00 – 18:00
Schwellwert:   24 °C
Differenz:     22 < 24 → Keine Verlängerung
Effektiv:      = Basis = 480 min
Ausschaltzeit: 18:00 Uhr
```

→ Normale Laufzeit, kein Nachteil.

### Kühler Sommer (Pool 20 °C)

```
Timer:         10:00 – 18:00
Schwellwert:   24 °C
Differenz:     20 < 24 → Keine Verlängerung
Effektiv:      = Basis = 480 min
```

→ Auch hier normale Laufzeit. Eine optionale **Kürzung** bei Kälte ist für Version 1 nicht vorgesehen, könnte aber später ergänzt werden.

### Heiß (Pool 34 °C, Max-Runtime greift)

```
Timer:         10:00 – 18:00
Differenz:     10 °C
Verlängerung:  10 × 30 = 300 min
Zwischensumme: 480 + 300 = 780 min
Max-Runtime:   720 min (12 h)
Effektiv:      min(780, 720) = 720 min
Ausschaltzeit: 10:00 + 720 min = 22:00 Uhr
```

→ Die Obergrenze verhindert übermäßigen Pumpenverschleiß.

## 4. Integration ins System

### Betroffene Komponenten

```
ConfigManager (ControllerSettings)
  + tempCircThreshold: double    — Schwellwert in °C
  + tempCircFactor: uint16_t     — Minuten Verlängerung pro °C
  + tempCircMaxRuntime: uint16_t — Obergrenze in Minuten

RuleTimer / RuleAuto
  + calculateEffectiveRuntime()  — gemeinsame Hilfsfunktion
  + Nutzung der berechneten Laufzeit statt fester Endzeit

MqttPublisher
  + 3 neue Number-Entities per HA Discovery
  + Command-Handler für /temp-circ-*/set

WebPortal (später)
  + Eingabefelder im Config-Tab
```

### Timer-Berechnung (Kernlogik)

```cpp
uint16_t calculateEffectiveRuntime(uint16_t baseMinutes, float poolTemp) {
    auto &s = ConfigManager::getSettings();

    // Bei NaN oder unter Schwellwert: Basis-Laufzeit
    if (poolTemp != poolTemp || poolTemp <= s.tempCircThreshold) {
        return baseMinutes;
    }

    float diff = poolTemp - s.tempCircThreshold;
    float extraMinutes = diff * s.tempCircFactor;

    // Auf ganze Minuten runden
    uint16_t extra = static_cast<uint16_t>(extraMinutes + 0.5f);
    uint16_t total = baseMinutes + extra;

    // Obergrenze anwenden
    return (total > s.tempCircMaxRuntime) ? s.tempCircMaxRuntime : total;
}
```

### Midnight-Crossing

Die Timer-Logik in `checkPoolPumpTimer()` unterstützt bereits Endzeiten, die über Mitternacht hinausgehen (z. B. Start 22:00, Ende 02:00). Die temperatur-basierte Verlängerung kann denselben Mechanismus nutzen — die effektive Endzeit wird nicht auf 24:00 begrenzt.

### NVS-Persistenz

Die 3 Parameter werden im `ControllerSettings`-Struct über NVS gespeichert. Sie werden selten geändert (Konfiguration), daher kein Wear-Problem.

## 5. HA-Integration (MQTT Discovery)

Für jeden Parameter wird ein Number-Entity mit HA Discovery published:

| Entity | Topic-Suffix | Min | Max | Step |
|---|---|---|---|---|
| `temp-circ-threshold` | `/number/pool-controller/temp-circ-threshold/…` | 0 | 40 | 0,5 |
| `temp-circ-factor` | `/number/pool-controller/temp-circ-factor/…` | 0 | 120 | 5 |
| `temp-circ-max-runtime` | `/number/pool-controller/temp-circ-max-runtime/…` | 60 | 1440 | 15 |

Zusätzlich ein Sensor für die berechnete effektive Laufzeit:

| Entity | Topic-Suffix | Unit |
|---|---|---|
| `effective-runtime` | `/sensor/pool-controller/effective-runtime/…` | min |

## 6. Umsetzungsplan

| Schritt | Datei(en) | Aufwand |
|---|---|---|
| 1. Config-Struct erweitern | `ConfigManager.hpp` | ~10 min |
| 2. Helper-Funktion + Defaults | `Timer.hpp`, `Timer.cpp` | ~15 min |
| 3. RuleTimer::loop() anpassen | `RuleTimer.cpp` | ~20 min |
| 4. RuleAuto::loop() anpassen | `RuleAuto.cpp` | ~10 min |
| 5. MQTT Discovery + Handler | `MqttPublisher.cpp` | ~30 min |
| 6. Build + Test | — | ~10 min |
| **Gesamt** | | **~1,5 h** |

## 7. Mögliche Erweiterungen (v2)

- **Kürzung bei Kälte**: Zusätzlicher Parameter `tempCircMinRuntime`, der bei Unterschreitung eines Schwellwerts die Laufzeit reduziert
- **Trend-basiert**: Berücksichtigung des Temperatur-Trends (steigend/falend), nicht nur des Absolutwerts
- **Saisonaler Kalender**: Automatische Anpassung der Basis-Parameter an Sommer/Winter
