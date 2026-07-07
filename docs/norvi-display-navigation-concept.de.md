# Bedien- und Navigationskonzept — NORVI IIOT-AE01-R OLED + 3 Tasten

> **Status:** Konzept / RFC (Review v1.1, Juli 2026)  
> **Ziel:** Verbesserung der UX des 128×64 OLED-Displays mit den drei
> Fronttasten (S1/S2/S3)  
> **Behandelt:** Button-Mapping, Bildschirm-Hierarchie, Hints,
> Wizard-Navigation, Long-Press-Feedback, Sonderfälle

---

## 1. Physikalische Gegebenheiten

- **Display:** 0,96″ SSD1306 OLED, 128×64 Pixel, I2C (Adresse 0x3C)
- **Tasten:** 3 Tact-Switches rechts neben dem Display (vertikale Anordnung)
  - **S1 (oben)** — ▲
  - **S2 (Mitte)** — ▼
  - **S3 (unten)** — ●
- **Tasten-Interface:** Resistor-Ladder an GPIO32 (ADC), kein Interrupt
  — Polling alle 50 ms
- **Long-Press:** Erkennung nach >2 s, kann Short-Press konsumieren
  (Callback-Rückgabe `true`)

---

## 2. Ist-Analyse: UX-Probleme der aktuellen Implementierung

### 2.1 S3 ist überladen und gefährlich

| Kontext | S3-Kurz (aktuell) | Problem |
|---------|-------------------|---------|
| MAIN | Betriebsmodus cyclen (auto→manu→boost→timer) | Ein Tastendruck ändert unumkehrbar den Betriebsmodus. Ein versehentlicher Druck während des Vorbeiscrollens schaltet von `auto` auf `manu` — die Pumpe läuft dann dauerhaft. |
| Andere Infoseiten | Betriebsmodus cyclen | Der Nutzer erwartet auf QRCODE keine Modus-Änderung. |
| SENSOR_SETUP | Wizard-Advance | Einziger Kontext, in dem S3 sinnvoll ist. |

**➜ S3 darf auf Infoseiten keine Seiteneffekte haben,
die den Anlagenbetrieb verändern.**

### 2.2 Hint-Labels „nxt" für S1 und S2

Beide Tasten zeigen auf normalen Seiten das Label `nxt`. Damit ist nicht
ersichtlich, dass S1 **zurück** und S2 **vor** blättert. Der Nutzer muss
durch Trial & Error herausfinden, welche Taste wohin navigiert.

### 2.3 Fehlende visuelle Hierarchie

- Keine Unterscheidung zwischen „Info-Seiten" und „Interaktions-Seiten"
- Kein Hinweis, dass man sich auf Seite X von Y befindet (nur eine nackte
  Zahl rechts unten)
- Kein „Exit"- oder „Back"-Gefühl — einmal im Sensor-Wizard steckt man fest,
  bis man entweder fertig ist oder 60 s gewartet hat

### 2.4 Long-Press ist unsichtbar

- S3 lang speichert Sensor-Mapping und rebootet — aber der Nutzer sieht das
  nirgends, solange nicht beide Sensoren zugewiesen sind.
- Es gibt kein konsistentes Long-Press-Modell, das der Nutzer lernen und
  erwarten kann.
- **Keine visuelle Rückmeldung** während des Haltens — der Nutzer weiß nicht,
  ob der Druck registriert wurde oder wie lange er noch halten muss.

### 2.5 Auto-Return ohne Vorwarnung

- Nach 60 s springt das Display unhinterfragt zurück zu MAIN — das kann
  mitten im Lesen einer IP-Adresse passieren.

---

## 3. Design-Prinzipien

1. **Konsistenz:** Jede Taste hat eine gleichbleibende Grundbedeutung
   (S1=▲=rauf/zurück, S2=▼=runter/vor, S3=●=Aktion/bestätigen).
2. **Sicherheit:** Keine Taste ändert den Anlagenzustand ohne explizite
   Bestätigung oder klares Label.
