---
name: cpp-code-quality
description: "Code quality, linting, formatting, and CI fix patterns for the pool-controller project. Use when asked to fix linter errors, run clang-format, fix EditorConfig issues, or resolve Super-Linter CI failures. 🇩🇪 Deutsche Trigger: Code-Qualität, Linting, Formatierung, clang-format, cpplint, Super-Linter, EditorConfig, CI-Fehler beheben, Pre-Commit."
keywords:
  - code qualität
  - code quality
  - linting
  - formatierung
  - clang-format
  - cpplint
  - super-linter
  - editorconfig
  - ci fehler
  - ci fixes
  - pre-commit
  - make lint
  - make lint-fix
  - quality gates
  - platformio check
  - static analysis
---

# Code Quality — Pool Controller

Linting and formatting standards for the pool-controller project. CI uses Super-Linter v8.3.1 with specific linters enabled.

> **🔍 Code Search**: Use `semble search "lint error"` or `semble search "clang-format violation"` to
> locate formatting issues. `semble find-related` helps trace patterns across the codebase. See
> `Agents.md` §7 for full `semble` usage.

---

## Pre-PR Quality Gate Checklist

**Jeder PR muss alle folgenden Gates grün durchlaufen, bevor er gemerged wird.**

### Gate 1 — C++ Formatting (clang-format)

```bash
# Native (Linux x86_64):
clang-format -i src/**/*.cpp src/**/*.hpp

# ARM64 Fallback (via Docker):
docker run --rm --platform linux/arm64 \
  -v $(pwd):/work -w /work ubuntu:22.04 \
  bash -c "apt-get update -qq && apt-get install -y -qq clang-format-14 && \
  find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format-14 -i -style=file"
```

### Gate 2 — C++ Linting (cpplint)

```bash
# Setup (einmalig):
python3 -m venv /tmp/lint-venv && /tmp/lint-venv/bin/pip install cpplint

# Run (mit Projekt-Filtern):
/tmp/lint-venv/bin/cpplint \
  --linelength=130 \
  --filter=-legal/copyright,-build/include_subdir,-runtime/int,-whitespace/indent,-readability/casting,-build/header_guard,-runtime/threadsafety,-whitespace/line_length,-runtime/string,-runtime/printf,-runtime/references,-readability/function \
  src/*.cpp src/*.hpp src/Nodes/*.cpp src/Nodes/*.hpp

# Erwartet: 0 errors
```

### Gate 3 — EditorConfig Compliance

**Wichtig:** CI prüft per `editorconfig-checker` **alle** Dateien im Repository (`.editorconfig` Rule `[*]`).
Daher müssen lokale Checks ebenfalls alle Dateitypen abdecken — nicht nur C++ und Web-UI.

#### Variante A (empfohlen): editorconfig-checker

```bash
# Einmalig installieren:
sudo npm install -g editorconfig-checker

# Prüft alle Dateien gegen .editorconfig:
editorconfig-checker -exclude '.git|.pio|.vscode|.platformio|build|lib|node_modules'
```

#### Variante B: Manuelle Checks (alle git-getrackten Textdateien)

```bash
# Alle git-getrackten Dateien als Grundlage
FILES=$(git ls-files '*.cpp' '*.hpp' '*.h' '*.html' '*.js' '*.css' \
  '*.ini' '*.yml' '*.yaml' '*.json' '*.md' '*.cfg' '*.txt' '*.sh' 'Makefile' \
  '.editorconfig' '.gitignore' '.prettierrc' 'CPPLINT.cfg')

# Keine Tabs:
echo "$FILES" | tr ' ' '\n' | xargs grep -l $'\t' 2>/dev/null \
  | grep . && echo "✗ TABS GEFUNDEN!" || echo "✓ Keine Tabs"

# Kein Trailing Whitespace:
echo "$FILES" | tr ' ' '\n' | xargs grep -l '[[:space:]]$' 2>/dev/null \
  | grep . && echo "✗ TRAILING WHITESPACE!" || echo "✓ Kein Trailing Whitespace"

# Final Newline:
echo "$FILES" | tr ' ' '\n' | while IFS= read -r f; do
  [ -f "$f" ] || continue
  last=$(tail -c 1 "$f")
  [ -z "$last" ] || echo "✗ KEINE FINAL NEWLINE: $f"
done | grep "✗" || echo "✓ Alle Dateien haben Final Newline"
```

### Gate 4 — Static Analysis (platformio check)

```bash
# Wie CI (platformio check mit skip-packages):
pio check --environment esp32dev --skip-packages

# Erwartet: keine Fehler (Warnungen sind akzeptabel)
```

### Gate 5 — Build

```bash
# Build für alle Targets:
pio run --environment esp32dev

# Erwartet: Erfolgreicher Build (exit code 0)
```

### Gate 6 — Nicht-lokal prüfbar (CI)

Diese Gates laufen nur in GitHub Actions und müssen auf dem PR grün sein:

- **Super-Linter**: cpplint, Markdown, YAML, JSON, GitHub Actions, EditorConfig, Gitleaks, Bash
- **CodeQL**: Security Analysis
- **pio-dependency-check**: Veraltete Pakete

