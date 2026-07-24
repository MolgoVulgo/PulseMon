# Configuration

## Backend environment

Variables read by `api/app/config.py`:

| Variable | Default | Purpose |
|---|---:|---|
| `STATS_BIND_HOST` | `0.0.0.0` | bind address |
| `STATS_BIND_PORT` | `8000` | HTTP port |
| `STATS_SAMPLE_INTERVAL_S` | `0.1` | sensor acquisition interval |
| `STATS_PUBLISH_INTERVAL_S` | `0.5` | snapshot/history publication interval |
| `STATS_HISTORY_CAPACITY` | `600` | in-memory history capacity |
| `STATS_API_KEY` | unset | optional API key for `/api/v1/*` |
| `STATS_API_KEY_HEADER` | `X-API-Key` | authentication header name |
| `STATS_LOG_LEVEL` | `INFO` | application log level |
| `STATS_DIAGNOSTICS` | `0` | diagnostics logging flag |
| `STATS_DIAG_RAW_CAPTURE` | `0` | raw capture flag |
| `STATS_DIAG_RAW_HZ` | `8.0` | raw capture frequency |
| `STATS_DIAG_RAW_DURATION_S` | `60` | raw capture duration |
| `STATS_DIAG_RAW_LOG_PATH` | `api/diagnostics/raw_metrics.jsonl` | raw capture path |
| `STATS_DIAG_COMPARE_CAPTURE` | `0` | raw/display comparison flag |
| `STATS_DIAG_COMPARE_HZ` | `10.0` | comparison frequency |
| `STATS_DIAG_COMPARE_DURATION_S` | `60` | comparison duration |
| `STATS_DIAG_COMPARE_LOG_PATH` | `api/diagnostics/raw_vs_display_gpu_pct.jsonl` | comparison path |
| `STATS_DISPLAY_EMA_ALPHA` | `0.25` | display EMA alpha |

Variables read directly by collectors or services:

| Variable | Default/behavior | Purpose |
|---|---|---|
| `STATS_GPU_PCI_SLOT` | automatic selection | force an AMD PCI slot |
| `STATS_GPU_TEMP_LABEL_PRIORITY` | `edge,junction,mem,unknown` | ordered GPU temperature labels |
| `STATS_CONFIG_DB_PATH` | path resolution below | SQLite database path |
| `STATS_FANS_MAPPING_FILE` | service fallback chain | legacy fan mapping import path |
| `STATS_FANS_REFERENCE_SEED_FILE` | repository `tmp/fan_reference_seed.json` unless overridden | retained fan reference catalog path |

## SQLite path

Resolution order:

1. `STATS_CONFIG_DB_PATH`;
2. `~/.config/pulsemon/config.db` when writable;
3. `/tmp/pulsemon/config.db`.

The supplied `pulsemon-api.conf` does not currently set `STATS_CONFIG_DB_PATH`; deployments requiring a fixed persistent path must define it explicitly.

## Firmware backend endpoint

The backend endpoint is compiled in `esp/src/pulsemon_api_config.h`:

```text
PULSEMON_API_HOST
PULSEMON_API_PORT
PULSEMON_API_BASE_URL
PULSEMON_HTTP_TIMEOUT_MS
PULSEMON_DASHBOARD_POLL_MS
```

The current values include a static LAN address and HTTP port. They are not stored in NVS and are not editable in the local portal.

The firmware does not currently send `STATS_API_KEY_HEADER`.

## Firmware NVS namespaces

Wi-Fi credentials:

```text
namespace: pulsemon_wifi
keys: ssid, password
```

Weather settings:

```text
namespace: pulsemon_cfg
keys: ow_key, gmt_min, ow_city, lang
```

News settings:

```text
namespace: news
keys: provider, gnews_key, enabled, refresh_min, category, lang,
      country, max_items, slide_speed, max_age_days, last_ok_ts, last_error
```

Default weather settings are GMT offset `+60 minutes`, language `fr`, no city and no key. Default news settings are enabled GNews, refresh `30 minutes`, category `general`, language/country `fr`, maximum `5` items, slide speed `35` and maximum age `15 days`.

## Wi-Fi behavior

- With saved credentials, firmware starts in station mode and connects.
- Without valid credentials, firmware starts AP+station mode and exposes the open `PulseMon-Setup` AP.
- After repeated station failures, the configuration AP is enabled.
- After a successful station connection, the AP is disabled.
- The HTTP configuration server and captive DNS tasks are started during normal firmware startup.