3. **Sichtbarkeit:** Jede Taste hat ein kontextspezifisches, eindeutiges
   Hint-Label. Long-Press wird visuell begleitet.
4. **Minimale Überraschung:** Keine Side-Effects beim Scrollen. Aktionen
   sind explizit.
5. **Fat-Finger-freundlich:** Wichtige Aktionen erfordern zwei Schritte
   oder Long-Press mit Fortschrittsanzeige.

---

## 4. Vorgeschlagenes Button-Mapping

### 4.1 Grundzuordnung (gilt als Default)

| Taste | Label | Kurz (Normal) | Kurz (Wizard) | Lang (>2 s) |
|-------|-------|---------------|---------------|-------------|
| **S1** | ▲ | Vorherige Seite | Vorherige Option | — (nicht belegt) |
| **S2** | ▼ | Nächste Seite | Nächste Option | — (nicht belegt) |
| **S3** | ● | Aktion / Enter (siehe Seite) | Bestätigen / Weiter | Kontext-Aktion (speichern, reboot) |

S1 und S2 sind **immer** Navigation — egal ob zwischen Seiten oder
innerhalb eines Wizards.  
S3 ist **immer** die Aktionstaste — was sie tut, steht im Hint-Label.

### 4.2 Hint-Labels pro Seite

| Seite | S1 | S2 | S3 | S3-Lang |
|-------|----|----|----|---------|
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
- S3 zeigt **nie** `ok` auf Infoseiten, sondern ist entweder leer (keine
  Aktion) oder zeigt klar, was passiert (`menu`, `setup`, `select`,
  `assign`)
- MAIN-Seite: S3 öffnet ein Aktionsmenü statt direkt den Modus zu wechseln

### 4.3 Internationalisierung (i18n)

Die Label sind bewusst kurz (max. 10 Zeichen), damit sie in die 30px
rechte Spalte passen. Alternativvorschläge pro Sprache:

| EN (Default) | DE | FR | Zweck |
|-------------|----|----|-------|
| prev | zurück | préc | Rückwärts |
| next | vor | suiv | Vorwärts |
| menu | menü | menu | Aktionen öffnen |
| setup | einr | conf | Wizard starten |
| select | wahl | choix | Auswahl bestätigen |
| assign | setz | attr | Rolle zuweisen |
| save | spei | save | Speichern |