---

## Quick Reference (Zusammenfassung)

```bash
# === Quality Gates (vor jedem PR ausführen) ===

# 1. Formatting
docker run --rm --platform linux/arm64 -v $(pwd):/work -w /work ubuntu:22.04 \
  bash -c "apt-get update -qq && apt-get install -y -qq clang-format-14 2>&1 | tail -1 && \
  find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format-14 -i -style=file"

# 2. Cpplint
/tmp/lint-venv/bin/cpplint --linelength=130 \
  --filter=-legal/copyright,-build/include_subdir,-runtime/int,-whitespace/indent,-readability/casting,-build/header_guard,-runtime/threadsafety,-whitespace/line_length,-runtime/string,-runtime/printf,-runtime/references,-readability/function \
  src/*.cpp src/*.hpp src/Nodes/*.cpp src/Nodes/*.hpp

# 3. EditorConfig
editorconfig-checker -exclude '.git|.pio|.vscode|.platformio|build|lib|node_modules'
# Fallback: git-getrackte Textdateien manuell prüfen
FILES=$(git ls-files '*.cpp' '*.hpp' '*.h' '*.html' '*.js' '*.css' \
  '*.ini' '*.yml' '*.yaml' '*.json' '*.md' '*.cfg' '*.txt' '*.sh' 'Makefile')
echo "$FILES" | tr ' ' '\n' | xargs grep -l '[[:space:]]$' 2>/dev/null \
  | grep . && echo "TRAILING WHITESPACE!"

# 4. Static Analysis
pio check --environment esp32dev --skip-packages

# 5. Build
pio run --environment esp32dev

# === Auto-fix bei Fehlern ===
# clang-format (siehe Gate 1)
# EditorConfig:
find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec sed -i 's/[[:space:]]*$//' {} \;
```

---

## CI Linter Configuration

From `.github/workflows/linter.yml` — diese Linters sind **enabled**:

- `VALIDATE_CPP=true` — cpplint für C++ Style
- `VALIDATE_MARKDOWN=true` — markdown-lint
- `VALIDATE_YAML=true` — yaml-lint
- `VALIDATE_JSON=true` — JSON validation
- `VALIDATE_GITHUB_ACTIONS=true` — workflow validation
- `VALIDATE_EDITORCONFIG=true` — EditorConfig compliance
- `VALIDATE_GITLEAKS=true` — secret detection
- `VALIDATE_BASH=true` — shell check

**Note**: `VALIDATE_CLANG_FORMAT` ist explizit disabled (Version incompatibility). clang-format wird lokal ausgeführt (s.o.).

## PlatformIO Static Analysis (CI)

CI führt `platformio check --environment esp32dev --skip-packages` aus. Lokal identisch.

Das entspricht den **enabled checks** aus `platformio.ini`. Typische Checks:

- `def-unsafe` (unsichere Definitionen)
- `nullptr-dereference` (Nullpointer)
- `uninitialized` (uninitialisierte Variablen)
- `unused` (unused variables/functions)
- `virtual` (virtuelle Destruktoren)

### CPPLINT.cfg Configuration

The project has a custom `CPPLINT.cfg` at the repository root that sets:

