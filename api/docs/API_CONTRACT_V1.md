# API Contract V1

Contract prefix: `/api/v1/`

GPU extension reference: `api/docs/API_GPU_CONTRACT_V1.md`

## Guarantees

- JSON contract version is always `"v": 1`.
- Field names are fixed.
- Units are fixed.
- Missing metrics are returned as `null` and never removed.
- History series are flat arrays with aligned lengths.
- History series are generated from real sample timestamps (ms) and bucketed by `step`.
- Each metric payload exposes raw/display/source metadata.
- Acquisition and publication are decoupled:
  - sensor acquisition target: 10 Hz
  - API publication target: 2 Hz

## Metric Envelope

Every dashboard metric uses this shape:

```json
{
  "value_raw": 87.0,
  "value_display": 91.6,
  "source": "/sys/class/drm/card1/device/gpu_busy_percent",
  "unit": "percent",
  "sampled_at": 1774256402000,
  "estimated": false,
  "valid": true
}
```

Fields:

- `value_raw`: direct instantaneous sensor value, or `null`
- `value_display`: UI value derived from `value_raw` (`Median(3)` then `EMA(alpha)` for `%` usage metrics), or `null`
- `source`: selected source path/signature
- `unit`: fixed unit (`percent`, `celsius`, `watt`, `bytes`)
- `sampled_at`: unix timestamp in ms
- `estimated`: `true` only for computed estimate (unused in current backend)
- `valid`: `true` only when a real value was read

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
    "pct": {
      "value_raw": 86.0,
      "value_display": 89.2,
      "source": "psutil.cpu_percent(interval=None)",
      "unit": "percent",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    },
    "temp_c": {
      "value_raw": 67.0,
      "value_display": 66.6,
      "source": "psutil.sensors_temperatures(fahrenheit=False):Tctl",
      "unit": "celsius",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    },
    "power_w": {
      "value_raw": null,
      "value_display": null,
      "source": "/sys/class/powercap/*/energy_uj",
      "unit": "watt",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": false
    }
  },
  "mem": {
    "used_b": {
      "value_raw": 9123454976,
      "value_display": 9123454976,
      "source": "psutil.virtual_memory()",
      "unit": "bytes",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    },
    "total_b": {
      "value_raw": 34359738368,
      "value_display": 34359738368,
      "source": "psutil.virtual_memory()",
      "unit": "bytes",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    },
    "pct": {
      "value_raw": 26.6,
      "value_display": 26.6,
      "source": "psutil.virtual_memory()",
      "unit": "percent",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    }
  },
  "gpu": {
    "pct": {
      "value_raw": 83.0,
      "value_display": 89.4,
      "source": "/sys/class/drm/card1/device/gpu_busy_percent",
      "unit": "percent",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    },
    "temp_c": {
      "value_raw": 64.0,
      "value_display": 64.2,
      "source": "/sys/class/hwmon/hwmon3/temp1_input:edge",
      "unit": "celsius",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    },
    "power_w": {
      "value_raw": 243.0,
      "value_display": 240.1,
      "source": "/sys/class/hwmon/hwmon3/power1_average",
      "unit": "watt",
      "sampled_at": 1774256402000,
      "estimated": false,
      "valid": true
    }
  },
  "state": {
    "ok": true,
    "stale_ms": 0
  }
}
```

### `GET /api/v1/history?window=300&step=1&mode=display`

Parameter bounds:

- `window`: `1..600`
- `step`: `1..10`
- `mode`: `display|raw` (default `display`)
- `since_ts_ms`: optional unix timestamp in milliseconds (`>=0`)

History rules:

- backend stores each sample with a real timestamp in milliseconds;
- response series are bucketed by `step` on an absolute time grid (`floor(ts_ms / step_ms)`);
- `ts_ms` carries the explicit X axis timeline (one timestamp per point);
- missing buckets are returned as `null` (no synthetic interpolation);
- `ts` corresponds to the latest available history point timestamp (seconds).
- default behavior (without `since_ts_ms`) returns the full window series;
- with `since_ts_ms`, response returns only points with `ts_ms > since_ts_ms`;
- if `since_ts_ms` is older than the retained in-memory timeline, backend falls back to full-window output (client resync path);
- `mode=display` uses filtered values (`value_display`), `mode=raw` uses raw values (`value_raw`).

Response:

```json
{
  "v": 1,
  "ts": 1774256402,
  "ts_ms": [1774256399000, 1774256400000, 1774256401000, 1774256402000],
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
    "cpu.power_w",
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
