# ESP32-S3 firmware

The firmware uses PlatformIO, ESP-IDF and LVGL 8.4 on the custom `jc3248w535c` board definition.

## Build environments

- `pulsmon-esp32s3-display`: default release build;
- `pulsmon-esp32s3-display-dev`: debug build with PulseMon diagnostics flags.

## Startup

`esp/src/main.c` initializes:

1. display and generated UI;
2. weather icon support;
3. Wi-Fi manager, NVS and AP lifecycle callbacks;
4. weather and news services;
5. Wi-Fi connection;
6. configuration HTTP server and captive DNS only when the setup AP actually starts;
7. backend poller after station connection.

## Active screens and navigation

```text
Main --left--> GPU --left--> Weather
Main <--right-- GPU <--right-- Weather
```

Only Main, GPU and Weather are valid navigation targets.

## Backend polling

`PULSEMON_DASHBOARD_POLL_MS` defaults to `1000 ms`.

- GPU screen: request `/api/v1/gpu/dashboard`.
- Other active screens: request `/api/v1/dashboard`.
- On backend failure: keep the last display values, mark backend offline and automatically switch to Weather.
- When the backend returns after an automatic offline switch: return to Main.

The backend host and port are loaded from NVS namespace `pulsemon_api`. `pulsemon_api_config.h` supplies the compiled fallback host/port and fixed timeout/polling values. Portal changes are reloaded immediately. The firmware does not use backend discovery or an API-key header. The portal is not a permanent LAN service: HTTP and captive DNS start with the setup AP and stop when the AP stops. A five-second hold in the top-left corner of Main, GPU or Weather opens a ten-minute manual window while preserving the station connection. The trigger is implemented in non-generated runtime code and does not modify EEZ outputs.

## Weather and news

Weather and GNews run independently from backend availability once Wi-Fi and required keys are available. Both use HTTPS with certificate validation through the ESP-IDF certificate bundle and retain local cached state according to their service implementations.

## EEZ files

- `esp/src/ui/` is generated firmware output. Never edit it directly.
- `esp/eez/pulsmon/` is the EEZ Studio project and saved application state. Modify it only through EEZ Studio.
- Non-generated runtime integration remains outside those generated files.

The generated output retains an unreachable legacy FAN screen and required compatibility bindings. It is not loaded by active navigation and is not backed by a generic FAN API. Remove it only through EEZ Studio followed by regeneration.
