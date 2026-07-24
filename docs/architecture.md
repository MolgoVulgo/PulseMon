# Architecture

PulseMon has two active runtimes.

## Linux backend

Entry point: `api/app/main.py`.

Responsibilities:

- collect CPU, memory, AMD GPU and retained fan telemetry;
- normalize values and preserve explicit invalid states;
- publish current main and GPU snapshots;
- maintain bounded in-memory histories;
- expose the HTTP API and local UI;
- persist user settings and retained fan mappings in SQLite.

HTTP handlers consume services and stores. They do not perform the normal high-frequency sampling loop themselves.

## ESP32-S3 firmware

Entry point: `esp/src/main.c`.

Responsibilities:

- manage Wi-Fi and the local configuration portal;
- poll backend dashboard endpoints;
- parse JSON and preserve the last valid values;
- update runtime variables and LVGL screens;
- fetch weather and GNews content directly;
- store device settings in NVS.

The backend endpoint is a compile-time setting in `esp/src/pulsemon_api_config.h`. There is no active backend discovery or NVS backend-address setting.

## Active firmware navigation

```text
Main <-> GPU <-> Weather
```

The generated FAN screen and related helpers remain in the tree but are deliberately excluded from active navigation.

## Data ownership

```text
Backend: Linux telemetry, API payloads, in-memory history, SQLite configuration
Firmware: Wi-Fi state, display cache, LVGL navigation, weather/news data, NVS settings
```

## EEZ ownership

- `esp/src/ui/`: generated output compiled by the firmware; never edit directly.
- `esp/eez/pulsmon/`: EEZ Studio project and saved application state; modify only through EEZ Studio.
- Runtime integration outside generated files belongs in `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` and related non-generated modules.

## Transport

Backend telemetry uses local HTTP with JSON. Weather currently uses OpenWeather over plain HTTP. GNews uses HTTPS and an `X-Api-Key` header.