- `linelength=130` (matching `.clang-format`, overriding cpplint's default 80)
- Disabled filters for embedded/Arduino patterns: `-legal/copyright`, `-build/include_subdir`,
  `-runtime/int`, `-whitespace/indent`, `-readability/casting`, and more

Check `CPPLINT.cfg` before adding/removing filters — it reflects deliberate project decisions to
accommodate Arduino/ESP32 idioms while enforcing Google C++ Style where it matters.

---

## Common CI Failures & Fixes

### 1. Clang-Format Violations

`.clang-format` enforces (from `CODING_GUIDELINES.md`):

- Max line length: **130 characters**
- Indentation: **2 spaces** (no tabs)
- Brace style: **K&R** (opening brace on same line)
- Pointer alignment: **left** (`int* ptr`)

**Auto-fix**:

```bash
# Native:
clang-format -i src/**/*.cpp src/**/*.hpp

# ARM64/Docker:
docker run --rm --platform linux/arm64 -v $(pwd):/work -w /work ubuntu:22.04 \
  bash -c "apt-get update -qq && apt-get install -y -qq clang-format-14 && \
  find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format-14 -i -style=file"
```

**Common pattern fixes**:

| Issue                  | Wrong              | Right               |
| ---------------------- | ------------------ | ------------------- |
| Namespace brace        | `namespace Foo\n{` | `namespace Foo {`   |
| Control flow space     | `if(x){`           | `if (x) {`          |
| Pointer style          | `int *ptr`         | `int* ptr`          |
| NULL → nullptr         | `ptr = NULL`       | `ptr = nullptr`     |
| Empty braces           | `Context{ }`       | `Context{}`         |
| Inline comment spacing | `code;//comment`   | `code;  // comment` |

### 2. EditorConfig Issues

`.editorconfig` requires:

- **2 space indentation** everywhere (no tabs)
- **No trailing whitespace**
- **Final newline** at end of every file

**Auto-fix** (alle git-getrackten Textdateien):

```bash
# Alle relevanten Dateien finden
FILES=$(git ls-files '*.cpp' '*.hpp' '*.h' '*.html' '*.js' '*.css' \
  '*.ini' '*.yml' '*.yaml' '*.json' '*.md' '*.cfg' '*.txt' '*.sh' 'Makefile' \
  '.editorconfig' '.gitignore' 'CPPLINT.cfg')

# Remove tabs → spaces (Vorsicht: Makefile erlaubt Tabs!)
echo "$FILES" | tr ' ' '\n' | grep -v '^Makefile$' | \
  xargs sed -i 's/\t/  /g' 2>/dev/null || true

# Remove trailing whitespace
echo "$FILES" | tr ' ' '\n' | xargs sed -i 's/[[:space:]]*$//' 2>/dev/null || true
```

### 3. Cpplint Include Order

Cpplint erwartet: eigener Header, C System Headers, C++ System Headers, andere Projekt-Header.

```cpp
// WRONG (projekt-header vor system-headern):
#include "NetworkManager.hpp"
#include "ConfigManager.hpp"
#include <esp_wifi.h>

// CORRECT (system-header vor projekt-header):
#include "NetworkManager.hpp"

#include <esp_wifi.h>

#include "ConfigManager.hpp"
```

**Auto-fix**: Includes manuell umordnen nach Google Style Guide Konvention.

### 4. Cpplint Line Length

Cpplint (via Super-Linter im CI) verwendet das `CPPLINT.cfg` mit `linelength=130`. Wenn der CI
cpplint-Trotzdem 80 Zeichen meldet, liegt es an einer veralteten CI-Cache oder Konfiguration.

### 5. Include Guard Style

The project uses `#pragma once` — ensure every header has it at the top.

### 6. Type Usage

```cpp
// WRONG (platform-dependent):
unsigned long timestamp;
long value;

// CORRECT (fixed width):
uint32_t timestamp;
int32_t value;
```

Checked files: `src/*.cpp`, `src/*.hpp`, `src/Nodes/*.cpp`, `src/Nodes/*.hpp`, `lib/Vector/*`.

---

## YAML Formatting (GitHub Actions)

From `.github/linters/.yaml-lint.yml`:

- Double quotes for strings: `name: "Build"` not `name: 'Build'`
- Long lines use `>` or `|` wrapping
- No trailing whitespace

## Markdown Formatting

From `.github/linters/.markdown-lint.yml`:

- "Wi-Fi" (not "WiFi" in prose — but code identifiers keep their original spelling)
- URLs in angle brackets: `<https://example.com>`
- No bare URLs outside code blocks

---

## PlatformIO Check (Static Analysis)

CI führt diesen Schritt aus. Lokale Ausführung:

```bash
pio check --environment esp32dev --skip-packages
```

**Typische Fehler und Fixes**:

- `[violation] variable 'x' is uninitialized` → Initialisierung hinzufügen
- `[violation] unused variable 'y'` → Variable entfernen oder `(void)y;` markieren
- `[violation] virtual destructor` → `virtual ~ClassName() = default;` in Basis-Klassen ergänzen

---

## Pre-Commit Hook

```bash
ln -s ../../scripts/pre-commit.sh .git/hooks/pre-commit
```

The hook runs clang-format on staged `.cpp`/`.hpp` files before each commit.

> **Tipp:** EditorConfig-Verstöße (Tabs, Trailing Whitespace) erkennt man am besten mit
> `editorconfig-checker -exclude '.git|.pio|.vscode|.platformio|build|lib|node_modules'`
> vor dem Commit — das prüft **alle** Dateitypen, inkl. `platformio.ini`.

---

## Full CI Simulation (ARM64)

Super-Linter Docker image (`ghcr.io/super-linter/super-linter:v8`) hat **kein ARM64 Image**. Lokal auf ARM64 daher
nicht via Docker ausführbar. Stattdessen die einzelnen Linter direkt installieren und ausführen:

### Installation der Einzel-Linter

```bash
# Python Linter (in venv):
python3 -m venv /tmp/lint-venv
/tmp/lint-venv/bin/pip install cpplint yamllint

# Node.js Linter (systemweit):
sudo npm install -g markdownlint-cli editorconfig-checker

# Eventuell weitere:
sudo apt install -y shellcheck  # VALIDATE_BASH
```

### Einzel-Linter ausführen (Alternative zu Super-Linter)

```bash
# MARKDOWN (120 Zeichen wie CI):
markdownlint --config /dev/null --rules '~MD013=120' \
  docs/**/*.md   # oder gezielt geänderte Dateien

# YAML:
yamllint .github/linters/ data/web/ templates/

# EditorConfig (via editorconfig-checker):
editorconfig-checker -exclude '.git' .

# Gitleaks (secret detection):
gitleaks detect --source . -v --no-git

# Bash:
shellcheck scripts/*.sh .github/scripts/*.sh
```

### Kurzcheck für PRs (nur geänderte Dateien)

```bash
# Geänderte Markdown-Dateien checken:
markdownlint $(git diff --name-only --diff-filter=AM HEAD~1 | grep '\.md$')
```
