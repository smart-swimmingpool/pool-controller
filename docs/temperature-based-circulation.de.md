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

### Verhalten bei Temperaturänderungen während des Laufs

Die Berechnung läuft **kontinuierlich** (jede `loop()`), aber die Endzeit darf sich **nur nach hinten** verschieben. Sobald die Pumpe läuft, wird die höchste jemals berechnete Endzeit gemerkt.

```
Start (10:00, 24 °C):   Ende = 10:00 + 480 min = 18:00
14:00, 28 °C → +4×30:   neueEnde = max(18:00, 20:00) = 20:00 ✓
16:00, 26 °C → +2×30:   neueEnde = max(20:00, 19:00) = 20:00 ⛔ bleibt
```

**Regel:** Die Pumpe schaltet **frühestens** zur ursprünglichen Timer-Endzeit aus — auch wenn es später wieder abkühlt. Dadurch gibt es keine Überraschungen und die Pumpe springt nicht unerwartet aus.

```cpp
// Kernlogik in RuleTimer/RuleAuto::loop():
uint16_t newEndMinutes = calculateEffectiveEndMinutes(
    baseStartMinutes, baseEndMinutes, poolTemp);

// Nur verlängern, nie verkürzen:
if (newEndMinutes > activeEndMinutes) {
    activeEndMinutes = newEndMinutes;
}
```

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

## 7. Benutzerhandbuch

### 7.1 Schnellstart

1. **Timer einstellen** — Die temperaturabhängige Filterlaufzeit funktioniert nur im **Timer**- oder **Auto**-Modus mit einem konfigurierten Zeitfenster. Stelle zuerst die Basis-Filterzeit ein (z. B. 10:00–18:00 Uhr).
2. **Schwellwert setzen** — Starte mit `24 °C`. Unterhalb dieser Temperatur läuft die Pumpe normal nach Timer.
3. **Faktor einstellen** — Starte mit `30 min/°C`. Jedes Grad über dem Schwellwert verlängert die Laufzeit um 30 Minuten.
4. **Maximale Laufzeit festlegen** — Starte mit `720 min (12 h)`. Verhindert übermäßige Laufzeiten an extrem heißen Tagen.

Fertig — die Pumpe läuft jetzt bei warmem Wasser automatisch länger.

> **Faustregel:** Einmal konfiguriert arbeitet das Feature vollautomatisch. Der **Circ. Runtime**-Wert auf dem Dashboard zeigt jederzeit die berechnete Laufzeit an.

### 7.2 Empfohlene Einstellungen nach Pool-Typ

Finde deine Pool-Situation und nutze diese Werte als Ausgangspunkt:

| Pool-Typ | Schwelle | Faktor | Max-Laufzeit | Warum |
|---|---|---|---|---|
| ☀️ **Kleiner Aufstellpool** (≤15 m³) | 22 °C | 20 min/°C | 480 min (8 h) | Weniger Volumen, weniger Reserve nötig |
| 🏊 **Mittlerer Familienpool** (25–50 m³) | 24 °C | 30 min/°C | 720 min (12 h) | Gute Balance aus Wasserqualität und Kosten |
| 🌴 **Großer Pool / hohe Belastung** (50+ m³) | 22 °C | 40 min/°C | 960 min (16 h) | Mehr Umwälzung für Hygiene nötig |
| 🌡️ **Beheizter Pool** | 26 °C | 15 min/°C | 600 min (10 h) | Wird ohnehin mehr gefiltert (Heizungspumpe) |

Bei solarem Heizen sollte der Schwellwert **mindestens** auf oder über der Solar-Minimumtemperatur liegen, um Überlappungen zu vermeiden.

### 7.3 Konfiguration über das Web-Interface

#### Pool-Tab

Öffne das Einstellungsmenü (Zahnrad) → **Pool** → Abschnitt **🌡️ Temperature-Based Circulation**.

| Feld | Beschreibung |
|---|---|
| **Circ. Temp Threshold** | Pooltemperatur muss diesen Wert überschreiten, damit die Verlängerung greift. |
| **Circ. Temp Factor** | Zusätzliche Minuten pro °C über dem Schwellwert. Höher = aggressivere Verlängerung. |
| **Circ. Max Runtime** | Absolute Obergrenze der täglichen Pumpenlaufzeit (inkl. Verlängerung). |

Klicke unten **Save Pool Settings**.

#### Dashboard

