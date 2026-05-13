# PulseMon API

This directory contains the Linux backend entry point and backend-specific files.

The backend is responsible for collecting Linux telemetry, normalizing sensor values, maintaining the current snapshot and short history, exposing the HTTP API, serving the local debug/admin UI, and storing local configuration when needed.

The canonical backend documentation is available in:

- `/docs/api.md` for the HTTP contract;
- `/docs/backend.md` for runtime behavior;
- `/docs/configuration.md` for environment variables and storage;
- `/docs/gpu.md` for AMD GPU telemetry;
- `/docs/fans.md` for fan monitoring and configuration;
- `/docs/development.md` for tests and contribution rules.

The `api/docs` path is a symbolic link to `/docs`.

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

## Main endpoints

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`
- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /api/v1/fans/config`
- `PUT /api/v1/fans/config`
- `GET /api/v1/fans/reference`
- `GET /api/v1/user/config`
- `PUT /api/v1/user/config`
- `GET /ui`
