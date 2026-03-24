# Stats Linux API (Bootstrap)

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

## Endpoints V1

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history?window=300&step=1`
- `GET /api/v1/meta`

### History params

- `window`: min `10`, max `600`, default `300`
- `step`: min `1`, max `10`, default `1`

Invalid `window` or `step` returns HTTP `400` with:

```json
{
  "v": 1,
  "error": "invalid_parameter",
  "field": "window"
}
```

## Runtime config (env)

- `STATS_BIND_HOST` default `0.0.0.0`
- `STATS_BIND_PORT` default `8000`
- `STATS_SAMPLE_INTERVAL_S` default `1.0`
- `STATS_HISTORY_CAPACITY` default `600`
- `STATS_API_KEY` optional
- `STATS_API_KEY_HEADER` default `X-API-Key`
- `STATS_LOG_LEVEL` default `INFO`
- `STATS_DIAGNOSTICS` default `0`

When `STATS_API_KEY` is set, all `/api/v1/*` routes require the configured header.

## Contract reference

- `api/docs/API_CONTRACT_V1.md`
