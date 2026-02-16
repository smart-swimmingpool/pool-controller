# Quick Reference: Super-Linter Fixes

## Schnelle Fehlerbehebung

### 1. Clang-Format Fehler

```bash
# Alle C++ Dateien automatisch formatieren
clang-format -i src/**/*.cpp src/**/*.hpp

# Einzelne Datei prüfen
clang-format --dry-run --Werror src/MyFile.cpp

# Einzelne Datei formatieren
clang-format -i src/MyFile.cpp
```

### 2. Häufigste Clang-Format Fehler

| Fehler              | Falsch             | Richtig                               |
| ------------------- | ------------------ | ------------------------------------- |
| Namespace-Klammer   | `namespace Foo\n{` | `namespace Foo {`                     |
| Leerzeichen bei if  | `if(x){`           | `if (x) {`                            |
| Pointer-Deklaration | `int *ptr`         | `int* ptr`                            |
| NULL verwenden      | `ptr = NULL`       | `ptr = nullptr`                       |
| Leere Klammern      | `Context{ }`       | `Context{}`                           |
| Kommentar-Abstand   | `int x;// comment` | `int x;  // comment` (2+ Leerzeichen) |

### 3. EditorConfig Fehler

```bash
# Tabs durch Leerzeichen ersetzen
find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec sed -i 's/\t/  /g' {} \;

# Trailing Whitespace entfernen
find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec sed -i 's/[[:space:]]*$//' {} \;
```

### 4. YAML (GitHub Actions) Fehler

```yaml
# Immer doppelte Anführungszeichen
name: "Build" # nicht name: 'Build'

# Lange Zeilen mit > oder | umbrechen
run: >
  echo "This is a very long command"
  that spans multiple lines

# Kein trailing whitespace am Zeilenende
```

### 5. Markdown Fehler

```markdown
<!-- WiFi → Wi-Fi -->

This project uses Wi-Fi connectivity.

<!-- Bare URLs in spitze Klammern -->

See <https://example.com> for details.

<!-- Nicht: -->

See https://example.com for details.
```

### 6. PlatformIO.ini Fehler

```ini
# 2 Leerzeichen Einrückung (keine Tabs)
[env:esp32dev]
lib_deps =
  me-no-dev/Homie@^3.0.0

# Keine URLs mit Leerzeichen im Namen
# FALSCH: me-no-dev/ESP Async WebServer
# RICHTIG:
lib_deps = https://github.com/me-no-dev/ESPAsyncWebServer.git
```

## Visual Studio Code Settings

Speichere in `.vscode/settings.json`:

```json
{
  "editor.formatOnSave": true,
  "editor.insertSpaces": true,
  "editor.tabSize": 2,
  "editor.detectIndentation": false,
  "C_Cpp.clang_format_style": "file",
  "files.insertFinalNewline": true,
  "files.trimTrailingWhitespace": true,
  "[cpp]": {
    "editor.defaultFormatter": "ms-vscode.cpptools"
  },
  "[yaml]": {
    "editor.defaultFormatter": "esbenp.prettier-vscode"
  },
  "[markdown]": {
    "editor.defaultFormatter": "esbenp.prettier-vscode"
  }
}
```

## Pre-Commit Hook

Erstelle `.git/hooks/pre-commit`:

```bash
#!/bin/bash
# Auto-format C++ files before commit

# Format all staged C++ files
STAGED_CPP_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp)$')

if [ -n "$STAGED_CPP_FILES" ]; then
    echo "Formatting C++ files..."
    clang-format -i $STAGED_CPP_FILES
    git add $STAGED_CPP_FILES
fi

exit 0
```

Dann:

```bash
chmod +x .git/hooks/pre-commit
```

## Lokale Super-Linter Tests

```bash
# Nur geänderte Dateien testen (schnell)
docker run --rm \
  -e RUN_LOCAL=true \
  -e VALIDATE_CPP=true \
  -e FILTER_REGEX_INCLUDE=".*src/.*" \
  -v $(pwd):/tmp/lint \
  github/super-linter:v7

# Alle Linter (vollständig)
docker run --rm \
  -e RUN_LOCAL=true \
  -e VALIDATE_EDITORCONFIG=true \
  -e VALIDATE_CPP=true \
  -e VALIDATE_MARKDOWN=true \
  -e VALIDATE_YAML=true \
  -e VALIDATE_CHECKOV=false \
  -v $(pwd):/tmp/lint \
  github/super-linter:v7
```

## Checkliste

- [ ] `clang-format -i src/**/*.cpp src/**/*.hpp` ausgeführt
- [ ] Keine Tabs, nur 2 Leerzeichen
- [ ] Kein trailing whitespace
- [ ] `nullptr` statt `NULL`
- [ ] `uint32_t` statt `unsigned long`
- [ ] Namespaces-Klammern auf gleicher Zeile
- [ ] 2+ Leerzeichen vor Inline-Kommentaren
- [ ] Leerzeichen bei `if (`, `while (`, `for (`
- [ ] Local build erfolgreich: `pio run`
- [ ] Git commit mit beschreibender Nachricht

## Weitere Hilfe

Siehe [CODING_GUIDELINES.md](CODING_GUIDELINES.md) für detaillierte Erklärungen.
