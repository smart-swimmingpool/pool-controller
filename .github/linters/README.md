# Super-Linter Configuration

This directory contains configuration files for the [GitHub Super-Linter](https://github.com/github/super-linter) action.

## Configuration Files

### `.super-linter.yml`
Main configuration file for Super-Linter. Optimized for PlatformIO embedded projects:

- **Excludes**: Build artifacts (`.pio`, `.platformio`, `build`), libraries (`lib/`), and test files
- **C/C++ Linting**: Only validates source code in `src/` directory
- **Enabled Linters**: 
  - C/C++ (clang-format, cpplint)
  - Markdown
  - YAML
  - JSON
  - GitHub Actions
  - EditorConfig
  - GitLeaks (secret scanning)
  - Bash/Shell scripts
- **Disabled Linters**: Languages not used in this project (Go, Python, Ruby, etc.)

### `.markdown-lint.yml`
Markdown linting rules:
- Line length: 120 characters
- List indentation: 2 spaces
- Allows inline HTML for documentation
- Consistent list style (dash)

### `.yaml-lint.yml`
YAML linting rules:
- Line length: 120 characters (warning level)
- Indentation: 2 spaces
- Lenient truthy checking
- Document start marker optional

## Usage

The linter runs automatically on all pushes (except to `master` branch) via the GitHub Actions workflow at `.github/workflows/linter.yml`.

### Running Locally

To run the linter locally:

```bash
# Using Docker (recommended)
docker run --rm -e RUN_LOCAL=true -e USE_FIND_ALGORITHM=true \
  -v $(pwd):/tmp/lint github/super-linter:latest

# Or using the GitHub CLI
gh api repos/:owner/:repo/actions/workflows/linter.yml/dispatches \
  -f ref=your-branch-name
```

## Customization

### Excluding Files

To exclude specific files or patterns, edit `FILTER_REGEX_EXCLUDE` in `.super-linter.yml`.

### Adjusting Rules

- **C/C++**: Edit `.clang-format` in the project root
- **Markdown**: Edit `.markdown-lint.yml` in this directory
- **YAML**: Edit `.yaml-lint.yml` in this directory
- **EditorConfig**: Edit `.editorconfig` in the project root

## Troubleshooting

### Linter Failing on Library Code

The configuration excludes `lib/` and `.pio/` directories. If linter still processes library code, check:
1. The `FILTER_REGEX_EXCLUDE` pattern in `.super-linter.yml`
2. The workflow's `fetch-depth` setting (should be 0 for full history)

### Too Many Warnings

Set `WARNINGS_AS_ERRORS: false` in the workflow file or `.super-linter.yml` to prevent warnings from failing the build.

### Specific Linter Issues

Disable individual linters by setting `VALIDATE_<LANGUAGE>: false` in `.github/workflows/linter.yml`.

## References

- [Super-Linter Documentation](https://github.com/github/super-linter)
- [Markdownlint Rules](https://github.com/DavidAnson/markdownlint/blob/main/doc/Rules.md)
- [YAML Lint Rules](https://yamllint.readthedocs.io/en/stable/rules.html)
- [EditorConfig](https://editorconfig.org/)