**Entscheidung:** Vorerst EN (geringster Speicherverbrauch, keine
zusätzliche Übersetzungslogik). Eine i18n-Erweiterung kann später
über PROGMEM-String-Tabellen nachgerüstet werden, falls das Display
auch in nicht-englischen Kontexten eingesetzt wird.

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
│  Step 1/2: SELECT_SENSOR            │
│  S1/S2 = up/dn (Sensor wählen)      │
│  S3    = select (bestätigen)        │
│  S1 am Ende → cancel               │
│                                     │
│  Step 2/2: SELECT_ROLE              │
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
│ AUTO  12:30  v2.1.0     1/5│
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
│ AUTO  12:30  v2.1.0     2/5│
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
│ AUTO  12:30  v2.1.0     3/5│
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
│ AUTO  12:30  v2.1.0     4/5│
└─────────────────────────────┘
```

### 6.5 WIFI_SETUP — Ersteinrichtung

Identisch zu QRCODE im Layout, aber mit zusätzlichem Text zur
WiFi-Einrichtung.

### 6.6 Aktionsmenü (auf MAIN via S3)

```
┌──────────────────────┬──────┐
│ > Mode: auto ◄       │      │
│   Pump: on/off       │ S1 ▲│
│   → Exit menu        │ S2 ▼│
│                      │ S3 ●│ select
│                      │      │
├──────────────────────┴──────┤
│ AUTO  12:30  v2.1.0     1/5│
└─────────────────────────────┘
```

| Menü-Eintrag | S3-Aktion |
|-------------|-----------|
| **Mode: auto** | Modus cyclen + zurück zu MAIN |
| **Pump: on/off** | Pumpe manuell ein/aus + zurück zu MAIN |
| **→ Exit menu** | Zurück zu MAIN ohne Änderung |

S1/S2 navigieren zwischen den Menüzeilen. S3 bestätigt die Auswahl.

**Warum ein Menü statt direkter Modus-Cycle?**
- Der Nutzer muss aktiv ins Menü gehen → kein versehentlicher Modus-Wechsel
  beim Blättern
- Das Label `menu` auf S3 ist eindeutig und erwartbar
- Langfristig können hier weitere Aktionen ergänzt werden (Pumpe togglen,
  Relais-Status, Neustart)
- Trade-off: ein Klick mehr für den Modus-Wechsel — gerechtfertigt durch
  die verhinderte Fehlbedienung

### 6.7 SENSOR_SETUP — Sensor-Zuordnungswizard

**Step 1/2 — SELECT_SENSOR (mit Schritt-Indikator):**

```
┌──────────────────────┬──────┐
│ Sensor Setup   [1/2] │      │
│──────────────────────│ S1 ▲│ up
│ ▓0: 28FFAABB  SOLAR  │ S2 ▼│ dn
│   22.5°C        ◄    │ S3 ●│ select
│ 1: 28FECD12          │      │
│   19.1°C             │      │
├──────────────────────┴──────┤
│ S1/S2=pick  S3=sel  L=cancel│
└─────────────────────────────┘
```

**Step 2/2 — SELECT_ROLE:**

```
┌──────────────────────┬──────┐
│ Sensor Setup   [2/2] │      │
│──────────────────────│ S1 ▲│ up
│ 0: 28FFAABB  SOLAR   │ S2 ▼│ dn
│ ┌──────────────────┐ │ S3 ●│ assign
│ │ Assign as:       │ │      │
│ │ ▓Solar           │ │      │
│ │   Pool           │ │      │
│ └──────────────────┘ │      │
├──────────────────────┴──────┤
│ S1/S2=role  S3=save  L=back│
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
│ ▓▓▓▓▓░░░ Hold S3=Save      │
└─────────────────────────────┘
```

---

## 7. Hint-Bar-Design (rechte Spalte)

### 7.1 Layout

Die Hint-Bar nutzt die rechten ~30 Pixel (Spalten 98–127). Sie wird auf
allen Info-Seiten gezeichnet.

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

### 7.2 Gestaltungsregeln

- S1 ist oben, S2 mitte, S3 unten — exakt wie die physikalische Anordnung
- Das Label steht **links** vom Symbol, damit es gelesen wird bevor das
  Auge zum Symbol wandert
- Wenn eine Taste keine Aktion hat, wird sie ausgeblendet (kein Text,
  kein Symbol)
- Symbole: ▲ (S1), ▼ (S2), ● (S3) — auch in der Lücke ohne Text erkennbar
- Die Farbgebung bleibt monochrom (SSD1306_WHITE)

### 7.3 Kontextsensitivität

Die Hint-Bar zeigt **nur** die aktuell relevanten Aktionen:

| Seite | Sichtbare Hints | Begründung |
|-------|----------------|-----------|
| MAIN | S2: next, S3: menu | S1 hat keine Funktion (erste Seite) |
| NETWORK–QRCODE | S1: prev, S2: next | S3 hat keine Funktion auf Info-Seiten |
| SENSOR_SETUP (aktiv) | S1: up, S2: dn, S3: select/assign | Wizard braucht alle drei |
| Aktionsmenü | S1: ▲, S2: ▼, S3: select | Menü-Navigation + Bestätigung |

### 7.4 Primary-Action-Hervorhebung

Die primäre Aktion (S3) kann durch einen **invertierten Hintergrund**
(ein gefülltes Rechteck hinter dem Label) hervorgehoben werden. Das
entspricht der existierenden `dspInvertedText()`-Funktion und benötigt
keine zusätzliche Farbe (OLED ist monochrom).

---

## 8. Long-Press: Visuelles Feedback

### 8.1 Problem

Aktuell gibt es **null visuelle Rückmeldung** beim Long-Press. Der Nutzer
hält S3 und weiß nicht, ob der Druck ankommt oder wie lange er noch halten
muss. Das ist besonders kritisch bei der Aktion „save + reboot".

### 8.2 Vorgeschlagenes Fortschritts-Banner

Statt eines Countdowns (zu viel Text für 128×64) wird ein **horizontaler
Fortschrittsbalken** unterhalb des Inhaltsbereichs, aber oberhalb des
Footers eingeblendet:

```
┌──────────────────────┬──────┐
│ Sensor Setup         │      │
│──────────────────────│      │
│ 0: 28FFAABB  SOLAR   │      │
│ 1: 28FECD12  POOL    │      │
│                      │      │
│ ▓▓▓▓▓░░░░░ save...  │      │   <- Fortschritt (wächst über 2 s)
├──────────────────────┴──────┤
│ Hold S3 to save & reboot    │
└─────────────────────────────┘
```

**Implementierung:**
- Balkenlänge wächst von 0 % auf 100 % über 2000 ms (LONG_PRESS_MS)
- Timer wird in der Display-loop() aktualisiert (forceRedraw während Press)
- Bei Erreichen von 100 % → Aktion auslösen + reboot
- Bei Loslassen vor 100 % → Balken verschwindet, keine Aktion

### 8.3 Alternative: Invertiertes Label

Bei sehr wenig Platz (z. B. im Aktionsmenü) reicht auch ein invertiertes
S3-Hint-Label, das nach >1 s von `save` zu `SAVE!` wechselt:

| Zeit | S3-Hint | Zustand |
|------|---------|---------|
| 0–1 s | save | Normal |
| 1–2 s | SAVE! | Invertiert (highlighted) |
| >2 s | Aktion feuert | — |

### 8.4 Konsistentes Long-Press-Modell

Long-Press wird nur für **kritische, bestätigungspflichtige Aktionen**
genutzt und **immer** mit visuellem Feedback:

- Sensor-Mapping speichern + Reboot
- Zukünftig: Factory Reset, Safe Mode, Neustart

Auf allen anderen Seiten hat S3-Lang keine Funktion (keine Überraschung).

---

## 9. Sonderfälle & Edge Cases

### 9.1 Erster Boot (kein WiFi / keine Sensor-Mapping)

- Display startet automatisch auf **WIFI_SETUP** oder **SENSOR_SETUP**
  (bereits implementiert)
- Button-Verhalten ist identisch zu den normalen Seiten — S1/S2 blättern,
  S3 hat keine Aktion außer auf SENSOR_SETUP
- Der Nutzer muss zwingend ins Web-Interface — das Display macht das klar

### 9.2 Auto-Return-Timeouts

| Zustand | Timeout | Ziel |
|---------|---------|------|
| Info-Seite (nicht MAIN) | 60 s | MAIN |
| Aktionsmenü | 30 s | MAIN (Menü schließt sich) |
| Sensor-Wizard (SELECT) | 120 s | IDLE + MAIN |
| Sensor-Wizard (ROLE) | 120 s | IDLE + MAIN |

**Verhalten:**
- Timeout wird nur **zwischen** Tastendrücken gemessen. Solange der Nutzer
  aktiv ist, passiert nichts.
- 5 s vor Auto-Return wird auf dem Display ein kurzer Hinweis eingeblendet:
  z. B. `→ MAIN in 5s` rechts unten im Footer (blinkend).

### 9.3 Wizard-Cancel / Rückzug

Im Sensor-Wizard gibt es aktuell keinen Weg zurück.

**Vorschlag 1 — S1-Cancel (bevorzugt):**
S1 auf der ersten Wizard-Stufe, wenn `setupSelectedDev_ == 0`, zeigt
`back` und setzt bei nochmaligem Drücken auf IDLE zurück.

```cpp
void NorviOledDisplay::setupSelectPrevious() {
  if (setupSelectedDev_ == 0 && setupStep_ == SetupStep::SELECT_SENSOR) {
    // Zurück zu IDLE
    setupStep_ = SetupStep::IDLE;
    setupSelectedDev_ = 0;
    forceRedraw_ = true;
    return;
  }
  // ... bestehende Logik ...
}
```

**Vorschlag 2 — S1-Lang-Cancel (Alternative):**
Falls S1-Kurz immer „vorherige Option" bleiben soll: S1 lang (>2 s) in
SELECT_SENSOR bricht ab → zurück zu IDLE. Vorteil: kein unbeabsichtigter
Cancel. Nachteil: inkonsistentes Long-Press-Modell (bisher nur S3).

**Entscheidung:** Vorschlag 1 (S1-Kurz am Ende der Liste). Einfach zu
implementieren, intuitiv („weiter zurück geht nicht → zurück zum Start").

### 9.4 Schritt-Indikator im Wizard

Der Wizard (SENSOR_SETUP) bekommt einen visuellen Schritt-Indikator:

- **Vorschlag:** `[1/2]` und `[2/2]` rechts neben dem Titel in der
  Titelzeile
- **Alternativ:** Drei kleine Punkte `●○○` / `○●○` / `○○●` — aber bei nur
  2 Schritten unnötig komplex
- **Umsetzung:** Minimaler Platzbedarf (ca. 25 px in der oberen Zeile)

### 9.5 QR-Code-Seiten ohne Hint-Bar

Auf QRCODE und WIFI_SETUP wird die Hint-Bar ausgeblendet, um Platz für
den QR-Code zu schaffen. Das ist akzeptabel, da der QR-Code die primäre
Interaktion ist und der Nutzer auf diesen Seiten typischerweise nicht
navigiert, sondern scannt.

---

## 10. Umsetzungsplan (Code-Änderungen)

### PR 1: Hint-Labels + Footer (niedriges Risiko)

**Dateien:** `NorviOledDisplay.cpp` — `drawButtonHints()`, `drawFooter()`

| Change | Aktuell | Neu |
|--------|---------|-----|
| S1-Label auf Info-Seiten | `nxt` | `prev` |
| S2-Label auf Info-Seiten | `nxt` | `next` |
| S3 auf MAIN | `ok` | `menu` |
| S3 auf NETWORK/SYSTEM/QRCODE/WIFI_SETUP | `ok` | leer (ausblenden) |
| Footer Page-Zahl | `1`–`6` | `1/5`–`5/5` |

### PR 2: Aktionsmenü + Wizard-Verbesserungen (mittleres Risiko)

**Dateien:** `PoolController.cpp`, `NorviOledDisplay.{hpp,cpp}`

1. S3-Callback auf MAIN → setzt `menuActive_`-Flag statt Mode-Cycle
2. Neue State-Machine in `NorviOledDisplay`:

```cpp
enum class MenuItem : uint8_t {
  MODE,
  PUMP,
  EXIT
};
static MenuItem menuSelection_;
static bool menuActive_;
```

1. Menü-Zeichnung als eigener Draw-Zweig in `drawPage()`
2. Wizard-Cancel in `setupSelectPrevious()` (siehe 9.3)
3. Schritt-Indikator `[1/2]` / `[2/2]` im Wizard-Titel

### PR 3: Long-Press-Feedback + Auto-Return-Warnung (mittleres Risiko)

**Dateien:** `NorviButtonHandler.cpp`, `NorviOledDisplay.cpp`

1. `NorviButtonHandler` exponiert Long-Press-Fortschritt:

```cpp
// In NorviButtonHandler.hpp
/** @brief Long-press progress 0.0–1.0, 0 if not pressing. */
static float getLongPressProgress();
```

1. `NorviOledDisplay::loop()` zeichnet Fortschrittsbalken während
   Long-Press auf S3 (siehe 8.2)
2. Auto-Return-Countdown: letzte 5 s zeigen `→ MAIN in 5s` im Footer

### PR 4 (optional): Hint-Bar-Code aufräumen

**Datei:** `NorviOledDisplay.cpp`

Aktuelle `drawButtonHints()` hat 3×3 verschachtelte Cases. Refaktor in
datengetriebene Schleife:

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

## 11. Review-Entscheidungen (Change-Log)

| Punkt | Review-Feedback | Entscheidung | Begründung |
|-------|----------------|-------------|-----------|
| Long-Press Feedback | Fortschrittsbalken fordern | ✅ **Aufgenommen** (siehe 8.2) | Kritisch für Vertrauen in die Aktion |
| Hint-Bar i18n | Labels internationalisieren? | ⏳ **Vorerst EN**, später erweiterbar | Speicher, kein dringender Bedarf |
| Wizard Cancel | Explizite Abbruch-Option | ✅ **S1-Cancel** (siehe 9.3) | Intuitiv, einfache Implementierung |
| Auto-Return Warnung | Countdown vor Rückkehr | ✅ **5-s-Hinweis** (siehe 9.2) | Verhindert Überraschungen |
| Prototyp/Test | Usability-Test empfohlen | ⏳ **Nach PR 2** — vor Merge in main | Risiko niedrig, Änderungen lokal |
| Schritt-Indikator | Fortschritt im Wizard visualisieren | ✅ **`[1/2]`** (siehe 9.4) | Minimaler Platzbedarf |

---

## 12. Offene Fragen / Diskussion

1. **S3 im Aktionsmenü:** Soll die Modus-Änderung sofort生效 oder erst
   beim Verlassen des Menüs?
   - Vote: **Sofort** — das Menü ist nur Auswahl, kein Formular. Nach
     Auswahl → zurück zu MAIN.

2. **S3-Lang auf Info-Seiten:** Aktuell nur im Sensor-Kontext sinnvoll.
   Soll S3-Lang auf MAIN einen Factory Reset / Reboot anbieten?
   - Vote: **Später** — erstmal nur Sensor-Speichern. Long-Press wird
     als Konzept etabliert, aber nicht überall mit Funktion belegt.

3. **S1 auf MAIN:** Soll S1 von MAIN aus direkt zur letzten Seite
   springen (wrap) oder deaktiviert sein?
   - Vote: **Deaktiviert** — kein Hint, keine Aktion. Der Nutzer kann
     nur vorwärts. Der Wrap ist nicht intuitiv.

4. **Auto-Return-Timeouts:** Sollen die unterschiedlich lang sein je
   nach Seite?
   - Vote: **Ja** — Info-Seiten 60 s, Menü 30 s, Wizard 120 s.
     Unterschiedliche Kontexte brauchen unterschiedliche Bedenkzeit.

5. **Prototyp:** Soll vor der main-Integration ein Prototyp aufgespielt
   werden?
   - Vote: **Ja** — PR 2 auf OTA-Kanal `dev` flashen und 1 Woche testen.
     Gibt Rückmeldung ob Menü-Navigation sich natürlich anfühlt.

---

## 13. Visual Summary

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
│  Kern-Änderungen:                                     │
│  - "prev" auf S1 ≠ "next" auf S2                     │
│  - S3 nie 'ok' auf Info-Seiten                       │
│  - Aktion in 2 Schritten (S3→menu→Auswahl→S3)        │
│  - Long-Press mit Fortschrittsbalken                 │
│  - Wizard-Cancel + Schritt-Indikator                 │
│  - Auto-Return mit 5s-Vorwarnung                     │
└─────────────────────────────────────────────────────┘
```

---

> **Nächste Schritte:**
> 1. Finales Review des Konzepts
> 2. **PR 1** — Hint-Labels + Footer (sofort umsetzbar)
> 3. **PR 2** — Aktionsmenü + Wizard-Cancel (mittelfristig)
> 4. **PR 3** — Long-Press-Feedback + Auto-Return-Warnung
> 5. **Optional:** PR 4 — Code-Refaktor Hint-Bar
> 6. Prototyp-Test auf OTA-Kanal `dev` vor Merge in main
