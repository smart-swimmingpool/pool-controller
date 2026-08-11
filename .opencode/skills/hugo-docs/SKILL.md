---
name: hugo-docs
description: "Hugo documentation workflow for pool-controller — docs/ directory structure, Hugo frontmatter conventions (type: docs, menu, weight), shortcodes, i18n (EN/DE), GitHub Actions dispatch to smart-swimmingpool/website, and version management across doc pages. Use when asked to edit, create, update, translate, or publish documentation. 🇩🇪 Deutsche Trigger: Dokumentation, Hugo-Dokumente bearbeiten, Übersetzungen EN/DE, Dokumentenstruktur, Menü-Gewichtung, Hugo-Shortcodes, Docs publizieren, Website-Deployment, smart-swimmingpool.com."
keywords:
  - hugo
  - documentation
  - docs
  - dokumentation
  - frontmatter
  - shortcodes
  - menu
  - weight
  - i18n
  - translation
  - übersetzung
  - smart-swimmingpool.com
  - website
  - repository_dispatch
  - GitHub Actions
  - publish
  - publizieren
  - doc update
  - seo
  - canonical url
  - olimex esp32 c6 evb
---

# Hugo Documentation — Pool Controller

Complete workflow for the Hugo-based documentation in `docs/`. The docs are
published to **[smart-swimmingpool.com](https://www.smart-swimmingpool.com)**
via a two-repo setup: this repo (pool-controller) triggers a rebuild on the
[smart-swimmingpool/website](https://github.com/smart-swimmingpool/website) repo.

## Architecture

```text
pool-controller/docs/              # Source: Hugo markdown + frontmatter
  ├── _index.md                    # EN landing page
  ├── _index.de.md                 # DE landing page
  ├── users-guide.md
  ├── hardware-guide.md
  ├── software-guide.md
  ├── mqtt-configuration.md
  ├── state-persistence.md
  ├── ota-updates.md
  ├── home-assistant/
  │   ├── _index.md                # EN HA integration doc
  │   ├── _index.de.md             # DE HA integration doc
  │   └── dashboard.yaml
  ├── ... (German variants, images, PDFs)
  └── _config.yml                  # GitHub Pages config (legacy/Jekyll fallback)

                │  push to main
                ▼
.github/workflows/notify-website-doc.yml  # repository_dispatch → smart-swimmingpool/website
                │
                ▼
smart-swimmingpool/website/        # Hugo site build → smart-swimmingpool.com
```

## Project-Specific Hugo Conventions

For Olimex local-UI hardware notes, keep the source-of-truth checklist in `docs/olimex-esp32-c6-evb.md` aligned with implementation and `src/Config.hpp` pin names.

### Frontmatter

All documentation files use Hugo frontmatter with these fields:

```yaml
---
title: Page Title # Display title
summary: Brief description for listings # Optional
date: "2020-05-28" # ISO date
lastmod: "2020-06-02" # Last modification date
draft: false # true = hidden from build
toc: true # Show table of contents
type: docs # Required — Hugo content type
featured: true # Featured/important page
tags: ["docs", "controller", "tutorial"] # Taxonomy tags
menu:
  docs: # Menu registration
    parent: Pool Controller # Top-level parent
    name: User Guide # Menu display name
    weight: 40 # Sort order (lower = higher)
---
```

### Weight Convention

| Weight | Page                   | File                |
| ------ | ---------------------- | ------------------- |
| 10     | Overview               | `_index.md`         |
| 20     | Hardware Guide         | `hardware-guide.md` |
| 25     | ESP32 Schaltplan-Opt.  | `esp32-...-de.md`   |
| 26     | ESP32 Komplett-Schalt. | `esp32-...-de.md`   |
| 30     | Software Guide         | `software-guide.md` |
| 40     | Users Guide            | `users-guide.md`    |
| 50     | Home Assistant         | `home-assistant/`   |

### Hugo Shortcodes Used

| Shortcode                | Usage                   |
| ------------------------ | ----------------------- |
| `{{</* figure ... */>}}` | Images with lightbox    |
| `{{%/* alert note */%}}` | Info/note callout boxes |

Examples:

```markdown
{{</* figure library="true" src="pool-controller_breadboard.png"
    title="Breadboard Circuit of Pool Controller" lightbox="true" */>}}

{{%/* alert note */%}}
For an ESP32-focused wiring analysis, see `docs/esp32-schematic-optimization-de.md`.
{{%/* /alert */%}}
```

### i18n: English / German

Each page has an English (`_index.md`) and German (`_index.de.md`) variant.

| File           | Language |
| -------------- | -------- |
| `_index.md`    | English  |
| `_index.de.md` | German   |

When creating new doc pages, always provide both language variants.

### Page Bundles

Some pages use leaf bundles (subdirectories with their own `_index.md`):

```
docs/home-assistant/
  _index.md       # EN
  _index.de.md    # DE
  dashboard.yaml  # Resource file (not a page)
```

## Version Management

### Current Version

The project version is maintained in these locations:

| Source              | File                            | How to update          |
| ------------------- | ------------------------------- | ---------------------- |
| **Source of truth** | `.release-please-manifest.json` | Auto by release-please |
| Firmware build flag | `platformio.ini:37`             | Auto by release-please |
| Fallback            | `src/Version.h`                 | Auto by release-please |
| Docs title EN       | `docs/_index.md:4`              | **Manual**             |
| Docs title DE       | `docs/_index.de.md:4`           | **Manual**             |
| README              | `README.md:1`                   | **Manual**             |

### When to Update Docs Version

- **On every major/minor release**: Update `title: Pool Controller X.Y` in
  `docs/_index.md` and `docs/_index.de.md`
- **On every release**: Update `README.md` title line
- **Version strings in docs**: Always update version references (`Pool Controller X.Y`,
  `vX.Y.Z`) to match the current release

### Common Version Strings to Update

Search for these patterns across docs when bumping versions:

```bash
# Find all version references in docs
grep -rn "Pool Controller [0-9]\.[0-9]" docs/
grep -rn "v[0-9]\+\.[0-9]\+\.[0-9]\+" docs/
```

## Deployment Pipeline

### Trigger

Every push to `main` branch triggers:

```yaml
# .github/workflows/notify-website-doc.yml
name: Alert `website` repository on `main` push
on:
  push:
    branches:
      - main
```

### What It Does

Sends a `repository_dispatch` event to `smart-swimmingpool/website`:

```bash
curl -X POST \
  -H "Authorization: Bearer ${HUGO_DEPLOY_TOKEN}" \
  https://api.github.com/repos/smart-swimmingpool/website/dispatches \
  -d '{"event_type":"doc_update"}'
```

The `website` repo then rebuilds the Hugo site, which pulls the latest
`docs/` content and publishes to **smart-swimmingpool.com**.

### Prerequisites

- `HUGO_DEPLOY_TOKEN` must be set as a repository secret in
  `smart-swimmingpool/pool-controller` (GitHub → Settings → Secrets)
- The token must have `repo` scope for `smart-swimmingpool/website`

### Verification

After pushing to `main`:

1. Go to: <https://github.com/smart-swimmingpool/pool-controller/actions>
2. Check the **"Alert `website` repository on `main` push"** workflow run
3. Wait ~2-3 minutes for the website build
4. Verify at: <https://www.smart-swimmingpool.com>

### Troubleshooting

| Symptom                           | Cause                              | Fix                                           |
| --------------------------------- | ---------------------------------- | --------------------------------------------- |
| Workflow shows warning "skipping" | `HUGO_DEPLOY_TOKEN` not set        | Add the token to repo secrets                 |
| HTTP 401/403 from API             | Token expired or wrong scope       | Regenerate token with `repo` scope            |
| Website not updated               | Build failed on website repo       | Check Actions in `smart-swimmingpool/website` |
| Doc not appearing in menu         | Missing `menu.docs` in frontmatter | Add menu section to frontmatter               |
| Broken layout                     | Missing/wrong `type: docs`         | Ensure `type: docs` is in frontmatter         |

## SEO & Canonical URLs

The `docs/` content is republished from this repo into the Hugo website. To
avoid duplicate content issues:

- Each doc page should be reachable from the website's menu structure
- The website repo sets `canonicalURL` in its Hugo config — no canonical
  meta needed in the source docs
- Keep `summary` frontmatter concise (< 160 chars) for search snippets

## Workflow: Adding a New Doc Page

1. Create the markdown file in `docs/`
2. Add Hugo frontmatter with `type: docs`, `menu.docs`, `weight`
3. Create both EN (`filename.md`) and DE (`filename-de.md`) variants
4. Add Hugo shortcodes for images and alerts as needed
5. Commit and push to `main`
6. Verify deployment via the GitHub Actions workflow
7. Check `smart-swimmingpool.com` after build completes

## Related Skills

| Skill                 | Purpose                                   |
| --------------------- | ----------------------------------------- |
| `deploy`              | Firmware deployment (not docs)            |
| `web-ui`              | ESP32 web dashboard (not Hugo docs)       |
| `mqtt-debug`          | MQTT debugging (referenced in docs)       |
| `platformio-workflow` | Build/flash — used in software guide docs |
