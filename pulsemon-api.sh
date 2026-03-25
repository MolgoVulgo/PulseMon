#!/usr/bin/env bash
set -euo pipefail

APP_DIR="/usr/lib/pulsemon-api"

exec /usr/bin/python -m uvicorn app.main:app \
  --app-dir "${APP_DIR}" \
  --host "${STATS_BIND_HOST:-0.0.0.0}" \
  --port "${STATS_BIND_PORT:-8000}" \
  --log-level "${STATS_UVICORN_LOG_LEVEL:-info}" \
  "$@"
