# Architecture

PulseMon has two active runtimes.

## Linux backend

Entry point: `api/app/main.py`.

Responsibilities:

- collect CPU, memory and AMD GPU telemetry;
- normalize values and preserve explicit invalid states;
- publish current main and GPU snapshots;
- maintain bounded in-memory histories;
- expose the HTTP API and local debug UI.

HTTP handlers consume services and stores. They do not own the high-frequency sampling loop. The backend has no persistent database.

## ESP32-S3 firmware

Entry point: `esp/src/main.c`.

Responsibilities:

- manage Wi-Fi and the setup-AP-scoped local configuration portal;
- poll backend dashboard endpoints;
- parse JSON and preserve the last valid values;
- update runtime variables and LVGL screens;
- fetch weather and GNews content directly;
- store the backend host/port, Wi-Fi, weather and news settings in NVS.

The backend endpoint is loaded from NVS namespace `pulsemon_api`. `esp/src/pulsemon_api_config.h` provides the compiled fallback host/port and fixed polling/timeout values. There is no automatic backend discovery. The HTTP portal follows the actual AP lifecycle: it starts on `WIFI_EVENT_AP_START`, stops on `WIFI_EVENT_AP_STOP`, and its handlers reject requests when the AP is inactive. Captive DNS is bound only to the setup AP address `192.168.4.1`. A runtime-only invisible hotspot on Main, GPU and Weather opens the setup AP after a five-second hold in the top-left corner. The manual window lasts ten minutes while a working station connection remains active.

## Active firmware navigation

```text
Main <-> GPU <-> Weather
```

## Data ownership

```text
Backend: Linux telemetry, API payloads, bounded in-memory histories
Firmware: Wi-Fi state, display cache, LVGL navigation, weather/news data, NVS settings
```

## EEZ ownership

- `esp/src/ui/`: generated output compiled by the firmware; never edit directly.
- `esp/eez/pulsmon/`: EEZ Studio project and saved application state; modify only through EEZ Studio.
- Runtime integration outside generated files belongs in `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` and related non-generated modules.

The generated UI retains an unreachable legacy FAN screen and compatibility bindings. These artifacts do not define an active route, API or navigation target. Their removal requires EEZ Studio and regeneration.

## Transport

Backend telemetry uses local HTTP with JSON. OpenWeather and GNews use HTTPS with certificate validation through the ESP-IDF certificate bundle. GNews also uses an `X-Api-Key` header.
