#!/usr/bin/env bash
set -euo pipefail

APP_DIR="/usr/lib/pulsemon-api"

# Validate the complete STATS_* contract before Uvicorn parses its own CLI.
PYTHONPATH="${APP_DIR}" /usr/bin/python -m app.config

UVICORN_LOG_LEVEL="${STATS_LOG_LEVEL:-INFO}"
case "${UVICORN_LOG_LEVEL^^}" in
  WARN) UVICORN_LOG_LEVEL="WARNING" ;;
  FATAL) UVICORN_LOG_LEVEL="CRITICAL" ;;
  *) UVICORN_LOG_LEVEL="${UVICORN_LOG_LEVEL^^}" ;;
esac
UVICORN_LOG_LEVEL="${UVICORN_LOG_LEVEL,,}"

exec /usr/bin/python -m uvicorn app.main:app \
  --app-dir "${APP_DIR}" \
  --host "${STATS_BIND_HOST:-0.0.0.0}" \
  --port "${STATS_BIND_PORT:-8000}" \
  --log-level "${UVICORN_LOG_LEVEL}" \
  "$@"
