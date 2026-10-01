#!/usr/bin/env bash
# Pre-compress web assets for LittleFS upload.
#
# The firmware serves the .gz variants with Content-Encoding: gzip when they
# are present (see WebPortal::serveWebFile), cutting the first-visit transfer
# from ~104 KB to ~35 KB. Run this before deploying web assets:
#
#   scripts/gzip-web-assets.sh
#   pio run --target uploadfs          # serial
#   # or the OTA /api/fs/upload flow   # network
#
# Deployments without this step keep working — the firmware falls back to the
# uncompressed files. Generated .gz files are gitignored.
set -euo pipefail

cd "$(dirname "$0")/../data/web"

for f in index.html style.css app.js sw.js manifest.json icon.svg; do
  if [ ! -f "$f" ]; then
    echo "skip $f (missing)"
    continue
  fi
  gzip -n -9 -c "$f" >"$f.gz"
  echo "gzipped $f ($(wc -c <"$f") -> $(wc -c <"$f.gz") bytes)"
done
