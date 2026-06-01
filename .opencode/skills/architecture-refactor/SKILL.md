---
name: architecture-refactor
description: "Architecture improvement patterns for the pool-controller — dependency injection, interface extraction, layered architecture, code deduplication. Use when asked to refactor the codebase for better testability, maintainability, or separation of concerns. 🇩🇪 Deutsche Trigger: Architektur verbessern, Dependency Injection, Interfaces extrahieren, Schichtenarchitektur, Code-Duplizierung entfernen, Testbarkeit, Entkopplung, Refactoring."
keywords:
  - architektur
  - architecture
  - dependency injection
  - interfaces
  - schnittstellen
  - schichtenarchitektur
  - layered architecture
  - code duplizierung
  - code deduplication
  - testbarkeit
  - testability
  - entkopplung
  - decoupling
  - refactoring
  - singleton
  - raii
---

# Architecture Refactoring — Pool Controller

Planned architecture improvements for the pool-controller firmware. Target structure from `Agents.md` §3.

> **🔍 Code Search**: Use `semble search "DallasTemperatureNode"` to find tight coupling points. `semble find-related` helps discover all usages of a class before extracting interfaces. See `Agents.md` §7 for full `semble` usage.

## Target Layer Architecture

```
src/
├── app/          # Application logic (use-cases, state machines)
│   ├── PoolController.*
│   ├── OperationModeNode.*
│   ├── Rule*.*
│   └── ...
├── drivers/      # Hardware drivers (no business logic)
│   ├── DallasTemperatureNode.*
│   ├── ESP32TemperatureNode.*
│   ├── RelayModuleNode.*
│   └── ...
├── services/     # Network, storage, time, telemetry
│   ├── NetworkManager.*
│   ├── MqttPublisher.*
│   ├── StateManager.*
│   ├── ConfigManager.*
│   ├── TimeClientHelper.*
│   ├── SystemMonitor.*
│   ├── DegradationManager.*
│   └── ...
├── platform/     # Board-specific adapters (IFDEFs only here)
│   └── ...
└── main.cpp      # Entry point only
```

**Current state**: All modules in flat `src/` directory with tight coupling.

## High-Impact Refactoring Targets

### 1. Extract Interfaces for Hardware Dependencies

**Current**: `OperationModeNode` directly depends on `DallasTemperatureNode*`.
**Target**: Introduce `ITemperatureSensor` interface.

```cpp
// New: include/ITemperatureSensor.hpp
class ITemperatureSensor {
public:
    virtual ~ITemperatureSensor() = default;
    virtual float readTemperature() = 0;
    virtual const char* getId() const = 0;
    virtual const char* getName() const = 0;
    virtual bool begin() = 0;
    virtual void loop() = 0;
};
```

**Files affected**:
- `DallasTemperatureNode` → implements `ITemperatureSensor`
- `ESP32TemperatureNode` → implements `ITemperatureSensor`
- `OperationModeNode` → uses `ITemperatureSensor*` instead of `DallasTemperatureNode*`
- `Rule.hpp` → uses `ITemperatureSensor*` for pool/solar temp providers

### 2. Extract IRelayController Interface

**Current**: `Rule*` subclasses directly control `RelayModuleNode*` instances.
**Target**: Interface for relay control.

```cpp
class IRelayController {
public:
    virtual ~IRelayController() = default;
    virtual void setState(bool on) = 0;
    virtual bool getState() const = 0;
    virtual const char* getId() const = 0;
};
```

### 3. Decouple Rule Base Class

**Current**: `Rule.hpp` is a monolithic base-class with getter/setter for all temperature + timer fields.
**Target**: Slice into focused interfaces or value objects.

### 4. Dependency Injection for ConfigManager

**Current**: Global static singleton pattern. Components call `ConfigManager::getSettings()` directly.
**Target**: Pass config references through constructor parameters for testability.

### 5. Extract `checkPoolPumpTimer()` Duplication

**Current**: `RuleAuto`, `RuleBoost`, `RuleTimer` each call timer logic.
**Target**: Shared `TimerService` or utility function.

### 6. Move Global Singletons to Managed Lifecycle

**Current pattern** (in multiple files):
```cpp
static SomeManager::begin();
static SomeManager::loop();
```

**Target pattern**: Instance managed by `PoolControllerContext`:
```cpp
class PoolControllerContext {
    NetworkManager networkManager_;
    MqttPublisher mqttPublisher_;
    StateManager stateManager_;
    // ...
};
```

## Refactoring Workflow

Each refactoring step should follow the OpenSpec change workflow:
1. **Propose** — describe what to change and why
2. **Design** — specify exact API signatures
3. **Implement** — one task per file/interface
4. **Verify** — build succeeds, lint passes, behavior unchanged
5. **Test** — add unit tests for extracted interfaces

## Testing Strategy

- **Native tests** (PlatformIO `platform = native`) for logic-only code (rules, state machines, timer logic)
- **Hardware-mocked tests** for driver/service code using the extracted interfaces
- **Integration tests** on real hardware for end-to-end verification

## Order of Refactoring (Recommended)

| Phase | Refactoring | Risk | Testability Gain |
|-------|-------------|------|-----------------|
| 1 | Extract `ITemperatureSensor` | Low | High |
| 2 | Extract `IRelayController` | Low | High |
| 3 | Move `checkPoolPumpTimer` to shared utility | Low | Medium |
| 4 | Dependency injection for Config | Medium | High |
| 5 | Instance-based managers (remove static singletons) | High | High |
| 6 | Layered directory structure | Medium | Medium |

## Verification

After each refactoring:
```bash
# Build must pass
make build

# Code quality must not regress
make lint-fix && make lint

# Check for no behavior change in serial output patterns
# Compare before/after for key patterns:
#   - "✓ Controller setup completed"
#   - Mode transitions
#   - Temperature readings
#   - Relay state changes
```
