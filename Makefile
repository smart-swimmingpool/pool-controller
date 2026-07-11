# Pool Controller - Local Development Tasks
# ==========================================

.PHONY: help lint lint-fix format build clean

# Default target - show help
help:
	@echo "Pool Controller Development Tasks"
	@echo "=================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make lint          - Run MegaLinter to check code quality"
	@echo "  make lint-fix      - Auto-fix linting issues (clang-format, prettier)"
	@echo "  make format        - Format C++ and markdown files"
	@echo "  make build         - Build project (ESP32)"
	@echo "  make clean         - Clean build artifacts"
	@echo ""
	@echo "Before committing, always run: make lint-fix && make lint"
	@echo ""

# Run MegaLinter locally (same config as CI)
# Requires Docker or Node.js (for mega-linter-runner)
# Configuration is in .mega-linter.yml
lint:
	@echo "Running MegaLinter..."
	@echo "Configuration: .mega-linter.yml (c_cpp flavor)"
	@echo ""
	@if command -v npx >/dev/null 2>&1; then \
		echo "→ Using mega-linter-runner..."; \
		npx mega-linter-runner --flavor c_cpp --remove-container; \
	elif command -v docker >/dev/null 2>&1; then \
		echo "→ Using Docker directly..."; \
		docker run --rm -v $(PWD):/tmp/lint:rw \
			-e MEGALINTER_CONFIG=.mega-linter.yml \
			ghcr.io/oxsecurity/megalinter:v9; \
	else \
		echo "✗ Weder npx noch docker gefunden. MegaLinter kann nicht lokal ausgeführt werden."; \
		exit 1; \
	fi
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

# Build for ESP32
build:
	@echo "Building for ESP32..."
	@pio run -e esp32dev
	@echo ""
	@echo "✓ Build complete!"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@pio run --target clean
	@rm -rf .pio/build
	@echo "✓ Clean complete!"
