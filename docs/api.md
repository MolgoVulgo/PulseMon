# HTTP API

The PulseMon backend exposes a local read-oriented HTTP API under `/api/v1`.

The API is optimized for an embedded client: stable keys, compact payloads, fixed units, numeric values, nullable unavailable metrics, and bounded history.

## General rules

- All payloads include `v`.
- Timestamps are Unix timestamps or millisecond timestamps where explicitly named.
- Percentages are numeric values.
- Temperatures are Celsius.
- Power values are watts.
- Memory values are bytes.
- Missing metrics remain present with `null` or with an invalid metric envelope.
- API keys are passed through a header when authentication is enabled.

## Health

`GET /api/v1/health`

Purpose: minimal service availability check.

Typical response:

```json
{
  "v": 1,
  "ts": 1774256402,
  "ok": true,
  "service": "stats-linux-api"
}
```

## Main dashboard

`GET /api/v1/dashboard`

Purpose: current host snapshot for the main screen.

The active implementation uses metric envelopes for displayed telemetry. Each metric envelope can contain:

- `value_raw` — raw collected value;
- `value_display` — smoothed or display-ready value;
- `source` — selected telemetry source;
- `unit` — fixed unit;
- `sampled_at` — sampling timestamp;
- `estimated` — whether the value is estimated;
- `valid` — whether the value is usable.

The firmware gives priority to `value_display`, falls back to `value_raw`, and can tolerate scalar values where compatibility is needed.

## Main history

`GET /api/v1/history?window=300&step=1&mode=display`

Purpose: aligned short history for charts or diagnostics.

Parameters:

- `window`: integer, minimum `1`, maximum `600`, default `300`;
- `step`: integer, minimum `1`, maximum `10`, default `1`;
- `mode`: `display` or `raw`, default `display`;
- `since_ts_ms`: optional millisecond timestamp for delta retrieval.

The response contains aligned series and an explicit `ts_ms` timeline. The embedded client must not realign arrays by itself.

Invalid parameter response:

```json
{
  "v": 1,
  "error": "invalid_parameter",
  "field": "window"
}
```

## Metadata

`GET /api/v1/meta`

Purpose: expose host, available metrics, history series and capability information for diagnostics and validation.

## GPU endpoints

`GET /api/v1/gpu/dashboard`

Purpose: detailed AMD GPU state for the GPU screen.

`GET /api/v1/gpu/history?window=300&step=1&mode=display`

Purpose: bounded GPU history for display or diagnostics.

`GET /api/v1/gpu/meta`

Purpose: GPU source and capability metadata.

## Fan endpoints

`GET /api/v1/fans/dashboard`

Purpose: current fan telemetry and computed fan percentages.

`GET /api/v1/fans/meta`

Purpose: fan telemetry metadata.

`GET /api/v1/fans/config`

Purpose: retrieve the configured fan mapping.

`PUT /api/v1/fans/config`

Purpose: update fan mapping and calibration values.

`GET /api/v1/fans/reference`

Purpose: provide fan reference data used by the admin UI.

## User configuration endpoints

`GET /api/v1/user/config`

Purpose: retrieve user-facing backend UI configuration.

`PUT /api/v1/user/config`

Purpose: update user-facing backend UI configuration.

## Local database admin endpoints

The backend can expose local admin routes for fan mapping management:

- `GET /api/v1/db/data`
- `GET /api/v1/db/fans?include_deleted=1`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`

These routes are local administration tools, not firmware display dependencies.

## Local UI

`GET /ui`

Purpose: local backend debug/admin interface.

## User configuration payload

`GET /api/v1/user/config` returns:

```json
{
  "v": 1,
  "settings": {}
}
```

`PUT /api/v1/user/config` replaces the full user settings object. The `settings` object is free-form JSON and is stored in SQLite by key.

Example:

```json
{
  "settings": {
    "refresh_hz": 1,
    "fans_view_mode": "meta",
    "show_gpu_graph": true
  }
}
```
