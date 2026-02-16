# Coding Guidelines für ESP8266/ESP32 PlatformIO Projekte

## Übersicht

Dieses Dokument beschreibt die Coding-Richtlinien für das Pool-Controller Projekt, um Super-Linter Fehler zu vermeiden und Best Practices für ESP8266/ESP32 Entwicklung mit PlatformIO zu befolgen.

## 1. C++ Code-Formatierung (clang-format)

### 1.1 Grundlegende Regeln

- **Zeilenlänge**: Maximal 130 Zeichen (definiert in `.clang-format`)
- **Einrückung**: 2 Leerzeichen (keine Tabs)
- **Klammern**: K&R-Stil (öffnende Klammer auf derselben Zeile)
- **Pointer-Ausrichtung**: Links (`int* ptr` statt `int *ptr`)

### 1.2 Wichtige clang-format Anforderungen

#### Leerzeichen zwischen Code und Kommentaren
```cpp
// FALSCH
int x = 5;// Kommentar

// RICHTIG
int x = 5;  // Kommentar (mindestens 2 Leerzeichen)
```

#### Leerzeichen bei Kontrollstrukturen
```cpp
// FALSCH
if(condition){
    doSomething();
}

// RICHTIG
if (condition) {
    doSomething();
}
```

#### Namespaces-Formatierung
```cpp
// FALSCH
namespace PoolController
{
    namespace Detail {
        // code
    }
}

// RICHTIG
namespace PoolController {
    namespace Detail {
        // code
    }
}
```

#### Initialisierung mit geschweiften Klammern
```cpp
// FALSCH
static Context context { };

// RICHTIG
static Context context{};
```

#### Pointer und NULL
```cpp
// FALSCH
TimeChangeRule *tcr = NULL;

// RICHTIG
TimeChangeRule* tcr = nullptr;
```

#### Alignment von Variablen
```cpp
// FALSCH
const char* cTimezone     = "timezone";
const char* cTimezoneName = "Timezone";

// RICHTIG (clang-format richtet automatisch aus)
const char* cTimezone     = "timezone";
const char* cTimezoneName = "Timezone";
// Aber achte darauf, dass AlignConsecutiveDeclarations aktiviert ist
```

### 1.3 Automatische Formatierung

Führe vor jedem Commit aus:
```bash
# Alle C++ Dateien formatieren
clang-format -i src/**/*.cpp src/**/*.hpp

# Oder einzelne Datei
clang-format -i src/PoolController.cpp

# Prüfen ohne Änderungen
clang-format --dry-run --Werror src/**/*.cpp
```

## 2. C++ Stil-Richtlinien (cpplint)

### 2.1 Zeilenlänge
- Maximal 80 Zeichen für cpplint (strenger als clang-format)
- Lange Kommentare über mehrere Zeilen aufteilen

### 2.2 Datentypen
```cpp
// FALSCH
unsigned long timestamp;
long value;

// RICHTIG
uint32_t timestamp;  // Feste Breite, plattformunabhängig
int32_t value;
```

### 2.3 Inklude-Guards
```cpp
// Verwende #pragma once statt Include-Guards
#pragma once

// Oder klassische Guards
#ifndef POOL_CONTROLLER_MODULE_HPP
#define POOL_CONTROLLER_MODULE_HPP
// ...
#endif  // POOL_CONTROLLER_MODULE_HPP
```

## 3. EditorConfig Konformität

### 3.1 Grundeinstellungen (.editorconfig)
- **Einrückung**: 2 Leerzeichen für alle Dateien
- **Keine Tabs**: Immer Leerzeichen verwenden
- **Trailing Whitespace**: Entfernen
- **Final Newline**: Immer einfügen
- **Charset**: UTF-8

### 3.2 Spezifische Datei-Formate

#### YAML-Dateien (.github/workflows/*.yml)
```yaml
# Immer doppelte Anführungszeichen
name: "Workflow Name"

# Lange Zeilen mit | oder > aufteilen
run: >
  command with many
  arguments
```

#### Markdown-Dateien
- "Wi-Fi" (not "WiFi" - note the hyphen)
- URLs in spitze Klammern: `<https://example.com>`

#### INI-Dateien (platformio.ini)
```ini
# 2 Leerzeichen für Einrückung
[env:esp32dev]
lib_deps =
  me-no-dev/Homie@^3.0.0
  https://github.com/me-no-dev/ESPAsyncWebServer.git
```

## 4. ESP8266/ESP32 Spezifische Best Practices

### 4.1 Speicherverwaltung
```cpp
// Vermeide große Stack-Allocations
char buffer[1024];  // Könnte Stack Overflow verursachen

// Besser: Heap-Allocation oder kleinere Puffer
String buffer;
buffer.reserve(1024);
```

### 4.2 String-Behandlung
```cpp
// FALSCH - Fragmentiert Heap
String result = "";
for (int i = 0; i < 100; i++) {
    result += String(i);
}

// RICHTIG - Reserve Speicher im Voraus
String result;
result.reserve(300);  // Schätze benötigten Platz
for (int i = 0; i < 100; i++) {
    result += String(i);
}
```

### 4.3 Async Operations
```cpp
// Nutze yield() in langen Schleifen
for (int i = 0; i < 10000; i++) {
    // Arbeit
    if (i % 100 == 0) {
        yield();  // Gibt ESP Zeit für WiFi/System-Tasks
    }
}
```

