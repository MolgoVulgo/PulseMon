# Configuration

## Backend environment

The backend recognizes exactly 20 `STATS_*` variables. `api/app/config.py` parses and validates all of them before stores, samplers or diagnostics are created. The packaged launcher runs the same validation preflight before Uvicorn starts.

| Variable | Default | Accepted contract | Purpose |
|---|---:|---|---|
| `STATS_BIND_HOST` | `0.0.0.0` | IPv4, IPv6 or valid hostname, no whitespace | bind address |
| `STATS_BIND_PORT` | `8000` | integer `1..65535` | HTTP port |
| `STATS_SAMPLE_INTERVAL_S` | `0.1` | finite number `0.01..60.0` | sensor acquisition interval |
| `STATS_PUBLISH_INTERVAL_S` | `0.5` | finite number `0.01..3600.0` | snapshot/history publication interval |
| `STATS_HISTORY_CAPACITY` | `600` | integer `1..1000000` | in-memory history capacity |
| `STATS_API_KEY` | unset | empty disables; otherwise trimmed, control-character-free string | optional API key for `/api/v1/*` |
| `STATS_API_KEY_HEADER` | `X-API-Key` | valid HTTP token, maximum 128 characters | authentication header name |
| `STATS_LOG_LEVEL` | `INFO` | `CRITICAL`, `ERROR`, `WARNING`, `INFO` or `DEBUG`; `WARN` and `FATAL` are normalized | application and Uvicorn log level |
| `STATS_DIAGNOSTICS` | `0` | `1/0`, `true/false`, `yes/no` or `on/off` | diagnostics logging flag |
| `STATS_DIAG_RAW_CAPTURE` | `0` | strict boolean | raw capture flag |
| `STATS_DIAG_RAW_HZ` | `8.0` | finite number `0.1..1000.0` | raw capture frequency |
| `STATS_DIAG_RAW_DURATION_S` | `60` | integer `1..86400` | raw capture duration |
| `STATS_DIAG_RAW_LOG_PATH` | `api/diagnostics/raw_metrics.jsonl` | non-empty path without control characters | raw capture path |
| `STATS_DIAG_COMPARE_CAPTURE` | `0` | strict boolean | raw/display comparison flag |
| `STATS_DIAG_COMPARE_HZ` | `10.0` | finite number `0.1..1000.0` | comparison frequency |
| `STATS_DIAG_COMPARE_DURATION_S` | `60` | integer `1..86400` | comparison duration |
| `STATS_DIAG_COMPARE_LOG_PATH` | `api/diagnostics/raw_vs_display_gpu_pct.jsonl` | non-empty path without control characters | comparison path |
| `STATS_DISPLAY_EMA_ALPHA` | `0.25` | finite number `>0.0` and `<=1.0` | display EMA alpha |
| `STATS_GPU_PCI_SLOT` | unset | PCI BDF `dddd:bb:ss.f` or `bb:ss.f` | force an AMD PCI slot |
| `STATS_GPU_TEMP_LABEL_PRIORITY` | `edge,junction,mem,unknown` | 1 to 32 unique comma-separated labels matching `[a-z0-9_-]`; `unknown` is appended when absent | ordered GPU temperature labels |

Invalid configuration raises a `ConfigError` and stops startup with the variable name and expected contract. Values are not echoed, so API keys are not disclosed in error output. Non-finite numeric values such as `nan` and `inf` are rejected.

Validate the current environment without starting the service:

```bash
cd api
python3 -m app.config
```

The GPU collector no longer reads its two variables directly. `load_config()` validates and normalizes them, then `api/app/main.py` applies the resulting GPU selection once during startup.

The backend has no persistent configuration database. Relative diagnostic paths are resolved from the process working directory.

## Firmware backend endpoint

The backend endpoint is split between NVS and compiled fallback values.

NVS runtime settings:

```text
namespace: pulsemon_api
keys: host, port
```

Accepted host values are IPv4 addresses or DNS/mDNS hostnames using letters, digits, dots and internal hyphens. Schemes, paths, whitespace, underscores and raw IPv6 literals are rejected. The port range is `1..65535`.

Compiled settings in `esp/src/pulsemon_api_config.h`:

```text
PULSEMON_API_DEFAULT_HOST
PULSEMON_API_DEFAULT_PORT
PULSEMON_HTTP_TIMEOUT_MS
PULSEMON_DASHBOARD_POLL_MS
```

The NVS host/port override the compiled defaults. Saving the local portal reloads the active client endpoint without reboot. Clearing PulseMon configuration erases the `pulsemon_api` namespace and restores the compiled fallback. The client caches the active endpoint in RAM and does not read NVS on every poll. Holding the top-left corner of Main, GPU or Weather for five seconds opens a manual setup-AP window for five minutes. The trigger does not modify NVS or disconnect an existing station connection.

The firmware does not currently store or send `STATS_API_KEY_HEADER` credentials.

## Printer configuration

Printer connectivity is runtime configuration stored in NVS:

```text
namespace: printer
keys: host, access_code
```

`host` is the printer LAN IPv4 address or DNS/mDNS hostname. `access_code` is used as the HTTP bootstrap token and direct MQTT password. Both values must be present for the Printer service to connect. The access code is a secret: the local configuration portal accepts replacement values but `GET /api/config` returns only `printer_access_code_set`, never the secret itself.

`esp/src/printer_config.h` contains only fixed protocol settings: HTTP port `80`, MQTT port `1883`, HTTP timeout `4000 ms`, MQTT timeout `5000 ms`, status polling `5000 ms`, reconnect retry `5000 ms` and application PING interval `30000 ms`. It contains no printer host or access code. Saving or clearing Printer settings through the portal wakes an active Printer service so it reloads NVS and reconnects without reboot.

## Firmware NVS namespaces

Backend endpoint:

```text
namespace: pulsemon_api
keys: host, port
```

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

Printer settings:

```text
namespace: printer
keys: host, access_code
```

Default weather settings are GMT offset `+60 minutes`, language `fr`, no city and no key. Default news settings are enabled GNews, refresh `30 minutes`, category `general`, language/country `fr`, maximum `5` items, slide speed `35` and maximum age `15 days`.

## Wi-Fi behavior

- With saved credentials, firmware starts in station mode and connects.
- Without valid credentials, firmware starts AP+station mode and exposes the open `PulseMon-Setup` AP.
- After repeated station failures, the configuration AP is enabled.
- A continuous five-second hold in the top-left corner of Main, GPU or Weather also opens the AP for five minutes.
- After a successful station connection, the automatic AP is disabled; an active manual window remains open until its timeout.
- The HTTP configuration server starts only after `WIFI_EVENT_AP_START` and stops after `WIFI_EVENT_AP_STOP`.
- Captive DNS follows the same AP lifecycle and binds only to `192.168.4.1`, never `INADDR_ANY`.
- Portal handlers return `503 Service Unavailable` if the AP is no longer active.
