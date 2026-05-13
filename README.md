# PulseMon

PulseMon is a local monitoring system for a Linux workstation and an ESP32-S3 display.

It collects Linux metrics on the host, exposes them through a local HTTP API, and displays the current state on a dedicated ESP32-S3 screen built with LVGL. The project is designed for a private LAN, with no cloud dependency and no external broker.

## What the project does

PulseMon provides:

- a Python/FastAPI backend for Linux system telemetry;
- CPU, memory, AMD GPU and fan monitoring;
- a local HTTP API consumed by the ESP32-S3 firmware;
- short in-memory history for charts;
- a local debug/admin web UI exposed by the backend;
- an ESP32-S3 firmware with Wi-Fi setup, API polling, LVGL rendering, weather display and autonomous news headlines;
- local configuration storage for backend and firmware settings.

## What it is for

PulseMon is intended to provide a small always-on hardware dashboard for a Linux workstation. It gives immediate visibility on CPU, RAM, GPU, fan and runtime state without opening a desktop dashboard or relying on remote services.

The ESP32-S3 can also display useful autonomous information when the Linux host is unavailable, such as weather data and news headlines, provided the required API keys are configured on the device.

## Repository layout

```text
PulseMon/
├── api/                  # Linux backend entry point and backend-specific files
├── esp/                  # ESP32-S3 firmware entry point and firmware-specific files
├── docs/                 # canonical English documentation
│   └── fr/               # French documentation
├── README.md             # default English README
└── README.fr.md          # French README
```

The canonical documentation is stored in `/docs`. The `api/docs` and `esp/docs` paths are symbolic links to `/docs`.

## Prerequisites

Backend:

- Linux host;
- Python 3.11 or newer;
- FastAPI;
- uvicorn;
- psutil;
- Pydantic;
- pytest and httpx for tests;
- access to Linux sysfs, hwmon and DRM paths for AMD CPU/GPU telemetry.

Firmware:

- ESP32-S3 board with display;
- PlatformIO with ESP-IDF support;
- LVGL;
- Wi-Fi network access;
- optional SD card for weather icon assets;
- optional OpenWeather and GNews API keys for autonomous weather/news features.

## Install the backend

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

The backend exposes the API under `/api/v1/*` and the local debug/admin UI under `/ui`.

## Build the firmware

```bash
cd esp
pio run -e LVGL-320-480
```

Wi-Fi credentials are not compiled into the firmware. They are stored in NVS through the ESP32-S3 configuration portal.

## Configure the project

Backend configuration is handled through environment variables. The main settings are host, port, sampling cadence, history capacity, optional API key, GPU detection hints, diagnostics and local SQLite configuration storage.

ESP32-S3 configuration is handled through NVS and the local configuration portal. It stores Wi-Fi credentials, backend API address, OpenWeather settings, GNews settings and display-related options. API keys must not be printed, logged or returned by configuration endpoints.

Detailed configuration is documented in `/docs/configuration.md`.

## Contribute

Contributions should preserve these rules:

- keep the backend, API contract, firmware polling and LVGL rendering separated;
- keep JSON payloads stable and compact;
- keep unavailable metrics nullable instead of removing fields;
- do not edit generated ESP32 UI files directly;
- keep secrets out of logs, URLs, screens and exported configuration;
- add tests for API contract changes;
- update `/docs` and `/docs/fr` when behavior changes.

## Detailed documentation

- `/docs/README.md` — documentation index;
- `/docs/overview.md` — project overview;
- `/docs/architecture.md` — backend/firmware architecture;
- `/docs/api.md` — HTTP API contract;
- `/docs/backend.md` — Linux backend behavior;
- `/docs/firmware.md` — ESP32-S3 firmware behavior;
- `/docs/configuration.md` — backend and device configuration;
- `/docs/gpu.md` — AMD GPU monitoring;
- `/docs/fans.md` — fan monitoring and configuration;
- `/docs/weather-news.md` — weather and autonomous news modules;
- `/docs/web-configuration.md` — ESP32 configuration portal;
- `/docs/development.md` — build, tests and contribution workflow;
- `/docs/troubleshooting.md` — diagnostics and operational notes;
- `/docs/fr/` — French documentation.
