# Bedien- und Navigationskonzept — NORVI IIOT-AE01-R OLED + 3 Tasten

> **Status:** Konzept / RFC  
> **Ziel:** Verbesserung der UX des 128×64 OLED-Displays mit den drei Fronttasten (S1/S2/S3)  
> **Behandelt:** Button-Mapping, Bildschirm-Hierarchie, Hints, Wizard-Navigation, Sonderfälle

---

## 1. Physikalische Gegebenheiten

- **Display:** 0,96″ SSD1306 OLED, 128×64 Pixel, I2C (Adresse 0x3C)
- **Tasten:** 3 Tact-Switches rechts neben dem Display (vertikale Anordnung)
  - **S1 (oben)** — ▲
  - **S2 (Mitte)** — ▼
  - **S3 (unten)** — ●
- **Tasten-Interface:** Resistor-Ladder an GPIO32 (ADC), kein Interrupt — Polling alle 50 ms
- **Long-Press:** Erkennung nach >2 s, kann Short-Press konsumieren (Callback-Rückgabe `true`)

---

## 2. Ist-Analyse: UX-Probleme der aktuellen Implementierung

### 2.1 S3 ist überladen und gefährlich

| Kontext | S3-Kurz (aktuell) | Problem |
|---------|-------------------|---------|
| MAIN    | Betriebsmodus cyclen (auto→manu→boost→timer) | Ein Tastendruck ändert unumkehrbar den Betriebsmodus. Ein versehentlicher Druck während des Vorbeiscrollens schaltet von `auto` auf `manu` — die Pumpe läuft dann dauerhaft. |
| Andere Infoseiten | Betriebsmodus cyclen | Der Nutzer erwartet auf PAGE 3 (QRCODE) keine Modus-Änderung. |
| SENSOR_SETUP | Wizard-Advance | Einziger Kontext, in dem S3 sinnvoll ist. |

**➜ S3 darf auf Infoseiten keine Seiteneffekte haben, die den Anlagenbetrieb verändern.**

### 2.2 Hint-Labels „nxt" für S1 und S2

Beide Tasten zeigen auf normalen Seiten das Label `nxt`. Damit ist nicht ersichtlich, dass S1 **zurück** und S2 **vor** blättert. Der Nutzer muss durch Trial & Error herausfinden, welche Taste wohin navigiert.

### 2.3 Fehlende visuelle Hierarchie

- Keine Unterscheidung zwischen „Info-Seiten" und „Interaktions-Seiten"
- Kein Hinweis, dass man sich auf Seite X von Y befindet (nur eine nackte Zahl rechts unten)
- Kein „Exit"- oder „Back"-Gefühl — einmal im Sensor-Wizard steckt man fest, bis man entweder fertig ist oder 60 s gewartet hat

### 2.4 Long-Press ist unsichtbar

- S3 lang speichert Sensor-Mapping und rebootet — aber der Nutzer sieht das nirgends, solange nicht beide Sensoren zugewiesen sind.
- Es gibt kein konsistentes Long-Press-Modell, das der Nutzer lernen und erwarten kann.

### 2.5 Auto-Return ohne Vorwarnung

- Nach 60 s springt das Display unhinterfragt zurück zu MAIN — das kann mitten im Lesen einer IP-Adresse passieren.

---

## 3. Design-Prinzipien

1. **Konsistenz:** Jede Taste hat eine gleichbleibende Grundbedeutung (S1=▲=rauf/zurück, S2=▼=runter/vor, S3=●=Aktion/bestätigen).
2. **Sicherheit:** Keine Taste ändert den Anlagenzustand ohne explizite Bestätigung oder klares Label.
3. **Sichtbarkeit:** Jede Taste hat ein kontextspezifisches, eindeutiges Hint-Label.
4. **Minimale Überraschung:** Keine Side-Effects beim Scrollen. Aktionen sind explizit.
5. **Fat-Finger-freundlich:** Wichtige Aktionen erfordern zwei Schritte oder Long-Press.

---

## 4. Vorgeschlagenes Button-Mapping

### 4.1 Grundzuordnung (gilt als Default)

| Taste | Label | Kurz (Normal) | Kurz (Wizard) | Lang (>2 s) |
|-------|-------|---------------|---------------|-------------|
| **S1** | ▲ | Vorherige Seite | Vorherige Option | — (nicht belegt) |
| **S2** | ▼ | Nächste Seite | Nächste Option | — (nicht belegt) |
| **S3** | ● | Aktion / Enter (siehe Seite) | Bestätigen / Weiter | Kontext-Aktion (speichern, reboot) |

