# Pool Controller - Local Development Tasks
# ==========================================

.PHONY: help lint lint-fix format build clean

# Default target - show help
help:
	@echo "Pool Controller Development Tasks"
	@echo "=================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make lint          - Run Super-Linter to check code quality"
	@echo "  make lint-fix      - Auto-fix linting issues (clang-format, prettier)"
	@echo "  make format        - Format C++ and markdown files"
	@echo "  make build         - Build project for all platforms"
	@echo "  make clean         - Clean build artifacts"
	@echo ""
	@echo "Before committing, always run: make lint-fix && make lint"
	@echo ""

# Run Super-Linter locally (same config as CI)
lint:
	@echo "Running Super-Linter..."
	@echo "Note: This uses the same configuration as GitHub Actions CI"
	@docker run --rm \
	  -e VALIDATE_ALL_CODEBASE=false \
	  -e DEFAULT_BRANCH=main \
	  -e FILTER_REGEX_EXCLUDE=.*/(\.pio|\.vscode|\.platformio|build|lib)/.* \
	  -e VALIDATE_CPP=true \
	  -e VALIDATE_CLANG_FORMAT=true \
	  -e VALIDATE_MARKDOWN=true \
	  -e VALIDATE_YAML=true \
	  -e VALIDATE_JSON=true \
	  -e VALIDATE_GITHUB_ACTIONS=true \
	  -e VALIDATE_EDITORCONFIG=true \
	  -e VALIDATE_GITLEAKS=true \
	  -e VALIDATE_BASH=true \
	  -e CPP_FILE_EXTENSIONS=cpp,hpp,h \
	  -e MARKDOWN_CONFIG_FILE=.markdown-lint.yml \
	  -e YAML_CONFIG_FILE=.yaml-lint.yml \
	  -e WARNINGS_AS_ERRORS=false \
	  -e LOG_LEVEL=NOTICE \
	  -v $(PWD):/tmp/lint \
	  --workdir /tmp/lint \
	  ghcr.io/super-linter/super-linter:v7.4.0
	@echo ""
	@echo "✓ Linting complete!"

# Auto-fix common linting issues
lint-fix:
	@echo "Auto-fixing linting issues..."
	@echo ""
	@echo "1. Formatting C++ files with clang-format..."
	@clang-format -i src/**/*.cpp src/**/*.hpp 2>/dev/null || \
	  (find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i)
	@echo "   ✓ C++ files formatted"
	@echo ""
	@echo "2. Formatting Markdown files with Prettier..."
	@npx prettier@3.5.3 --write "**/*.md" \
	  --ignore-path .gitignore \
	  --log-level warn
	@echo "   ✓ Markdown files formatted"
	@echo ""
	@echo "3. Formatting YAML files with Prettier..."
	@npx prettier@3.5.3 --write "**/*.yml" "**/*.yaml" \
	  --ignore-path .gitignore \
	  --log-level warn
	@echo "   ✓ YAML files formatted"
	@echo ""
	@echo "✓ Auto-fix complete! Now run 'make lint' to verify."

# Format C++ and Markdown files (alias for lint-fix)
format: lint-fix

# Build for ESP32 and ESP8266
build:
	@echo "Building for ESP32..."
	@pio run -e esp32dev
	@echo ""
	@echo "Building for ESP8266..."
	@pio run -e nodemcuv2
	@echo ""
	@echo "✓ Build complete for all platforms!"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@pio run --target clean
	@rm -rf .pio/build
	@echo "✓ Clean complete!"
