# Linux backend

The backend is a Python 3.11+/FastAPI service.

## Runtime structure

- collectors: `api/app/collectors/`;
- response models: `api/app/models/`;
- orchestration services: `api/app/services/`;
- current snapshots and histories: `api/app/store/`;
- HTTP entry point: `api/app/main.py`;
- local UI: `api/app/ui.py` and `api/app/ui/index.html`.

## Sampling and publication

Defaults from `api/app/config.py`:

- sensor acquisition: `0.1 s`;
- snapshot/history publication: `0.5 s`;
- history capacity: `600` entries;
- display EMA alpha: `0.25`.

Main and GPU snapshots are served from memory. Missing metrics stay present through invalid metric envelopes or nullable history points.

## SQLite configuration

`api/app/store/config_db.py` stores:

- retained `fan_mappings`;
- free-form `user_settings`;
- schema migration metadata.

Path resolution:

1. `STATS_CONFIG_DB_PATH` when set;
2. `~/.config/pulsemon/config.db` when writable;
3. `/tmp/pulsemon/config.db` fallback.

SQLite WAL mode is enabled.

## Optional API key

When `STATS_API_KEY` is set, `/api/v1/*` routes require the configured header, default `X-API-Key`.

Current integration limitation: the ESP32 firmware and backend web UI do not add this header. The standard local deployment therefore leaves `STATS_API_KEY` unset until those clients are updated.

## FAN status

The backend FAN implementation remains functional and tested, but it is retained legacy scope. It must not be used as evidence that FAN is active in the current firmware product flow.
