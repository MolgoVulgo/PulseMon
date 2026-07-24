# PulseMon

PulseMon is a local monitoring system composed of a Linux backend and an ESP32-S3 display.

The Linux host collects system metrics and exposes them through a FastAPI HTTP API. The ESP32-S3 polls that API, keeps the last valid values in a local cache and renders the active screens with LVGL. The target deployment is a personal workstation on a private LAN, without MQTT, a mandatory cloud service or an external broker.

## Active scope

The current runtime provides:

- CPU usage, temperature and optional power telemetry;
- memory usage and capacity telemetry;
- AMD GPU usage, clocks, VRAM, temperature, power and GPU fan telemetry when available;
- current snapshots and bounded in-memory history;
- a local backend debug/admin UI under `/ui`;
- active ESP32-S3 screens for Main, GPU and Weather;
- autonomous OpenWeather weather data and GNews headlines on the ESP32-S3;
- local configuration storage in SQLite on the backend and NVS on the ESP32-S3.

## Retained FAN subsystem

The backend still contains fan collectors, mapping storage, API endpoints and admin functions. Firmware types, generated UI elements and dormant runtime helpers also remain in the tree.

This subsystem is no longer an active product feature:

- the FAN screen is not reachable from the active firmware navigation;
- the firmware poller does not request the fan dashboard;
- the code is retained for now and must not be treated as an active implementation target unless explicitly reactivated.

## Repository layout

```text
PulseMon/
├── api/                         # Linux backend
├── esp/                         # ESP32-S3 firmware
│   ├── src/ui/                  # EEZ-generated UI compiled by the firmware
│   └── eez/pulsmon/             # EEZ Studio project and saved application state
├── docs/                        # canonical English documentation
│   └── fr/                      # French documentation
├── make-a.sh                    # creates the intentionally filtered PulseMon.zip snapshot
├── README.md
└── README.fr.md
```

`docs/` is the canonical documentation root. In the supplied snapshot, `api/docs` and `esp/docs` contain the relative target `../docs`; they represent the intended link to the canonical documentation.

## Backend setup

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Run the backend

```bash
cd api
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

The API is exposed under `/api/v1/*` and the local UI under `/ui`.

## Backend tests

```bash
cd api
.venv/bin/pytest -q
```

## Firmware build

Main environment:

```bash
cd esp
pio run -e pulsmon-esp32s3-display
```

Development environment with PulseMon debug flags:

```bash
cd esp
pio run -e pulsmon-esp32s3-display-dev
```

The default PlatformIO environment is `pulsmon-esp32s3-display`.

## Current firmware configuration model

Wi-Fi credentials, OpenWeather settings and GNews settings are stored in NVS through the local configuration portal.

The backend endpoint is not stored in NVS in the current implementation. It is compiled from `esp/src/pulsemon_api_config.h` through:

- `PULSEMON_API_HOST`;
- `PULSEMON_API_PORT`;
- `PULSEMON_API_BASE_URL`;
- `PULSEMON_HTTP_TIMEOUT_MS`;
- `PULSEMON_DASHBOARD_POLL_MS`.

The firmware does not currently send the optional backend API-key header.

## Security posture

PulseMon targets a local and personal deployment. Security remains proportionate to that context: secrets must not be logged or returned by configuration endpoints, input must remain validated, and unnecessary LAN exposure should be avoided.

Known defect: the OpenWeather client currently uses plain HTTP. GNews already uses HTTPS and the `X-Api-Key` header.

## Snapshot generation

`make-a.sh` creates the `PulseMon.zip` snapshot used for review and patch work. It deliberately excludes local build trees, virtual environments, caches, `tmp/`, local SDK configuration and other machine-specific content. The archive is therefore a controlled project snapshot, not an exhaustive copy of the working directory.

## Documentation

- `docs/README.md` — documentation index;
- `docs/overview.md` — current scope and runtime model;
- `docs/architecture.md` — ownership and data flows;
- `docs/api.md` — backend HTTP contract;
- `docs/backend.md` — backend behavior;
- `docs/firmware.md` — firmware behavior and active screens;
- `docs/configuration.md` — environment, SQLite and NVS configuration;
- `docs/fans.md` — retained FAN subsystem status;
- `docs/weather-news.md` — weather and news implementation;
- `docs/web-configuration.md` — ESP32 local portal;
- `docs/development.md` — build, tests and snapshot workflow;
- `docs/troubleshooting.md` — operational checks;
- `docs/fr/` — French mirror.
