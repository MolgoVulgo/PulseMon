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
7. backend poller after station connection;
8. read-only printer presence monitor, with transient MQTT polling while the Printer screen is inactive and live MQTT polling only while that screen is active.

After the station receives an IP address, network work is started by a non-blocking one-shot timer rather than directly in the Wi-Fi event callback. The current sequence is: backend poller after 0.5 s, Weather 1.5 s later (2.0 s cumulative), GNews 6.0 s later (8.0 s cumulative), then the first Printer presence/status cycle 10.0 s later (18.0 s cumulative). Weather and GNews still share an HTTPS gate, so a slower Weather transaction makes GNews wait instead of creating a concurrent TLS handshake.

## Memory and TLS policy

The validated ESP32-S3 memory policy is shared by release and dev builds:

- PSRAM is available to normal `malloc()` and allocations larger than 4096 bytes prefer external memory;
- 32 KiB of internal memory is reserved for INTERNAL/DMA-only allocations;
- MbedTLS uses the default allocator with dynamic TLS buffers;
- the Weather 32 KiB response body and GNews 16 KiB response body are allocated explicitly in PSRAM;
- Weather and GNews are serialized by `pulsemon_https_gate` across the complete HTTPS operation;
- the full LVGL draw buffer is in PSRAM while the two DMA transfer buffers use `hres * vres / 20`, about 30 KiB total at 320 × 480 RGB565.

The tuned task stacks are 6144 bytes for the backend poller, 7168 bytes for `MeteoTask`, 8192 bytes for `NewsTask` and 3072 bytes for `MeteoClock`. Heap, DMA and stack high-water diagnostics are compiled only in the dev environment through `PULSEMON_DEBUG`; the release build keeps the runtime protections but does not emit those diagnostic measurements or register the allocation-failure callback.

## Active screens and navigation

```text
Main --left--> GPU --left--> Weather --left--> Printer
Main <--right-- GPU <--right-- Weather <--right-- Printer
```

Printer navigation is unconditional and independent from printer availability. A lightweight periodic monitor checks the configured printer every 60 seconds while the screen is inactive. Each successful HTTP presence probe creates a short-lived MQTT worker, reads the current state, then destroys the MQTT client and worker. Entering Printer switches to live mode: the worker keeps its MQTT session and refreshes status every 5 seconds. Leaving Printer immediately ends that live session and returns to periodic monitoring. If an active print has already produced a valid display snapshot, that snapshot is retained in PSRAM while the screen is inactive and is restored immediately on the next Printer entry before live polling resumes. The EEZ-generated `imp_gone` label remains hidden while a cached active-job snapshot is available; otherwise it is visible until a valid method-1002 status makes the printer available. No file under `esp/src/ui/` is modified by the runtime integration.

## Backend polling

`PULSEMON_DASHBOARD_POLL_MS` defaults to `1000 ms`.

- GPU screen: request `/api/v1/gpu/dashboard`.
- Main, Weather and Printer screens: request `/api/v1/dashboard`. Printer availability itself is independent from backend availability.
- On backend failure: keep the last display values and mark the backend offline. Main/GPU automatically switch to Weather; an already active Printer screen is not displaced by backend failure.
- When the backend returns after an automatic offline switch: return to Main only if Weather is still the active screen; manual navigation elsewhere cancels the automatic return.

The backend host and port are loaded from NVS namespace `pulsemon_api`. `pulsemon_api_config.h` supplies the compiled fallback host/port and fixed timeout/polling values. Portal changes are reloaded immediately. The firmware does not use backend discovery or an API-key header. The portal is not a permanent LAN service: HTTP and captive DNS start with the setup AP and stop when the AP stops. A five-second hold in the top-left corner of Main, GPU or Weather opens a five-minute manual window while preserving the station connection. The trigger is implemented in non-generated runtime code and does not modify EEZ outputs.

## Printer telemetry

