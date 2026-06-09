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
# Wie CI (platformio check mit skip-packages, nur medium+):
pio check --environment esp32dev --skip-packages --severity medium

# NORVI-Variante auch prüfen:
pio check --environment norvi_ae01_r --skip-packages --severity medium

# Erwartet: 0 errors (warnings sind OK — aber prüfen!)
```

**Detailierte Erklärung und Tipps → [PlatformIO Check (Static Analysis)](#platformio-check-static-analysis)**

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

# 4. Static Analysis (→ PlatformIO Check für Details)
pio check --environment esp32dev --skip-packages --severity medium
pio check -e norvi_ae01_r --skip-packages --severity medium   # NORVI-Variante

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

## PlatformIO Check in CI

CI führt `platformio check --environment esp32dev --skip-packages` aus.

Der CI-Step verwendet die Standard-Clang-Tidy-Checks (kein `check_flags` in
`platformio.ini`). Lokal kann umfassender geprüft werden.

**Zusammenfassung der aktivierten CI-Checks:**

| Check | Kategorie | Was es findet |
|-------|-----------|--------------|
| `clang-analyzer-core.NullDereference` | Core | Nullpointer-Dereferenzierung |
| `clang-analyzer-core.uninitialized` | Core | Uninitialisierte Variablen |
| `clang-analyzer-deadcode.DeadStores` | Core | Tote Zuweisungen |
| `clang-analyzer-core.CallAndMessage` | Core | Aufruf auf gelöschtem Objekt |
| `bugprone-macro-parentheses` | Bugprone | Makros ohne Klammern |
| `performance-unnecessary-copy` | Performance | Unnötige Kopien (Heap!) |

**Detaillierte Erklärung → [PlatformIO Check (Static Analysis)](#platformio-check-static-analysis)**

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

`pio check` is PlatformIO's static analysis wrapper. It uses **clang-tidy** (default)
or **Clang Static Analyzer** (clangsa) under the hood to find bugs, anti-patterns,
and undefined behavior in C/C++ code.

> ⚠ **CI executes `pio check --environment esp32dev --skip-packages` as part of every PR.**

---

### 1. How It Works

```
pio check
  │
  ├─ clang-tidy (default)
  │   └─ 100+ C++ checks: bugprone, performance, readability, modernize, clang-analyzer, etc.
  │
  └─ clangsa (via --flag clangsa)
      └─ Path-sensitive: dead code, null dereference, memory leaks
