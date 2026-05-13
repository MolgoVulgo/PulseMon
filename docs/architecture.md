# Architecture

PulseMon is split into two runtime components: the Linux backend and the ESP32-S3 firmware.

## Linux backend

The backend responsibilities are:

- collect Linux metrics;
- read CPU, memory, AMD GPU and fan telemetry;
- apply ordered fallback rules for sensors;
- normalize units and numeric precision;
- keep the current snapshot in memory;
- keep short history in memory;
- expose the HTTP API;
- expose a local debug/admin UI;
- persist user and fan configuration when required.

The backend must not perform heavy sensor reads inside HTTP handlers. Handlers read the already-normalized in-memory store.

## ESP32-S3 firmware

The firmware responsibilities are:

- connect to Wi-Fi;
- provide a captive configuration portal when needed;
- locate or use the configured backend address;
- poll backend endpoints;
- parse JSON payloads;
- maintain local display caches;
- update LVGL screens;
- show network and data freshness state;
- fetch autonomous weather and news data when configured.

The firmware must not compute Linux metrics, infer missing fields, or depend on an HTTP request being active during rendering.

## Data ownership

The backend owns Linux telemetry and API payload generation.

The firmware owns display state, UI navigation, Wi-Fi state, weather display, autonomous news display and local device configuration.

## Transport

The backend and firmware communicate over local HTTP using compact JSON. Polling is preferred because there is one primary embedded client and the deployment target is a private LAN.

MQTT and broker-based communication are not part of the active architecture.

## Runtime separation

The firmware pipeline is:

```text
network -> HTTP client -> JSON parser -> local cache -> LVGL variables -> rendered screens
```

The backend pipeline is:

```text
sensor reads -> normalization -> snapshot/history store -> API serialization
```

This separation prevents UI freezes, avoids duplicated logic, and keeps failure handling deterministic.
