# Overview

PulseMon is a local monitoring stack for one Linux workstation and one ESP32-S3 display on a private LAN.

## Active scope

The Linux backend currently provides:

- CPU usage, temperature and optional power;
- memory used, total and percentage;
- AMD GPU usage, clocks, VRAM, temperatures, power and GPU fan telemetry when exposed by the driver;
- current dashboard snapshots;
- bounded main and GPU histories in memory;
- a local debug/admin UI;
- SQLite-backed user configuration.

The ESP32-S3 currently provides:

- Wi-Fi station setup with fallback configuration AP;
- polling of the main and GPU backend dashboards;
- local caching of the last valid display values;
- active Main, GPU and Weather screens;
- autonomous weather and GNews headline retrieval;
- local web configuration for Wi-Fi, weather and news settings.

## Retained FAN code

Fan collection, mappings, API routes and admin storage remain implemented on the backend. Related firmware client and UI code also remains, but FAN is not active in the current device navigation and is not polled by the firmware.

## Runtime model

```text
Linux sensors -> collectors -> services -> in-memory stores -> FastAPI JSON
FastAPI JSON -> ESP HTTP client -> parser/cache -> runtime variables -> LVGL screens
```

Weather and news are fetched directly by the ESP32-S3 and do not transit through the Linux backend.

## Non-goals

The current project is not a cloud service, multi-host observability platform, remote-control system, long-term metrics database or public Internet API. MQTT, an external broker and strong multi-user security are outside the active design.