### 4.4 Wi-Fi und Netzwerk
```cpp
// Prüfe Verbindungsstatus
if (WiFi.status() == WL_CONNECTED) {
    // Netzwerk-Operation
}

// Verwende WiFi.isConnected() für ESP32
if (WiFi.isConnected()) {
    // Netzwerk-Operation
}
```

## 5. PlatformIO Best Practices

### 5.1 Library-Verwaltung

#### ESPAsyncWebServer
```ini
# FALSCH - Package-Name mit Leerzeichen
lib_deps = me-no-dev/ESP Async WebServer @ 1.2.3

# RICHTIG - GitHub URL verwenden
lib_deps = https://github.com/me-no-dev/ESPAsyncWebServer.git
```

#### Duplikate vermeiden
```ini
# lib_ignore verwenden, um Konflikte zu vermeiden
[env:esp32dev]
lib_ignore = ESP Async WebServer  # Ignoriert Space-Variante
lib_deps = https://github.com/me-no-dev/ESPAsyncWebServer.git
```

#### Plattform-spezifische Dependencies
```ini
# AsyncTCP Libraries sind plattform-spezifisch
# ESP32 verwendet AsyncTCP (ohne "ESP" Präfix)
# ESP8266 verwendet ESPAsyncTCP (mit "ESP" Präfix)
# ESPAsyncWebServer bringt diese automatisch mit!
```

### 5.2 Build-Flags
```ini
# Debug-Informationen
build_flags =
  -DDEBUG_ESP_PORT=Serial
  -DDEBUG_ESP_CORE
  
# Release-Optimierung
build_flags =
  -Os  # Optimiere für Größe
```

### 5.3 Monitor-Einstellungen
```ini
monitor_speed = 115200
monitor_filters = esp32_exception_decoder  # Für ESP32
```

## 6. Git Workflow

### 6.1 Pre-Commit Checks
```bash
# Vor jedem Commit ausführen:

# 1. Formatiere C++ Code
clang-format -i src/**/*.cpp src/**/*.hpp

# 2. Prüfe EditorConfig
# (wird automatisch von Super-Linter gemacht)

# 3. Teste lokale Compilation
pio run -e esp32dev
pio run -e nodemcuv2
```

### 6.2 Commit Messages
```
# Format: <type>: <subject>

fix: Correct clang-format violations in PoolController.cpp
feat: Add NTP server configuration support
docs: Update coding guidelines
refactor: Improve memory usage in Timer class
```

## 7. Super-Linter Konfiguration

### 7.1 Aktivierte Linter
- **EditorConfig**: Datei-Formatierung
- **YAML**: GitHub Actions Workflows
- **Markdown**: Dokumentation
- **CPP**: C++ Code (clang-format)

### 7.2 Deaktivierte Linter
- **CHECKOV**: Zu viele False Positives (manuell validiert)

### 7.3 Lokale Super-Linter Ausführung
```bash
docker run -e RUN_LOCAL=true \
  -e VALIDATE_EDITORCONFIG=true \
  -e VALIDATE_CPP=true \
  -e VALIDATE_MARKDOWN=true \
  -e VALIDATE_YAML=true \
  -v $(pwd):/tmp/lint \
  github/super-linter:v7
```

## 8. Häufige Fehler und Lösungen

### 8.1 "Wrong indent style found (tabs instead of spaces)"
```bash
# Tabs durch Leerzeichen ersetzen
find src -name "*.cpp" -o -name "*.hpp" | xargs sed -i 's/\t/  /g'
```

### 8.2 "Trailing whitespace"
```bash
# Entferne trailing whitespace
find . -name "*.cpp" -o -name "*.hpp" | xargs sed -i 's/[[:space:]]*$//'
```

### 8.3 "Line too long"
```cpp
// Lange Zeilen umbrechen
// VORHER
static const char* veryLongVariableName = "This is a very long string that exceeds the line limit";

// NACHHER
static const char* veryLongVariableName =
    "This is a very long string that exceeds the line limit";
```

### 8.4 "clang-format-violations"
```bash
# Automatisch beheben
clang-format -i <file>
```

## 9. IDE-Integration

### 9.1 Visual Studio Code
```json
// .vscode/settings.json
{
  "editor.formatOnSave": true,
  "editor.insertSpaces": true,
  "editor.tabSize": 2,
  "C_Cpp.clang_format_style": "file",
  "C_Cpp.clang_format_fallbackStyle": "LLVM",
  "files.insertFinalNewline": true,
  "files.trimTrailingWhitespace": true
}
```

### 9.2 Extensions
- C/C++ (Microsoft)
- PlatformIO IDE
- EditorConfig for Visual Studio Code
- Prettier (für YAML/Markdown)

## 10. Checkliste vor PR

- [ ] Alle C++ Dateien mit clang-format formatiert
- [ ] Keine Tabs, nur Leerzeichen (2 Spaces)
- [ ] Kein trailing whitespace
- [ ] Zeilenlänge beachtet (80 für cpplint, 130 für clang-format)
- [ ] Fixed-width Typen verwendet (uint32_t statt unsigned long)
- [ ] Alle Tests laufen erfolgreich
- [ ] PlatformIO builds erfolgreich (beide Plattformen)
- [ ] Super-Linter CI läuft erfolgreich durch

## Referenzen

- [clang-format Dokumentation](https://clang.llvm.org/docs/ClangFormat.html)
- [EditorConfig](https://editorconfig.org/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [ESP32 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [PlatformIO Documentation](https://docs.platformio.org/)
