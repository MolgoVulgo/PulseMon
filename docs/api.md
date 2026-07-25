# HTTP API

The backend exposes versioned JSON routes under `/api/v1` and a local HTML UI under `/ui`.

## Contract rules

- response models use strict Pydantic fields;
- versioned payloads use `v: 1`;
- unavailable telemetry remains present with `valid: false`, `null` values or `null` history points;
- metric envelopes contain `value_raw`, `value_display`, `source`, `unit`, `sampled_at`, `estimated` and `valid`;
- history arrays remain aligned with `ts_ms`;
- invalid query parameters produce an explicit API error;
- authentication, when enabled, uses `STATS_API_KEY_HEADER`, default `X-API-Key`.

## Active routes

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

The local HTML UI is exposed through `GET /ui` and is not part of the versioned JSON contract.

## Main dashboard

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

## GPU dashboard

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

GPU fan fields are GPU telemetry. They do not imply a generic FAN mapping or management API.

## Query parameters

`/api/v1/history`:

- `window`: 1 to 600 seconds, default 300;
- `step`: 1 to 10 seconds, default 1;
- `mode`: `display` or `raw`;
- `since_ts_ms`: optional non-negative timestamp.

`/api/v1/gpu/history` accepts `window`, `step` and `mode` with the same bounds.

## State and persistence

No user-configuration or database routes are exposed. Backend state is limited to current snapshots and bounded histories in memory.

## Client authentication limitation

If `STATS_API_KEY` is enabled, all `/api/v1/*` requests require the configured header. The current firmware and backend UI do not send it; enabling the key requires matching client changes.
