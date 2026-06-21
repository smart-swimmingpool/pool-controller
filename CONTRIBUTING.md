# Contributing to Pool Controller

Thank you for your interest in contributing to the **Smart Swimming Pool Controller** project!
🏊‍♂️

This document provides guidelines for contributing to the project. Please read it
carefully before submitting your first pull request.

## 📋 Table of Contents

- [Code of Conduct](#-code-of-conduct)
- [How to Contribute](#-how-to-contribute)
- [Getting Started](#-getting-started)
- [Development Workflow](#-development-workflow)
- [Coding Standards](#-coding-standards)
- [Security Guidelines](#-security-guidelines)
- [Testing](#-testing)
- [Pull Request Process](#-pull-request-process)
- [Review Process](#-review-process)
- [Commit Message Guidelines](#-commit-message-guidelines)
- [Quality Gates](#-quality-gates)
- [Resources](#-resources)

---

## 🤝 Code of Conduct

This project adheres to the
[Contributor Covenant](https://www.contributor-covenant.org/version/1/4/code-of-conduct.html).
By participating, you are expected to uphold this code. Please report unacceptable
behavior to
[project maintainers](https://github.com/smart-swimmingpool/pool-controller/graphs/contributors).

See also: [code-of-conduct.md](code-of-conduct.md)

---

## 🌟 How to Contribute

### Reporting Bugs

- **Check existing issues**: Search
  [GitHub Issues](https://github.com/smart-swimmingpool/pool-controller/issues)
  before creating a new one
- **Use the issue template**: Provide detailed information about the bug
- **Include**:
  - Firmware version (from Web Dashboard or serial monitor)
  - Hardware setup (ESP32 model, relay module, sensors)
  - Steps to reproduce
  - Serial monitor output (if applicable)
  - Screenshots (if UI-related)

### Suggesting Enhancements

- **Check the roadmap**: See [README.md](README.md) for planned features
- **Discuss first**: Open a
  [GitHub Discussion](https://github.com/smart-swimmingpool/pool-controller/discussions)
  to discuss your idea
- **Check for duplicates**: Search existing issues and PRs

### Submitting Pull Requests

- **Fork the repository**: Create your own fork
- **Create a feature branch**: Use descriptive branch names
  (e.g., `feat/add-timer-functionality`)
- **Follow coding standards**: See
  [Coding Guidelines](.github/CODING_GUIDELINES.md)
- **Test your changes**: Ensure all tests pass
- **Update documentation**: Keep docs in sync with code changes

---

## 🚀 Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- [Git](https://git-scm.com/) installed
- Basic knowledge of C++ and ESP32 development
- Understanding of MQTT protocol (for Home Assistant integration)

### Setting Up the Development Environment

```bash
# Clone the repository
git clone https://github.com/smart-swimmingpool/pool-controller.git
cd pool-controller

# Install dependencies (handled by PlatformIO)
pio run -e esp32dev

# Run local linting
make lint-fix && make lint

# Build the project
make build
```

### Project Structure

```
pool-controller/
├── src/                  # Source code
│   ├── PoolController/    # Main controller classes
│   ├── Nodes/             # Sensor and relay nodes
│   └── main.cpp           # Entry point
├── data/                 # Web assets and configuration
│   └── web/               # Web interface files
├── docs/                 # Documentation
├── .github/              # GitHub configuration
│   ├── CODING_GUIDELINES.md  # Coding standards
│   └── workflows/         # CI/CD workflows
├── platformio.ini        # PlatformIO configuration
└── README.md             # Project overview
```

---

## 🔄 Development Workflow

### 1. Create a Feature Branch

```bash
# From main branch
git checkout main
git pull origin main

# Create new feature branch
git checkout -b feat/your-feature-name
```

**Branch Naming Conventions**:
- `feat/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation updates
- `refactor/` - Code refactoring
- `chore/` - Maintenance tasks
- `security/` - Security-related changes

### 2. Make Your Changes

- Follow [Coding Guidelines](.github/CODING_GUIDELINES.md)
- Keep changes focused and minimal
- Update relevant documentation
- Add tests if applicable

### 3. Run Quality Checks

```bash
# Auto-fix linting issues
make lint-fix

# Verify linting passes
make lint

# Build for ESP32
make build

# Clean up
make clean
```

### 4. Commit Your Changes

Follow [Conventional Commits](#-commit-message-guidelines) guidelines.

### 5. Push to Your Fork

```bash
git push origin feat/your-feature-name
```

### 6. Create a Pull Request

- Go to
  [GitHub Pull Requests](https://github.com/smart-swimmingpool/pool-controller/pulls)
- Click "New Pull Request"
- Select your branch
- Fill out the PR template
- Link to any related issues

---

## 📖 Coding Standards

### General Principles

- **Follow existing patterns**: Match the style and architecture of existing code
- **Keep it simple**: Prefer simple, readable code over clever optimizations
- **Document your code**: Use meaningful comments and docstrings
- **Handle errors gracefully**: Don't ignore error conditions

### C++ Specific

See [CODING_GUIDELINES.md](.github/CODING_GUIDELINES.md) for detailed C++ standards.

**Key Rules**:
- Maximum line length: **130 characters** (clang-format)
- Indentation: **2 spaces** (no tabs)
- Brace style: **K&R** (opening brace on same line)
- Pointer alignment: **Left** (`int* ptr`)
- Use `nullptr` instead of `NULL`
- Use fixed-width types (`uint32_t`, `int16_t`, etc.)

### ESP32 Specific

- **Avoid heap fragmentation**: Minimize `String` usage in hot paths
- **Use `constexpr`**: For compile-time constants
- **Watchdog awareness**: Feed the watchdog in long operations
- **Memory constraints**: ESP32 has ~320KB RAM - be mindful of allocations

### Security Considerations

- **Never hardcode credentials**: Use configuration files or secure storage
- **Validate all inputs**: Especially from network sources
- **Use secure protocols**: TLS for MQTT, HTTPS for web
- **Sanitize outputs**: Prevent injection attacks

---

## 🔒 Security Guidelines

### Reporting Security Vulnerabilities

**DO NOT** report security vulnerabilities through public GitHub issues. Instead:

1. **Private disclosure**: Email the maintainers directly
2. **Responsible disclosure**: Allow reasonable time for fixes
3. **Coordinate**: Work with maintainers on the fix

### Security Best Practices

- **Credentials**: Never commit passwords, API keys, or secrets
- **Gitleaks**: Run `gitleaks detect --source .` before committing
- **CodeQL**: Security analysis runs automatically in CI
- **Dependencies**: Keep dependencies updated (Dependabot enabled)

### Security Checklist for PRs

- [ ] No hardcoded credentials or secrets
- [ ] Input validation for all user inputs
- [ ] Secure communication protocols used
- [ ] Error messages don't reveal sensitive information
- [ ] Logging doesn't contain sensitive data
- [ ] Memory management prevents leaks

See also: [Security References](docs/security-references.md)

---

## 🧪 Testing

### Local Testing

```bash
# Build and run tests
make build

# Run PlatformIO tests
pio test
```

### Test Coverage

- **Unit tests**: For core functionality (native build)
- **Integration tests**: For component interactions
- **Manual testing**: For hardware-specific features

### Test Requirements

- All existing tests must pass
- New features should include tests
- Bug fixes should include regression tests

---

## 📤 Pull Request Process

### Before Submitting

1. **Self-review**: Check your own code
2. **Run linting**: `make lint-fix && make lint`
3. **Build successfully**: `make build`
4. **Test changes**: Ensure all tests pass
5. **Update docs**: Keep documentation in sync

### PR Template

Use the GitHub PR template and include:

- **Clear title**: Descriptive and concise
- **Detailed description**: What, why, and how
- **Related issues**: Link to any relevant issues
- **Breaking changes**: Note any breaking changes
- **Testing**: How you tested the changes

### PR Requirements

- **Green CI**: All GitHub Actions must pass
- **No merge conflicts**: Rebase if necessary
- **Proper commits**: Follow commit message guidelines
- **Code review**: At least one approval required

---

## 👀 Review Process

### What Reviewers Look For

1. **Code Quality**
   - Follows coding standards
   - Clean, readable code
   - Proper error handling
   - Good documentation

2. **Functionality**
   - Does it work as intended?
   - Are there edge cases to consider?
   - Is the implementation efficient?

3. **Security**
   - No security vulnerabilities
   - Proper input validation
   - Secure defaults

4. **Testing**
   - Are there tests?
   - Do existing tests still pass?
   - Is the code testable?

5. **Documentation**
   - Is documentation updated?
   - Are there clear commit messages?
   - Is the PR description informative?

### Review Timeline

- **Small changes**: Typically reviewed within 1-2 days
- **Large changes**: May take longer, especially for complex features
- **Security changes**: Require thorough review

### Addressing Feedback

- **Be responsive**: Reply to review comments promptly
- **Make changes**: Update your PR based on feedback
- **Push updates**: Amend commits or add new ones
- **Notify reviewers**: Tag reviewers when ready for re-review

---

## 📝 Commit Message Guidelines

We follow [Conventional Commits](https://www.conventionalcommits.org/) format:

```
type(scope): subject

body

footer
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only changes
- `style`: Changes that do not affect the meaning of the code
  (white-space, formatting, missing semi-colons, etc)
- `refactor`: Code change that neither fixes a bug nor adds a feature
- `perf`: Code change that improves performance
- `test`: Adding missing tests
- `chore`: Changes to the build process or auxiliary tools
- `revert`: Reverts a previous commit
- `security`: Security-related changes

### Examples

```bash
# Good commit messages
feat(mqtt): Add Home Assistant discovery support
fix(web): Correct session timeout handling
docs: Update hardware guide with new pin assignments
refactor(utils): Extract string formatting functions
security: Add CSRF protection to web portal
chore: Update PlatformIO dependencies
```

### Subject Line

- Use **imperative mood** ("Add feature" not "Added feature")
- **Capitalize first letter**
- **No period** at the end
- **Keep under 50 characters**

### Body

- Explain **what** was changed
- Explain **why** the change was made
- Reference **issues** or **PRs** if applicable

---

## ✅ Quality Gates

All pull requests must pass the following quality gates:

### 1. Super-Linter CI ✅

- **EditorConfig**: File formatting compliance
- **CPP Lint**: C++ style checking (cpplint)
- **Markdown**: Documentation formatting
- **YAML**: Workflow file validation
- **Gitleaks**: Secret detection

Run locally:
```bash
make lint
```

### 2. PlatformIO CI ✅

- **Build**: Successful compilation for both environments
  - `esp32dev` (standard ESP32)
  - `norvi_ae01_r` (NORVI industrial controller)

Run locally:
```bash
make build
```

### 3. CodeQL Analysis ✅

- **Security**: No critical security vulnerabilities
- **Code Quality**: No major code quality issues

### 4. Manual Review ✅

- **Code Review**: At least one approval
- **Functionality**: Changes work as intended
- **Documentation**: All docs updated

---

## 📚 Resources

### Documentation

- **[README.md](README.md)** - Project overview and quick start
- **[Quick Start Guide](docs/quick-start.md)** - Step-by-step setup
- **[Hardware Guide](docs/hardware-guide.md)** - Assembly and wiring
- **[MQTT Configuration](docs/mqtt-configuration.md)** - Home Assistant setup
- **[Coding Guidelines](.github/CODING_GUIDELINES.md)** - Detailed coding standards
- **[Security References](docs/security-references.md)** - Security best practices

### Development

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/)
- [Arduino Documentation](https://www.arduino.cc/en/Reference/HomePage)

### Community

- [GitHub Discussions](https://github.com/smart-swimmingpool/pool-controller/discussions)
  - Ask questions, discuss ideas
- [Home Assistant Community](https://community.home-assistant.io/)
  - Smart home integration
- [DIY My Smart Home (Medium)](https://medium.com/diy-my-smart-home)
  - Project blog

### Related Projects

- [Smart Swimming Pool Organization](https://github.com/smart-swimmingpool)
  - Other related projects
- [Home Assistant](https://www.home-assistant.io/)
  - Smart home platform

---

## 🙏 Recognition

Your contributions are valuable and appreciated! All contributors are listed in the
[contributors graph](https://github.com/smart-swimmingpool/pool-controller/graphs/contributors).

For significant contributions, you may be invited to become a maintainer.

---

## 📜 License

By contributing to this project, you agree that your contributions will be licensed
under the [MIT License](LICENSE).

---

**📅 Last Updated**: 2025-01-15

**🤖 Generated by**: Vibe Code - IoT Security Analysis

**📝 Related PR**:
[#112 - IoT Security & Memory Optimization Analysis](https://github.com/smart-swimmingpool/pool-controller/pull/112)
