#!/bin/bash
# Pre-commit hook for Pool Controller
# =====================================
# This script runs linting checks before allowing a commit.
#
# To install this hook:
#   ln -s ../../scripts/pre-commit.sh .git/hooks/pre-commit
#
# To bypass this hook (not recommended):
#   git commit --no-verify

set -e

echo "Running pre-commit checks..."
echo ""

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
  echo "Error: Must be run from repository root"
  exit 1
fi

# Auto-fix formatting issues
echo "Step 1: Auto-fixing formatting..."
make lint-fix || {
  echo "Warning: Auto-fix encountered issues, continuing..."
}
echo ""

# Run linters
echo "Step 2: Running linters..."
make lint || {
  echo ""
  echo "❌ Linting failed!"
  echo ""
  echo "Please fix the issues above before committing."
  echo "Run 'make lint-fix' to auto-fix common issues."
  echo ""
  echo "To bypass this check (not recommended):"
  echo "  git commit --no-verify"
  echo ""
  exit 1
}

echo ""
echo "✓ All pre-commit checks passed!"
echo ""

exit 0