`esp/src/printer_service.c` talks directly to the configured printer on the private LAN. It does not use an intermediate library, daemon, cloud service or external broker. The current implementation is read-only. While the Printer screen is inactive, a 60-second timer starts a temporary worker only when a check is due. The worker first performs the existing HTTP `/system/info` request as the low-cost reachability probe; if that probe fails, no MQTT client is created. If it succeeds, MQTT is created only long enough to refresh identity/status and is then destroyed together with the worker. The 10 KiB Printer worker stack is therefore not kept resident between inactive checks. Only the last valid active-job display snapshot and its thumbnail remain cached locally between sessions.

The printer host and access code are loaded from NVS namespace `printer` (`host`, `access_code`). `printer_config.h` contains only fixed Printer runtime/protocol constants and no credentials. The local portal exposes the saved host and only a boolean access-code presence flag; changing or clearing Printer settings requests an immediate refresh. A live session drops the previous connection, while an inactive service starts a short background cycle with the new settings, without reboot.

Connection flow:

1. HTTP `GET /system/info?X-Token=<access-code>` on port 80 acts as the presence probe and retrieves the printer serial number;
2. only after that probe succeeds, MQTT 3.1.1 connects directly to port 1883 with username `elegoo` and the printer access code as password;
3. the client registers under the printer serial-number topic tree;
4. method `1001` reads printer identity and method `1002` reads current job status;
5. while Printer is inactive, one status cycle is performed and MQTT is destroyed immediately afterwards;
6. while Printer is active, the MQTT session remains open, an application-level `PING` is sent every 30 seconds and full status is refreshed every 5 seconds.

The service updates `name_printer`, `printer_ip`, `print_file_name`, `print_time_start`, `print_time_end`, `print_time_elapsed`, `print_time_remaining`, `print_layer` and `print_bar`. `print_layer` is populated from `layerProgress` and keeps the printer format such as `123/456`. The displayed `print_file_name` removes a final `.gcode` suffix case-insensitively, while the raw filename is retained internally for job-change detection and method `1045`. Start/end clock values are derived from the current local clock plus the printer-reported elapsed/remaining durations. MQTT responses always update the internal snapshot/cache, but LVGL variables are written only while the Printer screen is active; when entering Printer, a cached active-job snapshot is restored first. Leaving Printer does not clear that cache. A no-job status or a Printer configuration change invalidates the cached job. A valid status response makes Printer available; HTTP probe, MQTT or status failure makes it unavailable without erasing an already cached active-job snapshot.

For the current-job preview, method `1002` remains the source of the raw active filename. A background status cycle can detect a job change and clear a stale cached preview, but it does not fetch a new thumbnail while Printer is inactive. In live Printer mode, the firmware sends read-only MQTT method `1045 GET_FILE_THUMBNAIL` with `storage_media="local"` and the unchanged raw `file_name=<filename>`. A successful response returns the slicer preview in `result.thumbnail` as base64 PNG. The firmware decodes that PNG into PSRAM and transfers ownership to the display cache. MQTT response reassembly uses a bounded temporary allocation (maximum 256 KiB) requested explicitly from PSRAM first, with normal heap fallback. The cached PNG is intentionally retained when leaving Printer if the job has not changed. It is cleared when the active job changes or ends, or when Printer settings are reloaded.

`esp/src/printer_thumbnail.c` binds the runtime image to the EEZ-generated `image_gode` frame without modifying `esp/src/ui/`. LVGL PNG support is already enabled by `LV_USE_PNG=1`; the runtime image cache is set to one entry so the PNG is decoded once and reused while displayed. The previous preview is cleared on job change or when the job ends. A transient MQTT/thumbnail failure is retried no more than once every 30 seconds; a valid method-1045 response with no usable PNG is not retried until the active filename changes. The printer access code is not part of the thumbnail request and is never written to logs.

## Weather and news

Weather and GNews run independently from backend availability once Wi-Fi and required keys are available. Both use HTTPS with certificate validation through the ESP-IDF certificate bundle and retain local cached state according to their service implementations.

## EEZ files

- `esp/src/ui/` is generated firmware output. Never edit it directly.
- `esp/eez/pulsmon/` is the EEZ Studio project and saved application state. Modify it only through EEZ Studio.
- Non-generated runtime integration remains outside those generated files.

The generated output retains an unreachable legacy FAN screen and required compatibility bindings. It is not loaded by active navigation and is not backed by a generic FAN API. Remove it only through EEZ Studio followed by regeneration.