```

The tool parses your code using the same build configuration as `pio run`. It
understands preprocessor defines, include paths, and platform-specific headers —
so ESP32-specific code is analyzed correctly.

---

### 2. Configuration in `platformio.ini`

Currently the project has **no explicit `pio check` config**. The defaults are:

```ini
; platformio.ini (all optional — shown with defaults)
;
; check_tool = clangtidy       ; or "clangsa" or "clangtidy,clangsa"
; check_flags.clangtidy =      ; extra -checks=... for clang-tidy
; check_flags.clangsa =        ; extra flags for Clang SA
; check_severity = low         ; "low", "medium", or "high"
; check_patterns =             ; file patterns to include
;   src/*.cpp
;   src/*.hpp
; check_skip_packages = false  ; skip library analysis (set in CI)
```

**Recommended project configuration:**

```ini
[env:esp32dev]
check_tool = clangtidy
check_severity = medium                      ; ignore noise from low-severity warnings
check_flags.clangtidy =
  -checks=-*,clang-analyzer-*,bugprone-*,performance-*,-performance-no-int-to-ptr
check_patterns = src/*.cpp src/*.hpp
check_skip_packages = true                   ; skip 3rd-party libraries by default
```

Use separate `[check]` section if you want the same config across all environments:

```ini
[check]
check_tool = clangtidy
check_skip_packages = true
check_severity = medium
```

---

### 3. CLI Reference

| Command | Effect |
|---------|--------|
| `pio check` | Run on **all** environments |
| `pio check --environment esp32dev` | Run on **one** environment |
| `pio check -e norvi_ae01_r` | Run on NORVI variant (also checks OLED code) |
| `pio check --skip-packages` | Skip library analysis (faster, CI does this) |
| `pio check --severity medium` | Only report medium+ severity (ignore noise) |
| `pio check --verbose` | Show full check output per file |
| `pio check --json` | Machine-readable JSON output |
| `pio check --flags "-*,clang-analyzer-*"` | Pass extra checker list to clang-tidy |
| `pio check --fail-on warning` | Exit non-zero on warnings (not just errors) |
| `pio check --fail-on severity=high` | Exit non-zero on high-severity only |

**Typical local workflow:**

```bash
# Fast — skip packages, show only medium+
pio check -e esp32dev --skip-packages --severity medium

# NORVI-specific
pio check -e norvi_ae01_r --skip-packages --severity medium

# Full deep analysis (takes longer)
pio check -e esp32dev --severity low

# See exactly which checks are enabled
pio check -e esp32dev --verbose | head -50
```

---

### 4. Checker Categories (clang-tidy)

The default PlatformIO check configuration enables a subset from each category:

| Category | Important Checks | Relevance |
|----------|-----------------|-----------|
| `clang-analyzer-*` | `core.NullDereference`, `core.uninitialized`, `deadcode.DeadStores` | **Critical** — real bugs |
| `bugprone-*` | `macro-parentheses`, `signed-char-misuse`, `integer-division` | Medium — embedded patterns |
| `performance-*` | `unnecessary-copy`, `inefficient-vector`, `move-const-arg` | Medium — ESP32 perf matters |
| `readability-*` | `redundant-control-flow`, `non-const-parameter` | Low — style preference |
| `modernize-*` | `use-override`, `deprecated-headers` | Low — code evolution |
| `cppcoreguidelines-*` | Various C++ Core Guidelines | Low — too strict for Arduino |

**Most useful single-check invocation for this project:**

```bash
# Just null-pointer, uninitialized, and dead store
pio check -e esp32dev --flags "-*,
  clang-analyzer-core.NullDereference,
  clang-analyzer-core.uninitialized,
  clang-analyzer-deadcode.DeadStores,
  clang-analyzer-optin.portability.UnixAPI"
```

---

### 5. Output Interpretation

Each violation has this structure:

```
src/PoolController.cpp:123:15: error: variable 'sensorAddr' is uninitialized
    when calling 'DallasTemperature::getAddress'
    [clang-analyzer-core.uninitialized]
```

| Part | Meaning |
|------|---------|
| `src/PoolController.cpp:123:15` | File: **line:column** of the violation |
| `error: / warning:` | **Severity** (error > warning > note) |
| `variable 'x' is uninitialized` | **Message** describing the issue |
| `when calling '...'` | **Context** — path that triggers the issue |
| `[checker-name]` | **Checker ID** — use for suppression or targeted runs |

**Severity mapping:**

| pio check severity | clang-tidy diagnostic | Action required |
|-------------------|----------------------|-----------------|
| **error** | `error` | Must fix |
| **warning** | `warning` | Should fix (or suppress) |
| **note** | `note` | Informational (review) |

---

### 6. Common Violations & Fixes

#### 🟥 A. Uninitialized Variable

```
error: variable 'buf' is uninitialized [clang-analyzer-core.uninitialized]
```

```cpp
// WRONG
char buf[8];
Utils::floatToString(t, buf, sizeof(buf), 1);  // assumes write, but analyzer sees path

// FIX — initialize on declaration (or mark as output param)
char buf[8] = {0};
```

#### 🟥 B. Null Pointer Dereference

```
error: Called CString object method on a dangling pointer [clang-analyzer-core.CallAndMessage]
```

```cpp
// WRONG
String* ptr = nullptr;
if (someCondition) { ptr = new String("hello"); }
ptr->length();  // 💥 null if !someCondition

// FIX — check before use or use stack variable
String s;
if (someCondition) { s = "hello"; }
s.length();
```

#### 🟥 C. Dead Store (useless assignment)

```
warning: Value stored to 'addr' is never read [clang-analyzer-deadcode.DeadStores]
```

```cpp
// WRONG
uint8_t addr[8];
addr[0] = 0x28;  // written but immediately overwritten
getDeviceAddress(addr);  // overwrites addr

// FIX — remove redundant assignment
uint8_t addr[8];
getDeviceAddress(addr);
```

#### 🟩 D. Unused Variable

```
warning: unused variable 'devCount' [clang-analyzer-deadcode.DeadStores]
```

```cpp
// WRONG
uint8_t devCount = getDeviceCount();
// (devCount never used below)

// FIX — remove, or mark explicitly unused
(void)getDeviceCount();
```

#### 🟩 E. Virtual Destructor

```
warning: class 'Singleton' defines a non-virtual destructor [clang-analyzer-optin.cplusplus.VirtualCall]
```

```cpp
// WRONG — base class with virtual methods but non-virtual dtor
class Singleton {
public:
  virtual void init();
  ~Singleton();  // 💥 not virtual!
};

// FIX
class Singleton {
public:
  virtual void init();
  virtual ~Singleton() = default;
};
```

> **Note:** In this project, `NorviOledDisplay` and similar utility classes with
> only static methods don't need virtual destructors. Suppress with `// NOLINT`.

#### 🟩 F. Macro Parentheses

```
warning: macro replacement list should be parenthesized [bugprone-macro-parentheses]
```

```cpp
// WRONG
#define FOO sum + 1

// FIX
#define FOO (sum + 1)
```

#### 🟨 G. Unnecessary Copy (performance)

```
warning: unnecessary copy of 'sensorAddr' of type 'uint8_t[8]' [performance-unnecessary-copy]
```

```cpp
// WRONG
auto addr = getAddress();  // copies 8 bytes

// FIX — use const ref
const auto& addr = getAddress();
```

---

### 7. Suppressing False Positives

Not every `pio check` finding is a real bug. ESP32/Arduino code uses patterns
(ISRs, PROGMEM, hardware registers) that can confuse static analysis.

#### Line-Level Suppression

```cpp
// NOLINTNEXTLINE(clang-analyzer-core.uninitialized)
int val = someHardwareRegister();  // known to be set by DMA
```

```cpp
int val;                          // NOLINT: hardware init via DMA
```

#### Block Suppression (clang-tidy ≥ 14)

```cpp
// NOLINTBEGIN(clang-analyzer-core.uninitialized)
int a;
int b;
initViaDma(&a, &b);
// NOLINTEND(clang-analyzer-core.uninitialized)
```

#### Project-Wide Suppression (`.clang-tidy`)

Create `.clang-tidy` in the project root (not currently used):

```yaml
# .clang-tidy — project-wide suppressions
Checks: '-*,clang-analyzer-*,bugprone-*,performance-*'
CheckOptions:
  - key: performance-unnecessary-copy
    value: "false"  # disable this specific check project-wide
```

> **⚠ Prefer `// NOLINT` over `.clang-tidy`** — suppress only where needed and
> document why, so a real violation elsewhere is still caught.

---

### 8. Integration Guide

#### Pre-Commit Hook

Add this to `scripts/pre-commit.sh` or `.git/hooks/pre-commit`:

```bash
#!/bin/bash
# Run pio check on staged C++ files
STAGED=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp)$')
if [ -n "$STAGED" ]; then
  echo "🔍 Running static analysis on staged files..."
  pio check --environment esp32dev --skip-packages --severity medium
  if [ $? -ne 0 ]; then
    echo "✖ pio check found violations. Commit blocked."
    exit 1
  fi
fi
```

#### CI GitHub Action

Current CI step (from `.github/workflows/linter.yml`):

```yaml
- name: "Static Analysis (platformio check)"
  run: |
    pip install platformio
    pio check --environment esp32dev --skip-packages --fail-on severity=medium
```

---

### 9. Quick Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `pio check` extremely slow | Analyzing libraries | Add `--skip-packages` |
| `clang-tidy: command not found` | Toolchain missing | `pio update` or re-run `pio check` to auto-install |
| False positive on PROGMEM/variable | Analyzer doesn't understand PROGMEM | `// NOLINTNEXTLINE` |
| "Too many errors" (clang-tidy caps at 500) | Too many violations | Fix warnings gradually, use `--severity high` first |
| Different results locally vs CI | Different clang-tidy versions | Run in CI Docker or add `--flags "-*"` to test baseline |
| No violations found but code has bugs | Wrong `check_tool` or `check_flags` | Try `check_tool = clangtidy,clangsa` or add `-checks=*` |

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
