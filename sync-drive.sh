#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REMOTE="${REMOTE:-gdrive:pulsemon}"
INDEX_FILE="${ROOT_DIR}/REPO_INDEX.json"
FILTER_FILE="${ROOT_DIR}/sync-drive.filter"

echo "Génération de REPO_INDEX.json..."
rclone lsjson "${ROOT_DIR}" --recursive --files-only --filter-from "${FILTER_FILE}" |
    python3 -c '
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

entries = json.load(sys.stdin)
files = sorted(
    ({"path": entry["Path"], "size": entry["Size"]} for entry in entries),
    key=lambda entry: entry["path"],
)
index = {
    "project": "PulseMon",
    "generated_at": datetime.now(timezone.utc).isoformat(),
    "file_count": len(files),
    "files": files,
}
Path(sys.argv[1]).write_text(json.dumps(index, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
print(f"{len(files)} fichiers indexés (hors index lui-même)")
' "${INDEX_FILE}"

echo "Synchronisation vers ${REMOTE}..."
rclone sync "${ROOT_DIR}" "${REMOTE}" --filter-from "${FILTER_FILE}"

echo "Publication de l’index..."
rclone copyto "${INDEX_FILE}" "${REMOTE}/REPO_INDEX.json"
echo "Synchronisation terminée."
