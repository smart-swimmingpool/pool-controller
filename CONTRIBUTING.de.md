# Beitragsrichtlinien für Pool Controller

Vielen Dank für Ihr Interesse, zum **Smart Swimming Pool Controller** Projekt
beizutragen! 🏊‍♂️

Dieses Dokument enthält Richtlinien für Beiträge zum Projekt. Bitte lesen Sie es
sorgfältig, bevor Sie Ihren ersten Pull Request einreichen.

## 📋 Inhaltsverzeichnis

- [Verhaltenskodex](#-verhaltenskodex)
- [Wie man beiträgt](#-wie-man-beiträgt)
- [Erste Schritte](#-erste-schritte)
- [Entwicklungsworkflow](#-entwicklungsworkflow)
- [Codierstandards](#-codierstandards)
- [Sicherheitsrichtlinien](#-sicherheitsrichtlinien)
- [Testen](#-testen)
- [Pull Request Prozess](#-pull-request-prozess)
- [Review-Prozess](#-review-prozess)
- [Commit-Nachrichten-Richtlinien](#-commit-nachrichten-richtlinien)
- [Qualitätsgates](#-qualitätsgates)
- [Ressourcen](#-ressourcen)

---

## 🤝 Verhaltenskodex

Dieses Projekt hält sich an den
[Contributor Covenant](https://www.contributor-covenant.org/de/version/1/4/code-of-conduct.html).
Durch die Teilnahme verpflichten Sie sich, diesen Kodex einzuhalten. Bitte melden
Sie inakzeptables Verhalten den
[Projektverantwortlichen](https://github.com/smart-swimmingpool/pool-controller/graphs/contributors).

Siehe auch: [code-of-conduct.md](code-of-conduct.md)

---

## 🌟 Wie man beiträgt

### Fehler melden

- **Vorhandene Issues prüfen**: Suchen Sie in
  [GitHub Issues](https://github.com/smart-swimmingpool/pool-controller/issues),
  bevor Sie ein neues erstellen
- **Issue-Vorlage verwenden**: Geben Sie detaillierte Informationen über den Fehler an
- **Enthalten**:
  - Firmware-Version (aus Web-Dashboard oder serieller Monitor)
  - Hardware-Aufbau (ESP32-Modell, Relaismodul, Sensoren)
  - Schritte zur Reproduktion
  - Ausgabe des seriellen Monitors (falls zutreffend)
  - Screenshots (falls UI-bezogen)

### Verbesserungsvorschläge

- **Roadmap prüfen**: Siehe [README.md](README.md) für geplante Funktionen
- **Zuerst diskutieren**: Öffnen Sie eine
  [GitHub Diskussion](https://github.com/smart-swimmingpool/pool-controller/discussions),
  um Ihre Idee zu besprechen
- **Auf Duplikate prüfen**: Durchsuchen Sie bestehende Issues und PRs

### Pull Requests einreichen

- **Repository forken**: Erstellen Sie Ihren eigenen Fork
- **Feature-Branch erstellen**: Verwenden Sie beschreibende Branch-Namen
  (z.B. `feat/timer-funktionalität-hinzufügen`)
- **Codierstandards befolgen**: Siehe
  [Coding Guidelines](.github/CODING_GUIDELINES.md)
- **Änderungen testen**: Stellen Sie sicher, dass alle Tests bestehen
- **Dokumentation aktualisieren**: Halten Sie die Docs mit den Code-Änderungen synchron

---

## 🚀 Erste Schritte

### Voraussetzungen

- [PlatformIO](https://platformio.org/) installiert
- [Git](https://git-scm.com/) installiert
- Grundkenntnisse in C++ und ESP32-Entwicklung
- Verständnis des MQTT-Protokolls (für Home Assistant-Integration)

### Entwicklungsumgebung einrichten

```bash
# Repository klonen
git clone https://github.com/smart-swimmingpool/pool-controller.git
cd pool-controller

# Abhängigkeiten installieren (wird von PlatformIO verwaltet)
pio run -e esp32dev

# Lokales Linting ausführen
make lint-fix && make lint

# Projekt bauen
make build
```

### Projektstruktur

```
pool-controller/
├── src/                  # Quellcode
│   ├── PoolController/    # Haupt-Controller-Klassen
│   ├── Nodes/             # Sensor- und Relais-Knoten
│   └── main.cpp           # Einstiegspunkt
├── data/                 # Web-Assets und Konfiguration
│   └── web/               # Web-Interface-Dateien
├── docs/                 # Dokumentation
├── .github/              # GitHub-Konfiguration
│   ├── CODING_GUIDELINES.md  # Codierstandards
│   └── workflows/         # CI/CD-Workflows
├── platformio.ini        # PlatformIO-Konfiguration
└── README.md             # Projektübersicht
```

---

## 🔄 Entwicklungsworkflow

### 1. Feature-Branch erstellen

```bash
# Vom main-Branch
git checkout main
git pull origin main

# Neuen Feature-Branch erstellen
git checkout -b feat/ihre-funktions-bezeichnung
```

**Branch-Namenskonventionen**:

- `feat/` - Neue Funktionen
- `fix/` - Bugfixes
- `docs/` - Dokumentationsupdates
- `refactor/` - Code-Refactoring
- `chore/` - Wartungsaufgaben
- `security/` - Sicherheitsbezogene Änderungen

### 2. Änderungen vornehmen

- [Codierrichtlinien](.github/CODING_GUIDELINES.md) befolgen
- Änderungen fokussiert und minimal halten
- Relevante Dokumentation aktualisieren
- Tests hinzufügen, falls zutreffend

### 3. Qualitätsprüfungen ausführen

```bash
# Linting-Probleme automatisch beheben
make lint-fix

# Überprüfen, dass Linting besteht
make lint

# Für ESP32 bauen
make build

# Aufräumen
make clean
```

### 4. Änderungen commiten

Befolgen Sie die [Conventional Commits](#-commit-nachrichten-richtlinien)-Richtlinien.

### 5. Zu Ihrem Fork pushen

```bash
git push origin feat/ihre-funktions-bezeichnung
```

### 6. Pull Request erstellen

- Gehen Sie zu
  [GitHub Pull Requests](https://github.com/smart-swimmingpool/pool-controller/pulls)
- Klicken Sie auf "New Pull Request"
- Wählen Sie Ihren Branch aus
- Füllen Sie die PR-Vorlage aus
- Verlinken Sie alle relevanten Issues

---

## 📖 Codierstandards

### Allgemeine Prinzipien

- **Vorhandene Muster befolgen**: Passen Sie den Stil und die Architektur des
  vorhandenen Codes an
- **Einfach halten**: Bevorzugen Sie einfachen, lesbaren Code gegenüber cleveren
  Optimierungen
- **Code dokumentieren**: Verwenden Sie sinnvolle Kommentare und Docstrings
- **Fehler elegant behandeln**: Ignorieren Sie keine Fehlerbedingungen

### C++-spezifisch

Siehe [CODING_GUIDELINES.md](.github/CODING_GUIDELINES.md) für detaillierte
C++-Standards.

**Wichtige Regeln**:

- Maximale Zeilenlänge: **130 Zeichen** (clang-format)
- Einrückung: **2 Leerzeichen** (keine Tabs)
- Klammern-Stil: **K&R** (öffnende Klammer auf derselben Zeile)
- Zeigerausrichtung: **Links** (`int* ptr`)
- Verwenden Sie `nullptr` statt `NULL`
- Verwenden Sie festen Typen (`uint32_t`, `int16_t`, etc.)

### ESP32-spezifisch

- **Heap-Fragmentierung vermeiden**: Minimieren Sie die Verwendung von `String`
  in Hot Paths
- **Verwenden Sie `constexpr`**: Für Compile-Time-Konstanten
- **Watchdog beachten**: Füttern Sie den Watchdog bei langen Operationen
- **Speicherbeschränkungen**: ESP32 hat ~320KB RAM - seien Sie sich der Zuweisungen
  bewusst

### Sicherheitsüberlegungen

- **Niemals Anmeldedaten hardcoden**: Verwenden Sie Konfigurationsdateien oder
  sicheren Speicher
- **Alle Eingaben validieren**: Besonders von Netzwerkquellen
- **Sichere Protokolle verwenden**: TLS für MQTT, HTTPS für Web
- **Ausgaben bereinigen**: Verhindern Sie Injection-Angriffe

---

## 🔒 Sicherheitsrichtlinien

### Melden von Sicherheitslücken

**MELDEN SIE KEINE** Sicherheitslücken über öffentliche GitHub Issues. Stattdessen:

1. **Private Offenlegung**: Kontaktieren Sie die Verantwortlichen direkt per E-Mail
2. **Verantwortungsvolle Offenlegung**: Gewähren Sie angemessene Zeit für Korrekturen
3. **Koordinieren**: Arbeiten Sie mit den Verantwortlichen an der Korrektur

### Sicherheits-Best Practices

- **Anmeldedaten**: Commiten Sie niemals Passwörter, API-Schlüssel oder Geheimnisse
- **Gitleaks**: Führen Sie `gitleaks detect --source .` vor dem Commit aus
- **CodeQL**: Sicherheitsanalyse läuft automatisch in CI
- **Abhängigkeiten**: Halten Sie Abhängigkeiten aktuell (Dependabot aktiviert)

### Sicherheits-Checkliste für PRs

- [ ] Keine hardcodierten Anmeldedaten oder Geheimnisse
- [ ] Eingabevalidierung für alle Benutzereingaben
- [ ] Sichere Kommunikationsprotokolle verwendet
- [ ] Fehlermeldungen enthüllen keine sensiblen Informationen
- [ ] Logging enthält keine sensiblen Daten
- [ ] Speicherverwaltung verhindert Lecks

Siehe auch: [Sicherheitsreferenzen](docs/security-references.de.md)

---

## 🧪 Testen

### Lokales Testen

```bash
# Bauen und Tests ausführen
make build

# PlatformIO-Tests ausführen
pio test
```

### Testabdeckung

- **Unittests**: Für Kernfunktionalität (Native Build)
- **Integrationstests**: Für Komponenteninteraktionen
- **Manuelles Testen**: Für hardwarespezifische Funktionen

### Testanforderungen

- Alle bestehenden Tests müssen bestehen
- Neue Funktionen sollten Tests enthalten
- Bugfixes sollten Regressionstests enthalten

---

## 📤 Pull Request Prozess

### Vor dem Einreichen

1. **Selbstprüfung**: Überprüfen Sie Ihren eigenen Code
2. **Linting ausführen**: `make lint-fix && make lint`
3. **Erfolgreich bauen**: `make build`
4. **Änderungen testen**: Stellen Sie sicher, dass alle Tests bestehen
5. **Dokumentation aktualisieren**: Halten Sie die Dokumentation synchron

### PR-Vorlage

Verwenden Sie die GitHub-PR-Vorlage und fügen Sie ein:

- **Klare Überschrift**: Beschreibend und präzise
- **Detaillierte Beschreibung**: Was, warum und wie
- **Verwandte Issues**: Verlinken Sie alle relevanten Issues
- **Breaking Changes**: Notieren Sie alle Breaking Changes
- **Testen**: Wie Sie die Änderungen getestet haben

### PR-Anforderungen

- **Grüne CI**: Alle GitHub Actions müssen bestehen
- **Keine Merge-Konflikte**: Rebasen Sie ggf.
- **Korrekte Commits**: Befolgen Sie die Commit-Nachrichten-Richtlinien
- **Code-Review**: Mindestens eine Genehmigung erforderlich

---

## 👀 Review-Prozess

### Was Reviewer suchen

1. **Code-Qualität**

   - Befolgt Codierstandards
   - Sauberer, lesbarer Code
   - Ordentliche Fehlerbehandlung
   - Gute Dokumentation

2. **Funktionalität**

   - Funktioniert es wie beabsichtigt?
   - Gibt es Randfälle zu beachten?
   - Ist die Implementierung effizient?

3. **Sicherheit**

   - Keine Sicherheitslücken
   - Ordentliche Eingabevalidierung
   - Sichere Standardwerte

4. **Testen**

   - Gibt es Tests?
   - Bestehen bestehende Tests noch?
   - Ist der Code testbar?

5. **Dokumentation**
   - Ist die Dokumentation aktualisiert?
   - Gibt es klare Commit-Nachrichten?
   - Ist die PR-Beschreibung informativ?

### Review-Zeitplan

- **Kleine Änderungen**: Typischerweise innerhalb von 1-2 Tagen überprüft
- **Große Änderungen**: Kann länger dauern, besonders bei komplexen Funktionen
- **Sicherheitsänderungen**: Erfordern gründliche Überprüfung

### Feedback bearbeiten

- **Reaktiv sein**: Antworten Sie prompt auf Review-Kommentare
- **Änderungen vornehmen**: Aktualisieren Sie Ihren PR basierend auf Feedback
- **Updates pushen**: Ändern Sie Commits oder fügen Sie neue hinzu
- **Reviewer benachrichtigen**: Markieren Sie Reviewer, wenn Sie für eine erneute
  Überprüfung bereit sind

---

## 📝 Commit-Nachrichten-Richtlinien

Wir folgen dem [Conventional Commits](https://www.conventionalcommits.org/)
Format:

```
Typ(Bereich): Betreff

Körper

Fußzeile
```

### Typen

- `feat`: Neue Funktion
- `fix`: Bugfix
- `docs`: Nur Dokumentationsänderungen
- `style`: Änderungen, die die Bedeutung des Codes nicht beeinflussen
  (Whitespace, Formatierung, fehlende Semikolons, etc.)
- `refactor`: Code-Änderung, die weder einen Bug behebt noch eine Funktion hinzufügt
- `perf`: Code-Änderung, die die Performance verbessert
- `test`: Hinzufügen fehlender Tests
- `chore`: Änderungen am Build-Prozess oder Hilfsprogrammen
- `revert`: Kehrt einen vorherigen Commit um
- `security`: Sicherheitsbezogene Änderungen

### Beispiele

```bash
# Gute Commit-Nachrichten
feat(mqtt): Home Assistant Discovery-Unterstützung hinzufügen
fix(web): Session-Timeout-Behandlung korrigieren
docs: Hardware-Anleitung mit neuen Pin-Zuweisungen aktualisieren
refactor(utils): String-Formatierungsfunktionen extrahieren
security: CSRF-Schutz zum Web-Portal hinzufügen
chore: PlatformIO-Abhängigkeiten aktualisieren
```

### Betreffzeile

- Verwenden Sie **Imperativ** ("Füge Funktion hinzu" nicht "Fügte Funktion hinzu")
- **Ersten Buchstaben großschreiben**
- **Kein Punkt** am Ende
- **Unter 50 Zeichen halten**

### Körper

- Erklären Sie **was** geändert wurde
- Erklären Sie **warum** die Änderung vorgenommen wurde
- **Verweisen Sie auf Issues oder PRs**, falls zutreffend

---

## ✅ Qualitätsgates

Alle Pull Requests müssen die folgenden Qualitätsgates bestehen:

### 1. Super-Linter CI ✅

- **EditorConfig**: Dateiformatierungskonformität
- **CPP Lint**: C++-Stilprüfung (cpplint)
- **Markdown**: Dokumentationsformatierung
- **YAML**: Workflow-Dateivalidierung
- **Gitleaks**: Geheimniserkennung

Lokal ausführen:

```bash
make lint
```

### 2. PlatformIO CI ✅

- **Build**: Erfolgreiche Kompilierung für beide Umgebungen
  - `esp32dev` (Standard ESP32)
  - `norvi_ae01_r` (NORVI Industrie-Controller)

Lokal ausführen:

```bash
make build
```

### 3. CodeQL-Analyse ✅

- **Sicherheit**: Keine kritischen Sicherheitslücken
- **Code-Qualität**: Keine größeren Code-Qualitätsprobleme

### 4. Manuelles Review ✅

- **Code-Review**: Mindestens eine Genehmigung
- **Funktionalität**: Änderungen funktionieren wie beabsichtigt
- **Dokumentation**: Alle Dokumente aktualisiert

---

## 📚 Ressourcen

### Dokumentation

- **[README.md](README.md)** - Projektübersicht und Schnellstart
- **[Quick Start Guide](docs/quick-start.md)** - Schritt-für-Schritt-Anleitung
- **[Hardware Guide](docs/hardware-guide.md)** - Montage und Verdrahtung
- **[MQTT-Konfiguration](docs/mqtt-configuration.md)** - Home Assistant-Einrichtung
- **[Codierrichtlinien](.github/CODING_GUIDELINES.md)** - Detaillierte Codierstandards
- **[Sicherheitsreferenzen](docs/security-references.de.md)** - Sicherheits-Best Practices

### Entwicklung

- [PlatformIO-Dokumentation](https://docs.platformio.org/)
- [ESP32-Dokumentation](https://docs.espressif.com/projects/esp-idf/)
- [Arduino-Dokumentation](https://www.arduino.cc/en/Reference/HomePage)

### Community

- [GitHub Diskussionen](https://github.com/smart-swimmingpool/pool-controller/discussions)
  - Fragen stellen, Ideen diskutieren
- [Home Assistant Community](https://community.home-assistant.io/)
  - Smart Home-Integration
- [DIY My Smart Home (Medium)](https://medium.com/diy-my-smart-home)
  - Projektblog

### Verwandte Projekte

- [Smart Swimming Pool Organisation](https://github.com/smart-swimmingpool)
  - Andere verwandte Projekte
- [Home Assistant](https://www.home-assistant.io/)
  - Smart Home-Plattform

---

## 🙏 Anerkennung

Ihre Beiträge sind wertvoll und werden geschätzt! Alle Mitwirkenden sind in der
[Contributors-Grafik](https://github.com/smart-swimmingpool/pool-controller/graphs/contributors)
aufgelistet.

Für bedeutende Beiträge können Sie eingeladen werden, ein Maintainer zu werden.

---

## 📜 Lizenz

Durch das Beitragen zu diesem Projekt stimmen Sie zu, dass Ihre Beiträge unter
der [MIT-Lizenz](LICENSE) lizenziert werden.

---

**📅 Zuletzt aktualisiert**: 2025-01-15

**🤖 Generiert von**: Vibe Code - IoT Sicherheitsanalyse

**📝 Verwandter PR**:
[#112 - IoT Security & Memory Optimization Analysis](https://github.com/smart-swimmingpool/pool-controller/pull/112)
