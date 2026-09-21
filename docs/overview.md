# Overview

PulseMon is a local monitoring stack for one Linux workstation and one ESP32-S3 display on a private LAN.

## Active scope

The Linux backend provides:

- CPU usage, temperature and optional power;
- memory used, total and percentage;
- AMD GPU usage, clocks, VRAM, temperature, power and GPU fan telemetry when exposed by the driver;
- current main and GPU dashboard snapshots;
- bounded main and GPU histories in memory;
- a local debug UI under `/ui`.

The ESP32-S3 provides:

- Wi-Fi station setup with fallback configuration AP;
- polling of the main and GPU backend dashboards;
- local caching of the last valid display values;
- active Main, GPU and Weather screens;
- autonomous weather and GNews headline retrieval;
- local web configuration for the backend endpoint, Wi-Fi, weather and news settings while the setup AP is active, with a five-second on-device touch trigger for a bounded manual window.

## Runtime model

```text
Linux sensors -> collectors -> services -> in-memory stores -> FastAPI JSON
FastAPI JSON -> ESP HTTP client -> parser/cache -> runtime variables -> LVGL screens
```

Weather and news are fetched directly by the ESP32-S3 and do not transit through the Linux backend.

## Persistence model

- backend snapshots and histories: volatile memory only;
- ESP32 backend endpoint, Wi-Fi, weather and news settings: NVS;
- backend endpoint fallback, HTTP timeout and polling interval: compile-time firmware configuration;
- long-term metrics storage: not implemented.

## Non-goals

The current project is not a cloud service, multi-host observability platform, remote-control system, long-term metrics database or public Internet API. MQTT, an external broker, a generic FAN-management subsystem and strong multi-user security are outside the active design.
