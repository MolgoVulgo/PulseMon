#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OUT="PulseMon.zip"
TMP="${OUT}.tmp"

rm -f "$OUT" "$TMP"

EXCLUDES=(
  "./$OUT"
  "./$TMP"

  "./.agents/*"
  "./.codex/*"
  "./.git/*"
  "./.venv/*"
  "./.vscode/*"
  "./.pio/*"
  "./.pio-core/*"
  "./tmp/*"

  "./api/.venv/*"
  "./api/.pytest_cache/*"
  "./api/node_modules/*"
  "./api/test-results/*"
  "./api/playwright-report/*"

  "./esp/.agents/*"
  "./esp/.codex/*"
  "./esp/.pio/*"
  "./esp/.vscode/*"
  "./esp/build/*"
  "./esp/src/ui/.eez-project-build/*"

  "*/.pytest_cache/*"
  "*/__pycache__/*"
  "*/node_modules/*"

  "*.log"
  "*.pyc"
  ".DS_Store"
  "Thumbs.db"
  ".env"
  ".env.*"
  "./sdkconfig"
  "./sdkconfig.*"
  "./esp/sdkconfig"
  "./esp/sdkconfig.*"
)

zip -r "$TMP" . -x "${EXCLUDES[@]}"

mv "$TMP" "$OUT"

echo "Archive créée : $OUT"
