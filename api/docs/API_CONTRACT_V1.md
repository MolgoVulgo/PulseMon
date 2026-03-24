# API Contract V1

Contract prefix: `/api/v1/`

## Guarantees

- JSON contract version is always `"v": 1`.
- Field names are fixed.
- Units are fixed.
- Missing metrics are returned as `null` and never removed.
- History series are flat arrays with aligned lengths.

## Endpoints

### `GET /api/v1/health`

Returns service availability:

```json
{
  "v": 1,
  "ts": 1774256402,
  "ok": true,
  "service": "stats-linux-api"
}
```

### `GET /api/v1/dashboard`

Returns current snapshot:

```json
{
  "v": 1,
  "ts": 1774256402,
  "host": "linux-main",
  "cpu": {
    "pct": 12.4,
    "temp_c": 43.8
  },
  "mem": {
    "used_b": 9123454976,
    "total_b": 34359738368,
    "pct": 26.6
  },
  "gpu": {
    "pct": 7.0,
    "temp_c": 39.0,
    "power_w": 36.4
  },
  "state": {
    "ok": true,
    "stale_ms": 0
  }
}
```

### `GET /api/v1/history?window=300&step=1`

Parameter bounds:

- `window`: `10..600`
- `step`: `1..10`

Response:

```json
{
  "v": 1,
  "ts": 1774256402,
  "window_s": 300,
  "step_s": 1,
  "series": {
    "cpu_pct": [8.0, 9.4, 11.2, 10.1],
    "cpu_temp_c": [41.0, 41.1, 41.3, 41.4],
    "gpu_pct": [2.0, 4.0, 7.0, 3.0],
    "gpu_temp_c": [38.0, 38.0, 39.0, 39.0]
  }
}
```

### `GET /api/v1/meta`

Response:

```json
{
  "v": 1,
  "host": "linux-main",
  "metrics": [
    "cpu.pct",
    "cpu.temp_c",
    "mem.used_b",
    "mem.total_b",
    "mem.pct",
    "gpu.pct",
    "gpu.temp_c",
    "gpu.power_w"
  ],
  "history_series": [
    "cpu_pct",
    "cpu_temp_c",
    "gpu_pct",
    "gpu_temp_c"
  ]
}
```

## Error payloads

Invalid parameter:

```json
{
  "v": 1,
  "error": "invalid_parameter",
  "field": "window"
}
```

Snapshot unavailable:

```json
{
  "v": 1,
  "error": "snapshot_unavailable",
  "field": null
}
```

Unauthorized (when API key is enabled):

```json
{
  "v": 1,
  "error": "unauthorized",
  "field": "x-api-key"
}
```
