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
8. read-only printer service, started on demand only while the Printer screen is active.

## Active screens and navigation

```text
Main --left--> GPU --left--> Weather --left--> Printer
Main <--right-- GPU <--right-- Weather <--right-- Printer
```

Printer navigation is unconditional and independent from printer availability. Entering Printer starts the read-only printer service; leaving Printer stops its MQTT session and task. If an active print has already produced a valid display snapshot, that snapshot is retained in PSRAM while the screen is inactive and is restored immediately on the next Printer entry before live polling resumes. The EEZ-generated `imp_gone` label remains hidden while a cached active-job snapshot is available; otherwise it is visible until a valid method-1002 status makes the printer available. No file under `esp/src/ui/` is modified by the runtime integration.

## Backend polling

`PULSEMON_DASHBOARD_POLL_MS` defaults to `1000 ms`.

- GPU screen: request `/api/v1/gpu/dashboard`.
- Main, Weather and Printer screens: request `/api/v1/dashboard`. Printer availability itself is independent from backend availability.
- On backend failure: keep the last display values and mark the backend offline. Main/GPU automatically switch to Weather; an already active Printer screen is not displaced by backend failure.
- When the backend returns after an automatic offline switch: return to Main only if Weather is still the active screen; manual navigation elsewhere cancels the automatic return.

The backend host and port are loaded from NVS namespace `pulsemon_api`. `pulsemon_api_config.h` supplies the compiled fallback host/port and fixed timeout/polling values. Portal changes are reloaded immediately. The firmware does not use backend discovery or an API-key header. The portal is not a permanent LAN service: HTTP and captive DNS start with the setup AP and stop when the AP stops. A five-second hold in the top-left corner of Main, GPU or Weather opens a five-minute manual window while preserving the station connection. The trigger is implemented in non-generated runtime code and does not modify EEZ outputs.

## Printer telemetry

`esp/src/printer_service.c` talks directly to the configured printer on the private LAN. It does not use an intermediate library, daemon, cloud service or external broker. The current implementation is read-only. The service has no Printer LAN traffic while the Printer screen is not active; its task and MQTT client are created on entry and released on exit. Only the last valid active-job display snapshot and its thumbnail remain cached locally while the service is stopped.

The printer host and access code are loaded from NVS namespace `printer` (`host`, `access_code`). `printer_config.h` contains only fixed protocol constants and no credentials. The local portal exposes the saved host and only a boolean access-code presence flag; changing or clearing Printer settings wakes an active service so it drops the previous session, reloads NVS and reconnects without reboot.

Connection flow:

1. HTTP `GET /system/info?X-Token=<access-code>` on port 80 retrieves the printer serial number;
2. MQTT 3.1.1 connects directly to port 1883 with username `elegoo` and the printer access code as password;
3. the client registers under the printer serial-number topic tree;
4. method `1001` reads printer identity and method `1002` reads current job status;
5. an application-level `PING` is sent every 30 seconds while the session is active;
6. full status is refreshed every 5 seconds.

The service updates `name_printer`, `printer_ip`, `print_file_name`, `print_time_start`, `print_time_end`, `print_time_elapsed`, `print_time_remaining` and `print_bar`. Start/end clock values are derived from the current local clock plus the printer-reported elapsed/remaining durations. While a print is active, the displayed values are mirrored into a small PSRAM-only runtime cache. Leaving Printer does not clear that cache; returning restores it immediately, then the service reconnects and replaces it with the next valid status. A no-job status or a Printer configuration change invalidates the cached job. A valid status response makes Printer available; bootstrap, MQTT or status failure makes it unavailable without erasing an already cached active-job snapshot.

For the current-job preview, method `1002` remains the source of the active filename. When that filename changes, the firmware sends read-only MQTT method `1045 GET_FILE_THUMBNAIL` with `storage_media="local"` and `file_name=<filename>`. A successful response returns the slicer preview in `result.thumbnail` as base64 PNG. The firmware decodes that PNG into PSRAM and transfers ownership to the display cache. MQTT response reassembly uses a bounded temporary allocation (maximum 256 KiB) requested explicitly from PSRAM first, with normal heap fallback. The cached PNG is intentionally retained when the Printer service stops so it can be shown immediately on the next screen entry. It is cleared when the active job changes or ends, or when Printer settings are reloaded.

`esp/src/printer_thumbnail.c` binds the runtime image to the EEZ-generated `image_gode` frame without modifying `esp/src/ui/`. LVGL PNG support is already enabled by `LV_USE_PNG=1`; the runtime image cache is set to one entry so the PNG is decoded once and reused while displayed. The previous preview is cleared on job change or when the job ends. A transient MQTT/thumbnail failure is retried no more than once every 30 seconds; a valid method-1045 response with no usable PNG is not retried until the active filename changes. The printer access code is not part of the thumbnail request and is never written to logs.

## Weather and news

Weather and GNews run independently from backend availability once Wi-Fi and required keys are available. Both use HTTPS with certificate validation through the ESP-IDF certificate bundle and retain local cached state according to their service implementations.

## EEZ files

- `esp/src/ui/` is generated firmware output. Never edit it directly.
- `esp/eez/pulsmon/` is the EEZ Studio project and saved application state. Modify it only through EEZ Studio.
- Non-generated runtime integration remains outside those generated files.

The generated output retains an unreachable legacy FAN screen and required compatibility bindings. It is not loaded by active navigation and is not backed by a generic FAN API. Remove it only through EEZ Studio followed by regeneration.
