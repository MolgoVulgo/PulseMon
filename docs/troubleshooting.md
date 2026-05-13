# Troubleshooting

## Backend does not expose metrics

Check:

- service process is running;
- bind host and port are correct;
- Linux user can read sysfs, hwmon and DRM paths;
- AMD GPU path selection is correct;
- diagnostics mode reports selected sensor paths;
- API key header is present when authentication is enabled.

## GPU values are missing

Check:

- GPU is visible under `/sys/class/drm/card*/device`;
- amdgpu driver exposes `gpu_busy_percent` or equivalent telemetry;
- hwmon entries expose temperature labels;
- `STATS_GPU_PCI_SLOT` is set when automatic selection picks the wrong card;
- `STATS_GPU_TEMP_LABEL_PRIORITY` matches exposed labels.

## GPU chart is too noisy

Use display values for UI and raw values for diagnostics. Verify `STATS_DISPLAY_EMA_ALPHA` and compare raw/display capture outputs.

## Fan percentage is null

Check:

- fan channel is detected;
- fan mapping exists in SQLite configuration;
- `rpm_min` and `rpm_max` are valid;
- the runtime value is inside a plausible range;
- no legacy import failed silently.

## ESP32 cannot connect to Wi-Fi

Check:

- stored NVS credentials;
- configuration portal availability on `PulseMon-Setup`;
- captive portal address `http://192.168.4.1/`;
- retry limit constants;
- signal strength and SSID visibility.

## ESP32 shows backend offline

Check:

- backend is reachable from the same LAN;
- configured host, port and base URL;
- firewall rules;
- optional API key header;
- HTTP timeout;
- backend route `/api/v1/health`.

## Weather or news does not appear

Check:

- Wi-Fi is connected;
- SNTP time is valid;
- OpenWeather or GNews key presence flag is true;
- keys are stored in NVS;
- quota or authorization errors are not active;
- weather alert priority is not masking news;
- news backoff has elapsed;
- SD card icon files exist if icon display is affected.

## Useful diagnostics

Backend raw/display GPU comparison:

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

Backend raw multi-metric capture:

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```
