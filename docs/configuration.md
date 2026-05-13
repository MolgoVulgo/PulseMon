# Configuration

PulseMon configuration is split between the Linux backend and the ESP32-S3 firmware.

## Backend environment variables

Common backend settings:

| Variable | Purpose |
|---|---|
| `STATS_BIND_HOST` | HTTP bind address, default `0.0.0.0` |
| `STATS_BIND_PORT` | HTTP port, default `8000` |
| `STATS_SAMPLE_INTERVAL_S` | sensor acquisition interval |
| `STATS_PUBLISH_INTERVAL_S` | snapshot/history publication interval |
| `STATS_HISTORY_CAPACITY` | in-memory history capacity |
| `STATS_DISPLAY_EMA_ALPHA` | EMA smoothing factor for displayed percentages |
| `STATS_API_KEY` | optional API key |
| `STATS_API_KEY_HEADER` | API key header, default `X-API-Key` |
| `STATS_LOG_LEVEL` | logging level |
| `STATS_GPU_PCI_SLOT` | optional AMD GPU selection hint |
| `STATS_GPU_TEMP_LABEL_PRIORITY` | ordered GPU temperature label priority |
| `STATS_DIAGNOSTICS` | diagnostics enable flag |
| `STATS_FANS_MAPPING_FILE` | optional legacy fan mapping file |
| `STATS_CONFIG_DB_PATH` | SQLite configuration database path |

Diagnostics capture settings:

| Variable | Purpose |
|---|---|
| `STATS_DIAG_RAW_CAPTURE` | enable raw capture |
| `STATS_DIAG_RAW_HZ` | raw capture frequency |
| `STATS_DIAG_RAW_DURATION_S` | raw capture duration |
| `STATS_DIAG_RAW_LOG_PATH` | raw capture output path |
| `STATS_DIAG_COMPARE_CAPTURE` | enable raw/display comparison |
| `STATS_DIAG_COMPARE_HZ` | comparison capture frequency |
| `STATS_DIAG_COMPARE_DURATION_S` | comparison capture duration |
| `STATS_DIAG_COMPARE_LOG_PATH` | comparison output path |

## Backend local storage

The backend uses SQLite for persistent local configuration such as fan mapping and user-facing UI configuration.

When an empty configuration database is detected, the backend can import a legacy fan mapping file or bootstrap a mapping from detected fan channels.

## Firmware API configuration

Backend access is defined in firmware configuration headers:

- `PULSEMON_API_HOST`;
- `PULSEMON_API_PORT`;
- `PULSEMON_API_BASE_URL`;
- `PULSEMON_HTTP_TIMEOUT_MS`;
- `PULSEMON_DASHBOARD_POLL_MS`.

A static LAN address can be used, but hostname or mDNS-based discovery is also compatible with the architecture.

## Firmware Wi-Fi configuration

Wi-Fi credentials are stored in NVS, not compiled into the firmware.

At boot:

1. the firmware checks for stored station credentials;
2. it attempts Wi-Fi connection;
3. if credentials are missing or connection fails, it opens the `PulseMon-Setup` access point;
4. the local portal is available at `http://192.168.4.1/`;
5. captive DNS redirects common names to the portal address.

## Secret handling

API keys must not be:

- printed on screen;
- written to logs;
- returned by configuration APIs;
- embedded in query strings when a header can be used;
- stored in generated documentation examples.

Configuration APIs should expose only presence flags such as `openweather_key_set` or `gnews_key_set`.