S1 und S2 sind **immer** Navigation — egal ob zwischen Seiten oder innerhalb eines Wizards.  
S3 ist **immer** die Aktionstaste — was sie tut, steht im Hint-Label.

### 4.2 Hint-Labels pro Seite

| Seite | S1-Hint | S2-Hint | S3-Hint | S3-Lang |
|-------|---------|---------|---------|---------|
| MAIN | — | next | menu | — |
| NETWORK | prev | next | — | — |
| SYSTEM | prev | next | — | — |
| QRCODE | prev | next | — | — |
| WIFI_SETUP | prev | next | — | — |
| SENSOR_SETUP (IDLE) | — | — | setup | save+reboot |
| SENSOR_SETUP (SELECT) | up | dn | select | — |
| SENSOR_SETUP (ROLE) | up | dn | assign | — |

**Änderungen:**
- S1 zeigt `prev` statt `nxt` — eindeutig anders als S2 (`next`)
- S3 zeigt **nie** `ok` auf Infoseiten, sondern ist entweder leer (keine Aktion) oder zeigt klar, was passiert (`menu`, `setup`, `select`, `assign`)
- MAIN-Seite: S3 öffnet ein Aktionsmenü statt direkt den Modus zu wechseln

---

## 5. Bildschirm-Hierarchie

```
┌─────────────────────────────────────┐
│         INFO-MODUS                   │
│  (Seiten: MAIN / NETWORK / SYSTEM / │
│   QRCODE / WIFI_SETUP)              │
│                                     │
│  S1/S2 = prev/next                  │
│  S3    = kontextspezifisch          │
│  Auto-Return nach 60 s → MAIN      │
└──────────────────┬──────────────────┘
                   │ S3 in MAIN
                   ▼
┌─────────────────────────────────────┐
│         AKTIONSMENÜ (MAIN)          │
│                                     │
│  > Mode: auto                       │
│    Pump: on/off                     │
│    → Back                           │
│                                     │
│  S1/S2 = Menü-Zeile wechseln       │
│  S3    = Auswahl aktivieren         │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│      SENSOR-SETUP-WIZARD            │
│  (erreichbar via S3 auf Seite       │
│   SENSOR_SETUP)                     │
│                                     │
│  Step 1: SELECT_SENSOR              │
│  S1/S2 = up/dn (Sensor wählen)      │
│  S3    = select (bestätigen)        │
│                                     │
│  Step 2: SELECT_ROLE                │
│  S1/S2 = up/dn (Solar/Pool)         │
│  S3    = assign (zuweisen)          │
│                                     │
│  Nach assign → zurück zu IDLE       │
│  S3 lang = save+reboot (wenn beide  │
│            Sensoren zugewiesen)     │
└─────────────────────────────────────┘
```

---

## 6. Detaillierte Seitenbeschreibungen

### 6.1 MAIN — Übersichtsseite

```
┌──────────────────────┬──────┐
│ ● 22.5°C             │      │
│ Pool       auto      │ S2 ▼│ next
│ ○ 19.1°C             │ S3 ●│ menu
│ Solar                │      │
├──────────────────────┴──────┤
│ AUTO  12:30  v2.1.0     1  │
└─────────────────────────────┘
```

