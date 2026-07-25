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

Main and GPU samplers publish to separate snapshot and history stores. HTTP handlers read those stores; they do not perform the normal sensor acquisition path themselves.

Missing metrics remain present through invalid metric envelopes or nullable history points.

## Runtime storage

The backend has no persistent database. Current snapshots and bounded histories are stored in memory and are rebuilt after restart. Diagnostic captures, when explicitly enabled, are written to configured JSONL paths and are not part of the API state model.

## Local UI

`/ui` displays current main and GPU metrics plus main history. It is a local debug/observation surface, not an administration interface.

## Optional API key

When `STATS_API_KEY` is set, `/api/v1/*` routes require the configured header, default `X-API-Key`.

Current integration limitation: the ESP32 firmware and backend web UI do not add this header. The standard local deployment therefore leaves `STATS_API_KEY` unset until those clients are updated.

## Strict configuration validation

All 20 backend `STATS_*` variables are parsed centrally by `api/app/config.py`. Startup rejects malformed values, non-finite numbers, out-of-range intervals and capacities, invalid booleans, invalid HTTP header names, invalid PCI BDF values and invalid GPU temperature-label lists.

The packaged launcher executes `python -m app.config` before Uvicorn, and importing `app.main` validates the same contract before runtime services are constructed. Error messages identify the variable and expected contract without echoing secret values.
