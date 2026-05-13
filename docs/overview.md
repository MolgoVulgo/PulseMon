# Overview

PulseMon is a local monitoring stack composed of a Linux backend and an ESP32-S3 display firmware.

The Linux backend collects host telemetry and exposes it through a compact HTTP API. The ESP32-S3 firmware polls the API, parses the data, caches the latest valid values, and renders the information through LVGL.

The project targets a personal workstation on a private LAN. It favors deterministic behavior, compact payloads, simple deployment, and local autonomy over large-scale monitoring features.

## Scope

PulseMon covers:

- CPU usage, temperature and optional power;
- memory usage and capacity;
- AMD GPU usage, temperature, power and detailed GPU screen data;
- fan speed monitoring and fan configuration mapping;
- short runtime history for charts;
- local debug/admin web UI;
- ESP32-S3 screens for main metrics, GPU data, weather and information line;
- autonomous weather and news retrieval from the ESP32-S3.

## Runtime model

The backend samples the host at a fixed cadence, stores the current snapshot in memory, stores short history in a ring buffer, and serves API responses from that memory state.

The firmware never calculates Linux metrics itself. It consumes the backend contract, accepts null values for unavailable metrics, and keeps the last valid display state when network or parsing errors occur.

## Non-goals

PulseMon is not a cloud monitoring service, a multi-host observability platform, a process-level profiler, a remote-control tool, or a public Internet-facing API.

It does not require MQTT, a broker, long-term database storage, strong multi-user authentication, or a rich web dashboard for normal operation.
