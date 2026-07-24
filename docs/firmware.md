# ESP32-S3 firmware

The firmware uses PlatformIO, ESP-IDF and LVGL 8.4 on the custom `jc3248w535c` board definition.

## Build environments

- `pulsmon-esp32s3-display`: default release build;
- `pulsmon-esp32s3-display-dev`: debug build with PulseMon diagnostics flags.

## Startup

`esp/src/main.c` initializes:

1. display and generated UI;
2. weather icon support;
3. Wi-Fi manager and NVS;
4. weather and news services;
5. local configuration HTTP server;
6. captive DNS;
7. Wi-Fi connection and backend poller after station connection.

## Active screens and navigation

```text
Main --left--> GPU --left--> Weather
Main <--right-- GPU <--right-- Weather
```

The FAN screen exists in generated sources but is not a valid navigation target.

## Backend polling

`PULSEMON_DASHBOARD_POLL_MS` defaults to `1000 ms`.

- GPU screen: request `/api/v1/gpu/dashboard`.
- Other active screens: request `/api/v1/dashboard`.
- On backend failure: keep the last display values, mark backend offline and automatically switch to Weather.
- When the backend returns after an automatic offline switch: return to Main.

The backend base URL is compiled in `pulsemon_api_config.h`. The firmware does not use backend discovery, NVS backend configuration or an API-key header.

## Weather and news

Weather and GNews run independently from backend availability once Wi-Fi and required keys are available. Weather and news keep local cached state according to their service implementations.

## EEZ files

- `esp/src/ui/` is generated firmware output. Never edit it directly.
- `esp/eez/pulsmon/` is the EEZ Studio project and saved application state. Modify it only through EEZ Studio.
- Non-generated runtime integration remains outside those generated files.

## Retained FAN code

FAN structures and helpers remain, but the firmware poller does not fetch FAN data and active navigation cannot load the FAN screen.