Ganz unten auf dem Dashboard zeigt **Circ. Runtime** die aktuell berechnete effektive Laufzeit in Minuten. Dieser Wert aktualisiert sich live — die `only-extend`-Regel merkt sich den jeweils höchsten Wert.

### 7.4 Konfiguration über Home Assistant

Nach MQTT-Verbindung erscheinen drei Number-Entities in Home Assistant:

| Entity | Zweck |
|---|---|
| `number.pool_controller_temp_circ_threshold` | Schwellwert einstellen |
| `number.pool_controller_temp_circ_factor` | Verlängerungsfaktor anpassen |
| `number.pool_controller_temp_circ_max_runtime` | Obergrenze setzen |
| `sensor.pool_controller_effective_runtime` | Nur-Lesen — zeigt berechnete Laufzeit |

Änderungen wirken sofort — kein Neustart nötig.

### 7.5 Beispiel: Mit Home Assistant automatisieren

**Szenario:** Faktor bei extremen Temperaturen automatisch erhöhen.

```yaml
automation:
  - alias: "Faktor bei Hitzewelle erhöhen"
    trigger:
      platform: numeric_state
      entity_id: weather.home
      attribute: temperature
      above: 32
    action:
      service: number.set_value
      target:
        entity_id: number.pool_controller_temp_circ_factor
      data:
        value: 45
```

**Szenario:** Benachrichtigung bei hoher Filterlaufzeit.

```yaml
automation:
  - alias: "Hohe Filterlaufzeit melden"
    trigger:
      platform: numeric_state
      entity_id: sensor.pool_controller_effective_runtime
      above: 700
    action:
      service: notify.mobile_app_phone
      data:
        message: "Pool-Filterlaufzeit nahe Maximum ({{ states('sensor.pool_controller_effective_runtime') }} min)"
```

### 7.6 FAQ / Problemlösung

> **F: Die Pumpe läuft viel länger als erwartet — woran liegt das?**

Prüfe die Pooltemperatur. Liegt sie über dem Schwellwert und der Faktor ist hoch eingestellt, sind längere Laufzeiten normal. Der **Circ. Runtime**-Wert zeigt die berechnete Verlängerung. Reduziere bei Bedarf den **Faktor** oder erhöhe den **Schwellwert**.

> **F: Die Pumpe hat früher ausgeschaltet als erwartet — warum?**

Die Verlängerung gilt nur, während die Pumpe aktiv ist. War sie schon länger an, wurde die Endzeit vielleicht früher verlängert, aber das Wasser ist inzwischen abgekühlt — die `only-extend`-Regel behält den höchsten Wert. Prüfe:
1. Liegt die Pooltemperatur tatsächlich über dem Schwellwert?
2. Ist das Timer-Fenster konfiguriert? (Die temperatur-basierte Verlängerung schaltet die Pumpe nicht selbst ein.)

> **F: Kann die Pumpe früher starten als der Timer?**

Nein. Die Funktion verlängert nur die **Endzeit**. Die Startzeit bleibt exakt wie im Timer eingestellt. (Siehe `only-extend`-Regel in §2.)

> **F: Setzt sich die Max-Laufzeit jeden Tag zurück?**

Ja. Zu Beginn jedes Timer-Zyklus wird die effektive Endzeit auf die Basis-Timer-Zeit zurückgesetzt. Die Verlängerung wird dann neu berechnet.

> **F: Ich habe Werte im Web-UI geändert, aber nichts passiert — warum?**

Prüfe den **Modus** auf dem Dashboard. Die temperaturabhängige Laufzeit funktioniert nur im **Timer**- und **Auto**-Modus. Im **Manuell**- oder **Boost**-Modus ist die Funktion deaktiviert.

> **F: Verschleißt das meine Pumpe schneller?**

Der Parameter `tempCircMaxRuntime` begrenzt die Laufzeit genau für diesen Zweck. 12 Stunden/Tag sind für die meisten Poolpumpen unbedenklich. Prüfe zur Sicherheit das Datenblatt deiner Pumpe (Einschaltdauer / duty cycle).

## 8. Mögliche Erweiterungen (v2)

- **Kürzung bei Kälte**: Zusätzlicher Parameter `tempCircMinRuntime`, der bei Unterschreitung eines Schwellwerts die Laufzeit reduziert
- **Trend-basiert**: Berücksichtigung des Temperatur-Trends (steigend/falend), nicht nur des Absolutwerts
- **Saisonaler Kalender**: Automatische Anpassung der Basis-Parameter an Sommer/Winter
