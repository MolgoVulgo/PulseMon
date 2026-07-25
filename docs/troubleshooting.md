# Troubleshooting

## Backend unavailable

Check:

- process or systemd service state;
- bind host and port;
- route `GET /api/v1/health`;
- permissions on sysfs, hwmon and DRM paths;
- optional `STATS_API_KEY` configuration.

Do not enable `STATS_API_KEY` for the standard current ESP/UI flow: those clients do not send the header yet.

## Backend state after restart

The backend does not use a persistent database. Current snapshots and short histories are rebuilt in memory after restart.

## Backend fails during configuration load

Run the validation preflight with the same environment as the service:

```bash
cd api
python3 -m app.config
```

The error names the invalid variable and its accepted contract. Check `pulsemon-api.conf` and process overrides. Common failures are a port outside `1..65535`, intervals below their minimum, a zero history capacity, a non-finite number, an invalid boolean spelling, an EMA alpha outside `(0, 1]`, an invalid HTTP header token or a malformed PCI BDF.

Configuration values are not echoed in the error message, so API keys are not disclosed.

## ESP32 shows backend offline

The backend host and port are configured in the local portal and stored in NVS namespace `pulsemon_api`. Check:

- `GET /api/config` values `backend_host` and `backend_port`;
- LAN routing, DNS and firewall;
- the backend bind address and effective port;
- the compiled fallback values `PULSEMON_API_DEFAULT_HOST` and `PULSEMON_API_DEFAULT_PORT` if the namespace was cleared;
- `PULSEMON_HTTP_TIMEOUT_MS`.

Saving the portal configuration reloads the target immediately. Clearing PulseMon configuration restores the compiled fallback.

## ESP32 Wi-Fi setup

Check:

- NVS namespace `pulsemon_wifi`;
- AP `PulseMon-Setup` when credentials are missing or retries are exhausted;
- portal address `http://192.168.4.1/` while connected to that AP;
- `GET /api/wifi/status` and `GET /api/wifi/scan` only while the setup AP is active.

The portal and captive DNS stop after a successful station connection. They are intentionally unavailable on the normal station LAN address. If the AP is active but DNS redirection fails, open `http://192.168.4.1/` directly.

With valid working credentials, hold the top-left corner of any active screen for five seconds to open `PulseMon-Setup`. The manual window lasts ten minutes and does not disconnect the station link. If the AP does not appear, verify that the hold is continuous and starts inside the top-left 64 × 64 pixel hotspot.

## Weather unavailable

Check:

- OpenWeather key and city ID presence;
- valid language and GMT offset;
- Wi-Fi and DNS;
- HTTPS/DNS access to `api.openweathermap.org`;
- SD-card icon files if only icons are missing.

OpenWeather TLS validation uses the ESP-IDF certificate bundle; a TLS or certificate error is reported as a failed refresh and the last valid snapshot is retained.

## News unavailable

Check:

- valid SNTP time;
- GNews key presence;
- HTTPS/DNS access to GNews;
- refresh interval and backoff;
- article age and validation rules.

## Legacy generated screen appears in source searches

The generated EEZ output still contains an unreachable legacy FAN screen and compatibility bindings. Active navigation only resolves Main, GPU and Weather. Do not edit generated files or compatibility bindings to remove the screen manually; perform the design change in EEZ Studio and regenerate.

GPU fan telemetry remains available through `/api/v1/gpu/dashboard` when the driver exposes it.

## Backend diagnostics

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```
