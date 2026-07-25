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

## Integrated validation

From the project root, `python3 tools/p8_validate.py --codex-full` runs the explicitly authorized P8 workflow: backend tests, both firmware builds, release flashing, a 180-second serial capture and API checks against `192.168.0.10:8000`. Codex must finalize the same report with the generated hardware checklist. See `../docs/p8-validation.md`.

## Active runtime

The firmware:

- stores Wi-Fi credentials in NVS;
- polls the main backend dashboard and GPU dashboard;
- caches the last valid metrics;
- renders active Main, GPU and Weather screens;
- fetches OpenWeather data and GNews headlines independently from the backend;
- stores the backend endpoint, weather and news settings in NVS;
- exposes a local configuration web server for Wi-Fi, backend, weather and news settings only while the setup AP is active; a five-second hold in the top-left corner opens a ten-minute manual window.

The backend host and port are stored in NVS namespace `pulsemon_api` and are editable through the web portal. `src/pulsemon_api_config.h` supplies the compiled fallback host/port plus the HTTP timeout and polling interval. The firmware does not currently store or send the optional backend API-key header. The portal HTTP server and captive DNS follow the real setup-AP lifecycle; captive DNS binds only to `192.168.4.1`. `src/config_mode_trigger.c` adds an invisible runtime hotspot to Main, GPU and Weather; a five-second hold opens the AP for ten minutes without modifying generated UI files.

## EEZ ownership rules

```text
esp/src/ui/              -> generated UI compiled by firmware; never edit directly
esp/eez/pulsmon/         -> EEZ Studio project and saved state; edit only through EEZ Studio
```

Changes to generated UI behavior must originate in EEZ Studio and then be regenerated. Runtime integration outside generated output belongs in files such as `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` and `ui_backend.h`.

The generated UI still contains an unreachable legacy FAN screen and required compatibility bindings. Active navigation contains only Main, GPU and Weather. The legacy generated elements must not be edited or removed manually.

## Transport security

OpenWeather and GNews requests use HTTPS with certificate validation through the ESP-IDF certificate bundle. GNews also uses the `X-Api-Key` header.

Canonical documentation is under `../docs/`. In the supplied snapshot, `esp/docs` contains the relative target `../docs`.
