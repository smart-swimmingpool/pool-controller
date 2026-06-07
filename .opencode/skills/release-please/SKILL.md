---
name: release-please
description: "Release-please automatisierte Versionierung und Release-Pipeline für den pool-controller — End-to-End Flow von Conventional Commit bis zum Firmware-Binary auf GitHub. Für Entwickler, die Releases verstehen, triggern oder troubleshooten wollen. 🇩🇪 Deutsche Trigger: Release, Version, Veröffentlichung, semver, Versionierung, Tag, Changelog, Firmware-Binary, GitHub Release, release-please, Pipeline."
keywords:
  - release-please
  - release pipeline
  - semver
  - version management
  - automatic release
  - firmware release
  - changelog
  - github release
  - version bump
  - release PR
  - tag
  - release workflow
  - CI release
  - ota release
  - version annotation
---

# Release-Please — Pool Controller

Dieses Dokument beschreibt die automatisierte Release-Pipeline des pool-controllers.
Sie basiert auf [release-please](https://github.com/googleapis/release-please) und
[Conventional Commits](./conventional-commits/SKILL.md).

## Übersicht

```
Developer Commit (main)
  │  └─ feat:, fix:, chore:, docs:, etc.
  ▼
release-please-action@v4
  │
  ├─ Scannt Commits seit letztem Tag
  ├─ Bestimmt nächste Version (Major/Minor/Patch)
  └─ Erzeugt / aktualisiert Release PR
       │
       ▼
    PR merge (main)
       │
       ▼
  release-please-action@v4
       │
       ├─ Erstellt GitHub Release + Tag (v3.x.y)
       ├─ Aktualisiert CHANGELOG.md
       ├─ Aktualisiert Versionen in:
       │   ├─ platformio.ini (FW_VERSION)
       │   └─ src/Version.h (Fallback)
       │
       ▼
  build-firmware Job
       │
       ├─ Baut firmware.bin (pio run)
       └─ Lädt Binary ins GitHub Release
            │
            ▼
         OTA-Update vom Gerät abrufbar
```

## Konfigurationsdateien

### `release-please-config.json`

```json
{
  "$schema": "https://raw.githubusercontent.com/googleapis/release-please/main/schemas/config.json",
  "release-type": "simple",
  "packages": {
    ".": {
      "changelog-path": "CHANGELOG.md",
      "extra-files": [
        { "type": "generic", "path": "platformio.ini" },
        { "type": "generic", "path": "src/Version.h" }
      ]
    }
  }
}
```

| Feld | Wert | Bedeutung |
|------|------|-----------|
| `release-type` | `simple` | Kein Package-Manager (kein package.json) — Version aus Tags |
| `extra-files` | `platformio.ini`, `Version.h` | Dateien, die release-please beim Bump aktualisiert |
| `type: generic` | — | `x-release-please-version`-Annotation in der Datei |

### `.release-please-manifest.json`

```json
{ ".": "3.3.0" }
```

Aktuelle Version des Projekts. Wird von release-please automatisch aktualisiert.

## Version-Annotations

### `platformio.ini` (Zeile 37)

```ini
'-D FW_VERSION="3.3.0"'  # x-release-please-version
```

Release-please ersetzt den String zwischen den Anführungszeichen auf dieser Zeile.
Der Build-Flag definiert `FW_VERSION` für den Compiler — die tatsächliche Version im Binary.

### `src/Version.h` (Zeile 11)

```cpp
#define FW_VERSION "0.0.0"  // x-release-please-version
```

Fallback für Kompilierung ohne Build-Flag. Wird von release-please auf die
Release-Version aktualisiert, bleibt aber normalerweise hinter dem Build-Flag zurück.

> **Wichtig**: Die `x-release-please-version`-Annotation MUSS auf derselben Zeile
> wie der zu ersetzende String stehen. Ohne diese Annotation wird die Datei ignoriert.
> release-please v4 verwendet `type: "generic"` statt des alten `search-regex`-Mechanismus.

## Release-Pipeline (`release.yml`)

```yaml
name: Release
on:
  push:
    branches: [main]
```

### Job 1: `release-please`

Läuft bei **jedem Push auf main**:

1. Scannt alle neuen Commits seit dem letzten Release-Tag
2. Bestimmt die nächste Version basierend auf Conventional Commit Types:

| Commits enthalten | Bump | Beispiel |
|---|---|---|
| `feat!` oder `BREAKING CHANGE` | **Major** (X.0.0) | v3.0.0 → v4.0.0 |
| `feat` (mind. einer) | **Minor** (x.Y.0) | v3.2.0 → v3.3.0 |
| `fix` (nur, keine `feat`) | **Patch** (x.y.Z) | v3.3.0 → v3.3.1 |
| Nur `chore`/`docs`/`ci`/`refactor` | **Kein Release** | — |

3. Erzeugt/aktualisiert einen **Release PR** mit:
   - Berechneter nächster Version im Titel (z. B. `chore(main): release 3.3.1`)
   - Automatisch generiertem CHANGELOG-Eintrag
   - Aktualisierten Versionen in `extra-files`

4. **Wird der Release PR gemerged**, erstellt release-please:
   - GitHub Release mit Tag `v3.x.y`
   - Aktualisiert `CHANGELOG.md` auf main
   - Aktualisiert `platformio.ini` + `Version.h` auf main
   - Setzt Output `release_created = true`

### Job 2: `build-firmware`

Läuft NUR wenn `release-please` einen Release erstellt hat:

```yaml
needs: release-please
if: needs.release-please.outputs.release_created == 'true'
```

1. Checkout mit `actions/checkout@v6`
2. Cache: `~/.platformio` + `.pio` (PlatformIO-Framework + Build-Artefakte)
3. Installiert PlatformIO via pip
4. Baut `firmware.bin`: `pio run --environment esp32dev`
5. Lädt `firmware.bin` ins GitHub Release hoch:
   ```bash
   gh release upload "$TAG_NAME" ".pio/build/esp32dev/firmware.bin#firmware-esp32dev.bin"
   ```
   Der `#firmware-esp32dev.bin`-Teil setzt den Anzeigenamen im Release.

## Developer Workflow

### Normale Entwicklung (kein Release-Wissen nötig)

```bash
git checkout -b feat/my-feature
# ... Änderungen ...
git commit -m "feat(ha): add heater climate entity"
git push
# → PR erstellen, reviewen, mergen
# → release-please erzeugt automatisch Release PR bei Bedarf
```

### Release manuell triggern

Ein Release wird automatisch erzeugt, sobald ein Release PR gemerged wird.
Der Release PR entsteht automatisch, wenn neue relevante Commits (`feat`/`fix`)
auf main kommen.

Soll sofort ein Release erzwungen werden:
```
git commit --allow-empty -m "fix: trigger immediate patch release"
git push
```

### Release PR manuell mergen

1. Release PR in GitHub öffnen (Titel: `chore(main): release x.y.z`)
2. PR reviewen (CHANGELOG, Version)
3. Mergen
4. → Release wird automatisch erstellt + Firmware gebaut + Binary hochgeladen

### Release-Firmware per OTA installieren

Im WebUI unter **System → Check for Updates**:
- Gerät prüft GitHub Releases auf neue Version
- Bei neuere Version: **Install Update** klicken
- Firmware wird direkt vom GitHub Release geladen und geflasht

## Troubleshooting

### Release PR wird nicht erzeugt

**Ursache**: Nur Commits ohne Release-Relevanz (`chore`, `docs`, `ci`, `refactor`)
seit letztem Release.

**Fix**: Einen `fix:` oder `feat:` Commit auf main bringen:
```bash
git commit --allow-empty -m "fix: trigger release"
git push
```

### Release PR existiert, wird aber nicht aktualisiert

**Ursache**: release-please aktualisiert den PR nur bei neuen Commits auf main.
Wenn seit Erstellung des PR nur `chore`-Commits kamen, bleibt der PR unverändert.

**Fix**: `feat:` oder `fix:` Commit auf main bringen → release-please aktualisiert den PR.

### `release_created` ist false obwohl PR gemerged

**Ursache**: Der Merge-Commit des Release PR muss von release-please erkannt werden.
Mögliche Probleme:
- Merge-Konflikt beim Mergen → manueller Merge statt PR-Merge-Button
- Branch-Schutzregeln verhindern Auto-Merge

**Fix**: Release PR über den GitHub PR-Merge-Button mergen (nicht manuell per `git merge`).

### Firmware-Build schlägt fehl

**Ursache**: Meist Compiler-Fehler oder fehlende Dependencies.

**Fix**:
1. Build lokal testen: `pio run --environment esp32dev`
2. Fehler beheben
3. Neuen `fix:` Commit auf main → release-please aktualisiert den Release PR
4. Release PR erneut mergen

### Firmware-Binary fehlt im Release

**Ursache**: `build-firmware` Job war deaktiviert oder fehlgeschlagen.

**Fix**: Manuell nachreichen:
```bash
gh release upload v3.x.y .pio/build/esp32dev/firmware.bin#firmware-esp32dev.bin
```

### Falsche Version im Binary

**Ursache**: `x-release-please-version`-Annotation in `platformio.ini` fehlt oder ist
verschoben.

**Fix**: Prüfen, dass genau eine Zeile in `platformio.ini` das `x-release-please-version`
enthält und die FW_VERSION darauf steht:
```ini
'-D FW_VERSION="3.3.0"'  # x-release-please-version
```

## Referenzen

- [release-please Dokumentation](https://github.com/googleapis/release-please)
- [release-please-action@v4](https://github.com/googleapis/release-please-action)
- [Conventional Commits](https://www.conventionalcommits.org/)
- Verwandte Skills:
  - `conventional-commits` — Commit-Format, semver, Changelog
  - `deploy` — Build, Flash, OTA, uploadfs
  - `platformio-env` — Build-Umgebung, platformio.ini
