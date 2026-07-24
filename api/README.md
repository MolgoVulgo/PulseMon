# PulseMon API

This directory contains the active Linux backend.

Runtime entry point:

```text
api/app/main.py
```

The backend collects Linux telemetry, normalizes values, publishes current snapshots and bounded in-memory histories, serves the HTTP API and local UI, and stores local configuration in SQLite.

## Setup

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Run

```bash
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## Test

```bash
.venv/bin/pytest -q
```

## Active monitoring endpoints

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`
- `GET /api/v1/user/config`
- `PUT /api/v1/user/config`
- `GET /api/v1/db/data`
- `GET /ui`

## Retained FAN endpoints

The following routes remain implemented, but FAN is no longer an active firmware feature:

- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /api/v1/fans/config`
- `PUT /api/v1/fans/config`
- `GET /api/v1/fans/reference`
- `GET /api/v1/db/fans`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`

## Documentation

Canonical documentation is under `../docs/`. In the supplied snapshot, `api/docs` contains the relative target `../docs`.
