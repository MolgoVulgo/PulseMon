# HTTP API

The backend exposes versioned JSON routes under `/api/v1` and a local HTML UI under `/ui`.

## Contract rules

- response models use strict Pydantic fields;
- versioned payloads use `v: 1`;
- unavailable telemetry remains present with `valid: false`, `null` values or `null` history points;
- metric envelopes contain `value_raw`, `value_display`, `source`, `unit`, `sampled_at`, `estimated` and `valid`;
- history arrays remain aligned with `ts_ms`;
- authentication, when enabled, uses `STATS_API_KEY_HEADER`, default `X-API-Key`.

## Active monitoring routes

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

Main dashboard metrics:

```text
cpu.pct, cpu.temp_c, cpu.power_w
mem.used_b, mem.total_b, mem.pct
gpu.pct, gpu.temp_c, gpu.power_w
state.ok, state.stale_ms
```

Main history series:

```text
cpu_pct, cpu_temp_c, gpu_pct, gpu_temp_c
```

GPU dashboard metrics:

```text
gpu.pct, gpu.core_clock_mhz, gpu.mem_clock_mhz
gpu.vram_used_b, gpu.vram_total_b, gpu.vram_pct
gpu.temp_c, gpu.power_w, gpu.fan_rpm, gpu.fan_pct
```

GPU history series:

```text
gpu_pct, gpu_core_clock_mhz, gpu_vram_used_b, gpu_temp_c,
gpu_power_w, gpu_mem_clock_mhz, gpu_fan_rpm
```

## User and database routes

- `GET /api/v1/user/config`
- `PUT /api/v1/user/config`
- `GET /api/v1/db/data`

`user/config` stores a free-form JSON object in SQLite.

## Retained FAN routes

These routes remain implemented but are not consumed by the active firmware flow:

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

## Client authentication limitation

If `STATS_API_KEY` is enabled, all `/api/v1/*` requests require the configured header. The current firmware and backend UI do not send it; enabling the key therefore requires client changes.
