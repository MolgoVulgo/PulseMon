# PulseMon ESP32-S3 firmware

This directory contains the ESP32-S3 firmware built with PlatformIO, ESP-IDF and LVGL 8.4.

Runtime entry point:

```text
esp/src/main.c
```

## Build environments

Main release environment:

```bash
pio run -e pulsmon-esp32s3-display
```

Development environment:

```bash
pio run -e pulsmon-esp32s3-display-dev
```

`pulsmon-esp32s3-display` is the default environment.

## Current runtime

The firmware:

- stores Wi-Fi credentials in NVS;
- polls the backend dashboard and GPU dashboard;
- caches the last valid metrics;
- renders active Main, GPU and Weather screens;
- fetches OpenWeather data and GNews headlines independently from the backend;
- exposes a local configuration web server for Wi-Fi, weather and news settings.

The backend URL is compiled in `src/pulsemon_api_config.h`. It is not configurable through NVS or the web portal in the current implementation. The firmware does not currently send the optional backend API-key header.

## FAN status

FAN client types, parsing code, generated UI objects and dormant runtime helpers remain in the source tree. The FAN screen is intentionally excluded from active navigation and the poller does not call the fan endpoint.

## EEZ ownership rules

```text
esp/src/ui/              -> UI output compiled by the firmware; never edit directly
esp/eez/pulsmon/         -> EEZ Studio project and saved application state; edit only through EEZ Studio
```

Changes to generated UI behavior must originate in EEZ Studio and then be regenerated. Runtime integration outside generated output belongs in files such as `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` and `ui_backend.h`.

## Known transport issue

OpenWeather requests currently use plain HTTP. GNews requests use HTTPS with the `X-Api-Key` header.

Canonical documentation is under `../docs/`. In the supplied snapshot, `esp/docs` contains the relative target `../docs`.
