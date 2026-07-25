# PulseMon API

This directory contains the active Linux backend.

Runtime entry point:

```text
api/app/main.py
```

The backend collects CPU, memory and AMD GPU telemetry, normalizes values, publishes current snapshots and bounded in-memory histories, and serves the HTTP API plus a local debug UI. It does not use a persistent database.

## Setup

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Run

```bash
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## Configuration validation

All 20 supported `STATS_*` variables are validated before runtime services start. Validate the current environment without starting Uvicorn:

```bash
.venv/bin/python -m app.config
```

Invalid values stop startup with the variable name and expected contract. Secret values are not echoed. The packaged launcher performs this preflight automatically.

## Test

```bash
.venv/bin/pytest -q
```

## Active routes

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`
- `GET /ui`

GPU fan RPM and percentage are GPU telemetry fields exposed by the GPU dashboard. There is no generic FAN mapping API, database API or user-configuration API.

If `STATS_API_KEY` is enabled, all `/api/v1/*` routes require the configured header. The current ESP32 firmware and backend UI do not send that header.

## Documentation

Canonical documentation is under `../docs/`. In the supplied snapshot, `api/docs` contains the relative target `../docs`.
