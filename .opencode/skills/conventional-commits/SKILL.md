---
name: conventional-commits
description: "Conventional Commits standard for the pool-controller project — commit message format, allowed scopes, release-please integration, and real examples from the project history. Use when writing commit messages, reviewing PRs, or preparing releases. 🇩🇪 Deutsche Trigger: Conventional Commits, Commit-Nachricht, Commit-Format, Release, semver, Changelog, Commit-Richtlinien."
keywords:
  - conventional commits
  - commit message
  - commit format
  - semver
  - release
  - changelog
  - release-please
  - commit style
  - git commit
  - commit guidelines
  - commit conventions
---

# Conventional Commits — Pool Controller

The project uses [Conventional Commits](https://www.conventionalcommits.org/) for structured commit messages.
This integrates with `release-please` to automate semver version bumps and changelog generation.

## Specification

```
<type>(<optional scope>): <subject>

<body (optional)>

<footer (optional)>
```

### Type

| Type | Release bump | Description |
|------|-------------|-------------|
| `fix` | **Patch** (x.y.**Z**) | Bug fix |
| `feat` | **Minor** (x.**Y**.0) | New feature |
| `feat!` | **Major** (**X**.0.0) | Breaking change |
| `fix!` | **Major** (**X**.0.0) | Breaking bug fix |
| `chore` | *No release* | Maintenance, deps, tooling |
| `docs` | *No release* | Documentation only |
| `refactor` | *No release* | Code restructuring |
| `test` | *No release* | Test additions/changes |
| `style` | *No release* | Formatting, linting |
| `perf` | *No release* | Performance improvement |
| `ci` | *No release* | CI/CD config changes |
| `build` | *No release* | Build system changes |

> `!` after the type/scope denotes a **breaking change** — always triggers a major version bump.

### Scope (project-specific)

Optional but recommended. Scopes are lowercase, short identifiers.

| Scope | Area | Example |
|-------|------|---------|
| *(none)* | General cross-cutting | `fix: correct temperature rounding` |
| `ha` | Home Assistant / MQTT Discovery | `feat(ha): add heater climate entity` |
| `wifi` | WiFi, WPS, provisioning | `feat(wifi): add WPS onboarding` |
| `web` | Web UI dashboard | `fix(web): update version display` |
| `mqtt` | MQTT communication | `fix(mqtt): increase buffer size` |
| `sensor` | Temperature sensors (DS18B20, DHT) | `fix(sensor): handle disconnected probe` |
| `relay` | Pump relay control | `feat(relay): add PWM speed control` |
| `config` | ConfigManager, NVS, persistence | `feat(config): add MQTT TLS toggle` |
| `system` | SystemMonitor, watchdog, reliability | `feat(system): add boot-loop detection` |
| `ota` | OTA updates | `fix(ota): verify signature before flashing` |
| `ntp` | NTP, timezone, time sync | `fix(ntp): handle DST transition` |
| `ci` | CI/CD, GitHub Actions | `chore(ci): update platformio version` |
| `deps` | Library dependencies | `chore(deps): bump ArduinoJson to 7.3.0` |
| `docs` | Documentation | `docs: add MQTT configuration guide` |

### Subject

- Imperative present tense ("fix", "add", "remove" — *not* "fixed", "added")
- No trailing period
- Max **72 characters**
- Lowercase after type/scope

### Body (optional)

- Use for motivation, context, or explanation
- Wrap at 72 characters
- Separate from subject by blank line

### Footer (optional)

- `BREAKING CHANGE:` for breaking changes (also triggers major bump)
- `Reviewed-by:`, `Refs:`, `Co-authored-by:`, etc.

## Real Examples from Project History

### Feature

```
feat(ha): replace timer H/Min number entities with HH:MM text entities

The separate hour/minute number entities were confusing in Home Assistant.
Replace them with a single text entity accepting "HH:MM" format.

BREAKING CHANGE: Timer configuration entities have changed.
Home Assistant automations referencing the old entities must be updated.
```

```
feat(wifi): add WPS onboarding for initial WiFi provisioning
```

```
feat(web): add OTA firmware upload via web interface
```

### Fix

```
fix(sensor): handle disconnected DS18B20 gracefully
```

```
fix(web): correct temperature alignment on mobile
```

### Chore / Maintenance

```
chore(deps): bump ArduinoJson from 6.x to 7.3.0
```

```
chore(ci): update release-please-action to v4
```

### Breaking Change

```
feat!: drop ESP8266 support, migrate to ESP32 only
```

```
refactor!: restructure ConfigManager API

BREAKING CHANGE: getSettings() now returns a const reference
instead of a mutable reference. Use setSettings() for updates.
```

## release-please Integration

The CI pipeline (`release.yml`) uses `release-please-action@v4`:

1. On push to `main`, release-please scans commits since last release
2. Determines next version based on commit types:
   - At least one `feat!` or `BREAKING CHANGE` → **Major**
   - At least one `feat` → **Minor**
   - Everything else with conventional commits → **Patch**
   - Only `chore`/`docs`/`refactor` → **No release**
3. Creates/updates a Release PR with changelog and version bumps
4. On merge, creates GitHub Release + tag + triggers firmware build

## Anti-patterns

| ❌ Bad | ✅ Good | Why |
|--------|---------|-----|
| `fixed sensor bug` | `fix(sensor): handle disconnected DS18B20` | Missing type/scope, vague |
| `feat: Added new feature` | `feat: add OTA update support` | Past tense, not imperative |
| `fix(web): fixed alignment` | `fix(web): correct temperature alignment` | Past tense |
| `chore: fix stuff` | `fix: correct temperature hysteresis calculation` | Wrong type, vague |
| `Update file.txt` | `docs: update MQTT configuration example` | Not a conventional commit |
| `feat: big update` | `feat(web): redesign dashboard layout` | Vague, no scope, no specifics |

## Related

- [Conventional Commits spec](https://www.conventionalcommits.org/)
- [release-please docs](https://github.com/googleapis/release-please)
- Project skill: `deploy` — release pipeline and version management
- Project file: `release-please-config.json` — configuration
