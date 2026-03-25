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

## Quick visual check

Open:

- `http://127.0.0.1:8000/ui`

The mini UI polls:

- `dashboard` every 1.0s (1 Hz)
- `history` every 1.0s (1 Hz)

## Test

```bash
.venv/bin/pytest -q
```

## Endpoints V1

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history?window=300&step=1&mode=display`
- `GET /api/v1/meta`

### History params

- `window`: min `1`, max `600`, default `300`
- `step`: min `1`, max `10`, default `1`
- `mode`: `display|raw`, default `display`
- `since_ts_ms`: optional unix ms (`>=0`), returns only points newer than this timestamp

Invalid `window`, `step`, `mode` or `since_ts_ms` returns HTTP `400` with:

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
- `STATS_SAMPLE_INTERVAL_S` default `0.1` (acquisition capteurs, 10 Hz)
- `STATS_PUBLISH_INTERVAL_S` default `0.5` (émission dashboard/history, 2 Hz)
- `STATS_HISTORY_CAPACITY` default `600`
- `STATS_DISPLAY_EMA_ALPHA` default `0.25` (`value_display = EMA(Median3(raw))`)
- `STATS_API_KEY` optional
- `STATS_API_KEY_HEADER` default `X-API-Key`
- `STATS_LOG_LEVEL` default `INFO`
- `STATS_DIAGNOSTICS` default `0`
- `STATS_DIAG_RAW_CAPTURE` default `0`
- `STATS_DIAG_RAW_HZ` default `10.0`
- `STATS_DIAG_RAW_DURATION_S` default `60`
- `STATS_DIAG_RAW_LOG_PATH` default `api/diagnostics/raw_metrics.jsonl`
- `STATS_DIAG_COMPARE_CAPTURE` default `0`
- `STATS_DIAG_COMPARE_HZ` default `10.0`
- `STATS_DIAG_COMPARE_DURATION_S` default `60`
- `STATS_DIAG_COMPARE_LOG_PATH` default `api/diagnostics/raw_vs_display_gpu_pct.jsonl`
- `STATS_GPU_TEMP_LABEL_PRIORITY` default `edge,junction,mem,unknown`

History behavior:

- history points are timestamped in real milliseconds in the backend store;
- `/api/v1/history` series are bucketed by `step` on an absolute time grid;
- `/api/v1/history` exposes `ts_ms` timeline aligned with each Y-series point;
- missing buckets are exposed as `null` (no synthetic values);
- default call (without `since_ts_ms`) returns the full window;
- `since_ts_ms` enables incremental fetch (`ts_ms > since_ts_ms`);
- if `since_ts_ms` is older than the in-memory window, backend falls back to full-window response.

When `STATS_API_KEY` is set, all `/api/v1/*` routes require the configured header.

## Diagnostic capture

Comparaison raw/display (mode par défaut, 60s):

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

Capture brute de toutes les métriques:

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```

Champs clés en mode `compare`:

- `value_raw` (mesure capteur instantanée)
- `value_display` (Median(3) puis EMA pour UI)
- `source`
- `timestamp_unix_ms`
- `valid`
- `read_error`

Champs clés en mode `raw`:

- `metric`
- `raw_value`
- `source`
- `unit`
- `timestamp_mono_s`
- `timestamp_unix_ms`
- `read_ok`
- `read_error`
- `estimated`

## Contract reference

- `api/docs/API_CONTRACT_V1.md`