- S2 = `next` → nächste Info-Seite (NETWORK)
- S3 = `menu` → öffnet Aktionsmenü (siehe 6.6)
- S1 = kein Hint, da MAIN die erste Seite ist (kein „prev" nötig)

### 6.2 NETWORK — Netzwerkstatus

```
┌──────────────────────┬──────┐
│ Network Status       │      │
│──────────────────────│ S1 ▲│ prev
│ WiFi: MyHomeWLAN     │ S2 ▼│ next
│ IP: 192.168.1.42     │ S3  │
│ MQTT: CONNECTED      │      │
├──────────────────────┴──────┤
│ AUTO  12:30  v2.1.0     2  │
└─────────────────────────────┘
```

- S1/S2 = Seiten blättern (wrap-around über MAIN–QRCODE)
- S3 = kein Hint — kein Side-Effect

### 6.3 SYSTEM — Systeminformationen

```
┌──────────────────────┬──────┐
│ System Info          │      │
│──────────────────────│ S1 ▲│ prev
│ Up: 12d 5h 32m       │ S2 ▼│ next
│ Heap: 123456 B       │ S3  │
│ FW: v2.1.0           │      │
│ Min: 98000 B         │      │
├──────────────────────┴──────┤
│ AUTO  12:30  v2.1.0     3  │
└─────────────────────────────┘
```

### 6.4 QRCODE — Web-Interface QR

```
┌──────────────────────┬──────┐
│ http://192.168.1.42  │      │
│ ┌──────────────┐     │ S1 ▲│ prev
│ │  ██  ██  ██  │     │ S2 ▼│ next
│ │  ██  ██  ██  │     │ S3  │
│ │  ██  ██  ██  │     │      │
│ └──────────────┘     │      │
├──────────────────────┴──────┤
│ AUTO  12:30  v2.1.0     4  │
└─────────────────────────────┘
```

### 6.5 WIFI_SETUP — Ersteinrichtung

Identisch zu QRCODE im Layout, aber mit zusätzlichem Text zur WiFi-Einrichtung.

### 6.6 Aktionsmenü (auf MAIN via S3)

```
┌──────────────────────┬──────┐
│ > Mode: auto ◄       │      │
│   Pump: on/off       │ S1 ▲│
│   → Exit menu        │ S2 ▼│
│                      │ S3 ●│ select
│                      │      │
├──────────────────────┴──────┤
│ AUTO  12:30  v2.1.0     1  │
└─────────────────────────────┘
```

| Menü-Eintrag | S3-Aktion |
|-------------|-----------|
| **Mode: auto** | Modus cyclen (auto→manu→boost→timer→auto) + sofort zurück zu MAIN |
| **Pump: on/off** | Pumpe manuell ein/aus + zurück zu MAIN |
| **→ Exit menu** | Zurück zu MAIN ohne Änderung |

S1/S2 navigieren zwischen den Menüzeilen. S3 bestätigt die Auswahl.

**Warum ein Menü statt direkter Modus-Cycle?**
- Der Nutzer muss aktiv ins Menü gehen → kein versehentlicher Modus-Wechsel beim Blättern
- Das Label `menu` auf S3 ist eindeutig und erwartbar
- Langfristig können hier weitere Aktionen ergänzt werden (Pumpe togglen, Relais-Status, Neustart)

### 6.7 SENSOR_SETUP — Sensor-Zuordnungswizard

**Step 1 — SELECT_SENSOR:**

```
┌──────────────────────┬──────┐
│ Sensor Setup         │      │
│──────────────────────│ S1 ▲│ up
│ ▓0: 28FFAABB  <-SOLAR│ S2 ▼│ dn
│   22.5°C        ◄    │ S3 ●│ select
│ 1: 28FECD12          │      │
│   19.1°C             │      │
├──────────────────────┴──────┤
│ S1/S2=pick  S3=select       │
└─────────────────────────────┘
```

**Step 2 — SELECT_ROLE:**

```
┌──────────────────────┬──────┐
│ Sensor Setup         │      │
│──────────────────────│ S1 ▲│ up
│ 0: 28FFAABB  SOLAR   │ S2 ▼│ dn
│ ┌──────────────────┐ │ S3 ●│ assign
│ │ Assign as:       │ │      │
│ │ ▓Solar           │ │      │
│ │   Pool           │ │      │
│ └──────────────────┘ │      │
├──────────────────────┴──────┤
│ S1/S2=role  S3=assign       │
└─────────────────────────────┘
```

**IDLE-Zustand (beide zugewiesen):**

```
┌──────────────────────┬──────┐
│ Sensor Setup         │      │
│──────────────────────│      │
│ 0: 28FFAABB  SOLAR   │ S3 ●│ setup
│ 1: 28FECD12  POOL    │      │
│                      │ S3  │
│                      │  lang│ save+reboot
├──────────────────────┴──────┤
│ Both set. Hold S3=Save      │
└─────────────────────────────┘
```

---

## 7. Hint-Bar-Design (rechte Spalte)

Die Hint-Bar nutzt die rechten ~30 Pixel (Spalten 98–127). Sie wird auf allen Info-Seiten gezeichnet.

```
Spalte:  98     108    118    128
         ┌──────┬──────┬──────┐
         │      │      │      │
    S1   │ prev │  ▲   │      │   <- Text, darunter Dreieck-Symbol
         │      │      │      │
   Trenner│      │      │      │   <- vertikale Linie
         │      │      │      │
    S2   │ next │  ▼   │      │
         │      │      │      │
   Trenner│      │      │      │
         │      │      │      │
    S3   │ menu │  ●   │      │
         │      │      │      │
         └──────┴──────┴──────┘
```

**Gestaltungsregeln:**
- S1 ist oben, S2 mitte, S3 unten — exakt wie die physikalische Anordnung
- Das Label steht **links** vom Symbol, damit es gelesen wird bevor das Auge zum Symbol wandert
- Wenn eine Taste keine Aktion hat, wird sie ausgeblendet (kein Text, kein Symbol)
- Symbole: ▲ (S1), ▼ (S2), ● (S3) — auch in der Lücke ohne Text erkennbar
- Die Farbgebung bleibt monochrom (SSD1306_WHITE)

---

## 8. Sonderfälle & Edge Cases

### 8.1 Erster Boot (kein WiFi / keine Sensor-Mapping)

- Display startet automatisch auf **WIFI_SETUP** oder **SENSOR_SETUP** (bereits implementiert)
- Button-Verhalten ist identisch zu den normalen Seiten — S1/S2 blättern, S3 hat keine Aktion außer auf SENSOR_SETUP
- Der Nutzer muss zwingend ins Web-Interface — das Display macht das klar („Scan QR or enter URL")

### 8.2 Auto-Return-Timeouts

| Zustand | Timeout | Ziel |
|---------|---------|------|
| Info-Seite (nicht MAIN) | 60 s | MAIN |
| Aktionsmenü | 30 s | MAIN |
| Sensor-Wizard (SELECT) | 120 s | IDLE+MAIN |
| Sensor-Wizard (ROLE) | 120 s | IDLE+MAIN |

- **Neu:** 5 s vor Auto-Return wird der Bildschirm nicht sofort umgeschaltet — stattdessen läuft das Menü einfach aus. Der Timeout wird nur **zwischen** Tastendrücken gemessen. Solange der Nutzer aktiv ist, passiert nichts.

### 8.3 Wizard-Cancel / Rückzug

Im Sensor-Wizard gibt es aktuell keinen Weg zurück, außer abzubrechen (Timeout).  
**Vorschlag:** S1 auf der ersten Wizard-Stufe (SELECT_SENSOR) zeigt `back`, wenn `setupSelectedDev_ == 0` und der Nutzer nochmal S1 drückt → zurück zu IDLE.  
(Das erfordert minimale Logik-Änderung in `setupSelectPrevious()`.)

### 8.4 Long-Press-Modell

Long-Press wird nur für **kritische, bestätigungspflichtige Aktionen** genutzt:
- Sensor-Mapping speichern + Reboot
- Zukünftig: Factory Reset, Safe Mode

Optimalerweise wird Long-Press auf S3 **auf allen Seiten** konsistent als „erweiterte Aktion" etabliert.

### 8.5 QR-Code-Seiten mit vollem Bildschirm

Auf QRCODE und WIFI_SETUP wird die Hint-Bar aktuell ausgeblendet, um Platz für den QR-Code zu schaffen.
Das ist akzeptabel, da der QR-Code die primäre Interaktion ist und der Nutzer auf diesen Seiten typischerweise nicht navigiert, sondern scannt.

---

## 9. Umsetzungsplan (Code-Änderungen)

### 9.1 Hint-Labels korrigieren

**Dateien:** `NorviOledDisplay.cpp` — Funktion `drawButtonHints()`

| Change | Aktuell | Neu |
|--------|---------|-----|
| S1-Label auf Info-Seiten | `nxt` | `prev` |
| S2-Label auf Info-Seiten | `nxt` | `next` |
| S3 auf MAIN | `ok` | `menu` |
| S3 auf NETWORK/SYSTEM/QRCODE/WIFI_SETUP | `ok` | leer (ausblenden) |

### 9.2 S3-Modus-Cycle entfernen, Aktionsmenü einbauen

**Datei:** `PoolController.cpp` — Callback S3

Statt direktem Mode-Cycle:
1. S3 auf MAIN → setzt internen `menuActive_`-Flag
2. Display zeigt Menü-Overlay (3 Zeilen: Mode, Pump, Exit)
3. S1/S2 navigieren im Menü
4. S3 bestätigt Auswahl
5. S3 (oder Exit) → zurück zu MAIN

Separate kleine State-Machine in `NorviOledDisplay`:
```cpp
enum class MenuItem : uint8_t {
  MODE,
  PUMP,
  EXIT
};
static MenuItem menuSelection_;
static bool menuActive_;
```

### 9.3 Wizard-Cancel

**Datei:** `NorviOledDisplay.cpp` — `setupSelectPrevious()`

Wenn `setupSelectedDev_ == 0` und nochmal `previous()` → zurücksetzen auf IDLE.

```cpp
void NorviOledDisplay::setupSelectPrevious() {
  if (setupSelectedDev_ == 0 && setupStep_ == SetupStep::SELECT_SENSOR) {
    // Back to IDLE
    setupStep_ = SetupStep::IDLE;
    forceRedraw_ = true;
    return;
  }
  // ... existing logic ...
}
```

### 9.4 Footer-Zeile optimieren

**Datei:** `NorviOledDisplay.cpp` — `drawFooter()`

- Page-Zahl rechts: aktuell "1"–"6" statt "1/6"–"6/6" (oder Punkte-Dots)
- Vorschlag: `◉○○○○○` statt Zahl (füllt weniger Platz und ist intuitiver)
- Oder Zahl mit Max: `1/6`, `2/6` etc. (max. 4 Zeichen)

### 9.5 Hint-Bar-Code aufräumen

Die aktuelle `drawButtonHints()` hat 3×3 verschachtelte Cases. Besser:

```cpp
struct ButtonHint {
  const char *label;
  bool hasIcon;
};
ButtonHint hints[3];
// ... fill based on page + setup step ...
// ... render loop ...
```

---

## 10. Offene Fragen / Diskussion

1. **S3 im Aktionsmenü:** Soll die Modus-Änderung sofort生效 oder erst beim Verlassen des Menüs?
   - Vote: **Sofort** — das Menü ist nur Auswahl, kein Formular. Nach Auswahl → zurück zu MAIN.
2. **Belegung S3-Lang auf Info-Seiten:** Aktuell nur im Sensor-Kontext sinnvoll. Soll S3-Lang auf MAIN einen Factory Reset / Reboot anbieten?
   - Vote: **Später** — erstmal nur Sensor-Speichern.
3. **S1 auf MAIN:** Soll S1 von MAIN aus direkt zur letzten Seite springen (wrap) oder deaktiviert sein?
   - Vote: **Deaktiviert** — kein Hint, keine Aktion. Der Nutzer kann nur vorwärts.
4. **Auto-Return-Timeouts:** Sollen die unterschiedlich lang sein je nach Seite?
   - Vote: **Einheitlich 60 s** — einfacher zu verstehen. Nur Menü und Wizard bekommen eigene Timeouts (30 s / 120 s).

---

## 11. Visual Summary

```
┌─────────────────────────────────────────────────────┐
│                  NAVIGATIONSKONZEPT                   │
│                                                       │
│   Tasten       Infoseiten         Wizard / Menü       │
│  ┌──────┐     ┌──────────┐      ┌──────────┐        │
│  │ S1 ▲ │ ──→ │ prev     │      │ up/back  │        │
│  │──────│     │ Seite◄   │      │ Option◄  │        │
│  │ S2 ▼ │ ──→ │ next     │      │ down     │        │
│  │──────│     │ Seite►   │      │ Option►  │        │
│  │ S3 ● │ ──→ │ menu     │      │ select   │        │
│  │      │     │ (MAIN)   │      │ assign   │        │
│  │  lang│ ──→ │ save     │      │ save+    │        │
│  └──────┘     │ (Sensor) │      │ reboot   │        │
│               └──────────┘      └──────────┘        │
│                                                       │
│   FAQ: "prev" auf S1 ≠ "next" auf S2                 │
│   → Klare Richtungshinweise, kein "nxt" mehr         │
│                                                       │
│   S3 nie 'ok' auf Info-Seiten                         │
│   → Entweder 'menu' (MAIN) oder leer                 │
│                                                       │
│   Aktion erfordert immer zwei Schritte:               │
│   S3→menu → S1/S2→Auswahl → S3→Bestätigung           │
└─────────────────────────────────────────────────────┘
```

---

> **Nächste Schritte:** Review des Konzepts, dann Implementierung in folgenden PRs:
> 1. Hint-Labels korrigieren + Footer verbessern (niedriges Risiko)
> 2. S3-Aktionsmenü einbauen (mittleres Risiko, neue State-Machine)
> 3. Wizard-Cancel + Long-Press-Modell (mittleres Risiko)
