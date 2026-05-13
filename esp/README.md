# PulseMon ESP32-S3 firmware

This directory contains the ESP32-S3 firmware entry point and firmware-specific files.

The firmware connects to Wi-Fi, locates the backend, polls the HTTP API, parses compact JSON payloads, stores the latest valid values locally, updates LVGL screens, and displays connection/freshness state.

It also contains autonomous device-side features such as the configuration portal, weather display, weather icons, and GNews headline retrieval. These features do not depend on the Linux backend once the ESP32-S3 has network access and the required API keys are stored in NVS.

The canonical firmware documentation is available in:

- `/docs/firmware.md` for runtime behavior;
- `/docs/web-configuration.md` for the ESP32 configuration portal;
- `/docs/weather-news.md` for weather and news modules;
- `/docs/api.md` for backend endpoints consumed by the firmware;
- `/docs/development.md` for build and maintenance rules.

The `esp/docs` path is a symbolic link to `/docs`.

## Build

```bash
pio run -e LVGL-320-480
```

## Runtime rules

- Wi-Fi credentials are stored in NVS.
- Backend polling is separated from LVGL rendering.
- The UI uses the last valid local cache, not direct HTTP state.
- Generated files under `src/ui/` must not be edited directly.
- API keys must not be logged, displayed or returned by configuration endpoints.
