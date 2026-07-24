# Troubleshooting

## Backend unavailable

Check:

- process or systemd service state;
- bind host and port;
- route `GET /api/v1/health`;
- permissions on sysfs, hwmon and DRM paths;
- optional `STATS_API_KEY` configuration.

Do not enable `STATS_API_KEY` for the standard current ESP/UI flow: those clients do not send the header yet.

## SQLite configuration is not persistent

Check the resolved path through `/api/v1/db/data` or logs. Set `STATS_CONFIG_DB_PATH` explicitly for a fixed persistent path. Without it, the backend uses `~/.config/pulsemon/config.db` when writable, otherwise `/tmp/pulsemon/config.db`.

## ESP32 shows backend offline

The backend endpoint is compiled in `esp/src/pulsemon_api_config.h`. Check:

- `PULSEMON_API_HOST`;
- `PULSEMON_API_PORT` and `PULSEMON_API_BASE_URL` consistency;
- LAN routing and firewall;
- backend port 8000 unless changed at both ends;
- HTTP timeout.

There is no current portal or NVS setting for the backend address.

## ESP32 Wi-Fi setup

Check:

- NVS namespace `pulsemon_wifi`;
- AP `PulseMon-Setup` when credentials are missing or retries are exhausted;
- `GET /api/wifi/status`;
- visible networks through `GET /api/wifi/scan`.

## Weather unavailable

Check:

- OpenWeather key and city ID presence;
- valid language and GMT offset;
- Wi-Fi and DNS;
- plain-HTTP reachability to `api.openweathermap.org` in the current implementation;
- SD-card icon files if only icons are missing.

OpenWeather HTTP is a known defect, not the intended final security state.

## News unavailable

Check:

- valid SNTP time;
- GNews key presence;
- HTTPS/DNS access to GNews;
- refresh interval and backoff;
- article age and validation rules.

## FAN

FAN is retained but inactive in the firmware. Missing FAN navigation or polling is expected current behavior, not an operational fault.

## Backend diagnostics

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```
