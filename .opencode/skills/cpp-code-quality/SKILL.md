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
---

# Code Quality — Pool Controller

Linting and formatting standards for the pool-controller project. CI uses Super-Linter v8.3.1 with specific linters enabled.

> **🔍 Code Search**: Use `semble search "lint error"` or `semble search "clang-format violation"` to locate formatting issues. `semble find-related` helps trace patterns across the codebase. See `Agents.md` §7 for full `semble` usage.

## Quick Reference

```bash
# Auto-fix all formatting (C++ + Markdown + YAML)
make lint-fix

# Run full lint check (same as CI)
make lint

# Build check (must pass before commit)
make build
```

## CI Linter Configuration

From `.github/workflows/linter.yml` — these linters are **enabled**:
- `VALIDATE_CPP=true` — cpplint for C++ style
- `VALIDATE_MARKDOWN=true` — markdown-lint
- `VALIDATE_YAML=true` — yaml-lint
- `VALIDATE_JSON=true` — JSON validation
- `VALIDATE_GITHUB_ACTIONS=true` — workflow validation
- `VALIDATE_EDITORCONFIG=true` — EditorConfig compliance
- `VALIDATE_GITLEAKS=true` — secret detection
- `VALIDATE_BASH=true` — shell check

**Note**: `VALIDATE_CLANG_FORMAT` is explicitly disabled (version incompatibility). Use `clang-format -i` locally.

## Common CI Failures & Fixes

### 1. Clang-Format Violations

`.clang-format` enforces (from `CODING_GUIDELINES.md`):
- Max line length: **130 characters**
- Indentation: **2 spaces** (no tabs)
- Brace style: **K&R** (opening brace on same line)
- Pointer alignment: **left** (`int* ptr`)

**Auto-fix**:
```bash
clang-format -i src/**/*.cpp src/**/*.hpp
```

**Common pattern fixes**:

| Issue | Wrong | Right |
|-------|-------|-------|
| Namespace brace | `namespace Foo\n{` | `namespace Foo {` |
| Control flow space | `if(x){` | `if (x) {` |
| Pointer style | `int *ptr` | `int* ptr` |
| NULL → nullptr | `ptr = NULL` | `ptr = nullptr` |
| Empty braces | `Context{ }` | `Context{}` |
| Inline comment spacing | `code;//comment` | `code;  // comment` |

### 2. EditorConfig Issues

`.editorconfig` requires:
- **2 space indentation** everywhere (no tabs)
- **No trailing whitespace**
- **Final newline** at end of every file

**Auto-fix**:
```bash
# Remove tabs
find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec sed -i 's/\t/  /g' {} \;

# Remove trailing whitespace
find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec sed -i 's/[[:space:]]*$//' {} \;
```

### 3. Cpplint Line Length

Cpplint enforces **80 characters** max (stricter than clang-format's 130). Break long lines:

```cpp
// TOO LONG (violates cpplint):
const char* longName = "This is a string that exceeds the eighty character limit for cpplint processing";

// FIXED:
const char* longName =
    "This is a string that exceeds the eighty character limit for cpplint processing";
```

### 4. Include Guard Style

The project uses `#pragma once` — ensure every header has it at the top.

### 5. Type Usage

```cpp
// WRONG (platform-dependent):
unsigned long timestamp;
long value;

// CORRECT (fixed width):
uint32_t timestamp;
int32_t value;
```

Checked files: `src/*.cpp`, `src/*.hpp`, `src/Nodes/*.cpp`, `src/Nodes/*.hpp`.

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

## Pre-Commit Hook

```bash
ln -s ../../scripts/pre-commit.sh .git/hooks/pre-commit
```

The hook runs clang-format on staged `.cpp`/`.hpp` files before each commit.

## Full CI Simulation

```bash
# Run exact same checks as GitHub Actions
make lint
```
