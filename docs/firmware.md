# ESP32-S3 firmware

The firmware uses PlatformIO, ESP-IDF and LVGL 8.4 on the custom `jc3248w535c` board definition.

## Build environments

- `pulsmon-esp32s3-display`: default release build;
- `pulsmon-esp32s3-display-dev`: debug build with PulseMon diagnostics flags.

Automatic SD screenshots are disabled by default in both release and dev builds. `PULSEMON_SCREENSHOT_AUTOSTART=0` is set in the common build flags and `capture_config.h` also defaults the same guard to `0`; therefore `main.c` does not start the periodic `lcd_capture` task unless both screenshot debug support and the separate autostart guard are explicitly enabled.

## Startup

`esp/src/main.c` initializes:

1. display and generated UI;
2. weather icon support;
3. Wi-Fi manager, NVS and AP lifecycle callbacks;
4. weather and news workers plus the read-only Printer presence monitor;
5. Wi-Fi connection;
6. configuration HTTP server and captive DNS only when the setup AP actually starts;
7. an autonomous first-network sequence after the station obtains an IP address;
8. the PC backend poller last, without making backend availability a startup requirement.

After `IP_EVENT_STA_GOT_IP`, a dedicated `startup_net` task performs the first-network sequence outside the Wi-Fi event callback. The normal order is: start SNTP against `pool.ntp.org` and wait up to 8 seconds for a valid system clock, request the first Weather cycle and wait up to 12 seconds for that worker to finish, request the first GNews cycle and wait up to 10 seconds, request the first Printer background cycle, then start the PC backend poller. Weather and GNews continue to share `pulsemon_https_gate`, so their HTTPS/TLS work remains serialized. The Printer request is intentionally non-blocking at startup: the inactive Printer worker performs its HTTP presence probe and, only when the configured printer answers, continues with the transient MQTT status cycle. The backend poller is launched last. Startup then waits up to 9 seconds for the poller to resolve the initial backend state: a confirmed online backend releases the splash directly to Main; an unavailable or still unresolved backend releases it to Weather with Main/GPU navigation disabled.

The startup screen waits at most 10 seconds for the initial Wi-Fi connection. If no station IP is available by then, the firmware releases the startup screen directly to Weather with the PC-monitoring screens disabled while `startup_net` keeps waiting in the background; the same NTP/Weather/GNews/Printer/backend sequence starts when Wi-Fi later obtains an IP address. When the backend later becomes confirmed online, Main/GPU are re-enabled and PulseMon returns to Main if Weather is still the active autonomous fallback screen. An NTP timeout is non-fatal: SNTP remains active for later synchronization and startup continues instead of depending on the PC clock. The system clock is UTC from SNTP; the Weather clock display applies the configured `gmt_min` offset locally.

## Memory and TLS policy

The validated ESP32-S3 memory policy is shared by release and dev builds:

- PSRAM is available to normal `malloc()` and allocations larger than 4096 bytes prefer external memory;
- 32 KiB of internal memory is reserved for INTERNAL/DMA-only allocations;
- MbedTLS uses the default allocator with dynamic TLS buffers;
- the Weather 32 KiB response body and GNews 16 KiB response body are allocated explicitly in PSRAM;
- Weather and GNews are serialized by `pulsemon_https_gate` across the complete HTTPS operation;
- the full LVGL draw buffer is in PSRAM while the two DMA transfer buffers use `hres * vres / 20`, about 30 KiB total at 320 × 480 RGB565.

The tuned task stacks are 6144 bytes for the backend poller, 7168 bytes for `MeteoTask`, 8192 bytes for `NewsTask` and 3072 bytes for `MeteoClock`. The LVGL task uses 5120 bytes and the transient Printer worker uses 7168 bytes; these two values are based on DEV high-water measurements from a real active-print session including thumbnail retrieval. Heap, DMA and stack high-water diagnostics are compiled only in the dev environment through `PULSEMON_DEBUG`; the release build keeps the runtime protections but does not emit those diagnostic measurements or register the allocation-failure callback. The dev build additionally samples LVGL heap/stack once per minute, the Printer worker after its first successful status and before task deletion, and heap state around Printer thumbnail cache updates. It also reports the blocking LVGL flush cost once per minute and, for animated screen changes, the exact `LV_EVENT_SCREEN_LOADED` elapsed time together with the flush count/average/maximum for that transition.

## Active screens and navigation

```text
Main --left--> GPU --left--> Weather --left--> Printer
Main <--right-- GPU <--right-- Weather <--right-- Printer
```

Animated swipe transitions use a shared 160 ms duration. The startup fade remains 200 ms. The shorter navigation duration is intentional: DEV measurements on the previous 220 ms setting showed 70 real transitions averaging about 250 ms, while display flush work accounted for only part of that elapsed time. When the PC backend is confirmed offline, Main and GPU are temporarily excluded from navigation, leaving Weather <-> Printer available. They are restored only after backend recovery is confirmed.

Printer navigation is unconditional and independent from printer availability. A lightweight periodic monitor checks the configured printer every 60 seconds while the screen is inactive. Each successful HTTP presence probe creates a short-lived MQTT worker, reads the current state, then destroys the MQTT client and worker. Entering Printer switches to live mode: the worker keeps its MQTT session and refreshes status every 5 seconds. Leaving Printer immediately ends that live session and returns to periodic monitoring. If an active print has already produced a valid display snapshot, that snapshot is retained in PSRAM while the screen is inactive and is restored immediately on the next Printer entry before live polling resumes. The EEZ-generated `imp_gone` label remains hidden while a cached active-job snapshot is available; otherwise it is visible until a valid method-1002 status makes the printer available. No file under `esp/src/ui/` is modified by the runtime integration.

## Backend polling

`PULSEMON_DASHBOARD_POLL_MS` defaults to `1000 ms`.

- GPU screen: request `/api/v1/gpu/dashboard`.
- Main, Weather and Printer screens: request `/api/v1/dashboard`. Printer availability itself is independent from backend availability.
- Backend availability is tracked as `UNKNOWN`, `ONLINE`, `SUSPECT` or `OFFLINE`. A single failed request only changes `ONLINE`/`UNKNOWN` to `SUSPECT`; the current screen and last valid values are preserved.
- `OFFLINE` is entered only after failures have remained continuous for `PULSEMON_BACKEND_OFFLINE_GRACE_MS` (5000 ms). Main/GPU are then disabled and an active Main/GPU screen switches once to Weather; Printer is never displaced.
- While `OFFLINE`, recovery requires `PULSEMON_BACKEND_RECOVERY_SUCCESSES` (2) consecutive successful dashboard requests. A failed recovery probe resets that counter.
- Once recovery is confirmed, Main/GPU are re-enabled. If Weather is still the automatic fallback screen, PulseMon returns to Main; if the user is on Printer, that screen is left undisturbed.

The backend host and port are loaded from NVS namespace `pulsemon_api`. `pulsemon_api_config.h` supplies the compiled fallback host/port and fixed timeout/polling values. Portal changes are reloaded immediately. The firmware does not use backend discovery or an API-key header. The portal is not a permanent LAN service: HTTP and captive DNS start with the setup AP and stop when the AP stops. A five-second hold in the top-left corner of Main, GPU or Weather opens a five-minute manual window while preserving the station connection. The trigger is implemented in non-generated runtime code and does not modify EEZ outputs.

## Printer telemetry

`esp/src/printer_service.c` talks directly to the configured printer on the private LAN. It does not use an intermediate library, daemon, cloud service or external broker. The current implementation is read-only. While the Printer screen is inactive, a 60-second timer starts a temporary worker only when a check is due. The worker first performs the existing HTTP `/system/info` request as the low-cost reachability probe; if that probe fails, no MQTT client is created. If it succeeds, MQTT is created only long enough to refresh identity/status and is then destroyed together with the worker. The 7 KiB Printer worker stack is therefore not kept resident between inactive checks. Only the last valid active-job display snapshot and its thumbnail remain cached locally between sessions.

The printer host and access code are loaded from NVS namespace `printer` (`host`, `access_code`). `printer_config.h` contains only fixed Printer runtime/protocol constants and no credentials. The local portal exposes the saved host and only a boolean access-code presence flag; changing or clearing Printer settings requests an immediate refresh. A live session drops the previous connection, while an inactive service starts a short background cycle with the new settings, without reboot.

Connection flow:

1. HTTP `GET /system/info?X-Token=<access-code>` on port 80 acts as the presence probe and retrieves the printer serial number;
2. only after that probe succeeds, MQTT 3.1.1 connects directly to port 1883 with username `elegoo` and the printer access code as password;
3. the client registers under the printer serial-number topic tree; the firmware waits up to 30 seconds for `register_response` before treating that MQTT session as unavailable, while the normal MQTT request timeout remains 5 seconds;
4. method `1001` reads printer identity and method `1002` reads current job status;
5. while Printer is inactive, one status cycle is performed and MQTT is destroyed immediately afterwards;
6. while Printer is active, the MQTT session remains open, an application-level `PING` is sent every 30 seconds and full status is refreshed every 5 seconds.

The service updates `name_printer`, `printer_ip`, `print_file_name`, `print_time_start`, `print_time_end`, `print_time_elapsed`, `print_time_remaining`, `print_layer` and `print_bar`. `print_layer` is always formatted as `current/total` for an active job: the current layer comes from method `1002` field `result.print_status.current_layer`, while the total layer count is fetched separately with read-only method `1046 GET_FILE_DETAIL` using `storage_media="local"` and the raw `filename`; the total is read from `result.layer`. The 1046 lookup is cached per active filename and retried no more than once every 30 seconds while the total is unavailable. While the total is still pending, the runtime keeps the two-part display shape as `current/--`. Legacy textual `layerProgress` remains accepted only as a fallback when `current_layer` itself is unavailable. The displayed `print_file_name` is the original model name: it is truncated at the first `.stl`, `.obj` or `.3mf` source-model extension, case-insensitively, so slicer-added suffixes are hidden; if no source-model extension is present, only a final `.gcode` suffix is removed. The raw printer filename is retained internally for job-change detection and methods `1045`/`1046`. `print_time_elapsed` advances locally once per second while Printer is active, anchored to the latest printer `print_duration`; each method-1002 status acts as a correction/resynchronization rather than being the only clock update. Cached Printer display restoration also derives elapsed time from that same anchor. Start/end clock values continue to be recalculated from the current local clock and the latest printer-reported elapsed/remaining durations. MQTT responses always update the internal snapshot/cache, but LVGL variables are written only while the Printer screen is active; when entering Printer, a cached active-job snapshot is restored first. Leaving Printer does not clear that cache. A no-job status or a Printer configuration change invalidates the job in cache. The authenticated HTTP probe is the presence authority for an inactive-screen cycle: a successful probe sets Printer available before MQTT starts. If the short-lived MQTT session or status request then fails in the background, availability remains set and the worker is destroyed; the next 60-second cycle retries normally. A failed HTTP probe makes Printer unavailable. In live Printer mode, an MQTT/session or status failure can still mark live telemetry unavailable until the next reconnect/probe, without erasing an already cached active-job snapshot.

For the current-job preview, method `1002` remains the source of the raw active filename. A background status cycle can detect a job change and clear a stale cached preview, but it does not fetch a new thumbnail while Printer is inactive. In live Printer mode, the firmware sends read-only MQTT method `1045 GET_FILE_THUMBNAIL` with `storage_media="local"` and the unchanged raw `file_name=<filename>`. A successful response returns the slicer preview in `result.thumbnail` as base64 PNG. The firmware decodes that PNG into PSRAM and transfers ownership to the display cache. MQTT response reassembly uses a bounded temporary allocation (maximum 256 KiB) requested explicitly from PSRAM first, with normal heap fallback. The cached PNG is intentionally retained when leaving Printer if the job has not changed. It is cleared when the active job changes or ends, or when Printer settings are reloaded.

`esp/src/printer_thumbnail.c` binds the runtime image to the EEZ-generated `image_gode` frame without modifying `esp/src/ui/`. LVGL PNG support is enabled by `LV_USE_PNG=1`, and `LV_IMG_CACHE_DEF_SIZE=1` enables one decoded-image cache entry so the PNG can be reused while displayed instead of being decoded for every redraw. Large malloc-backed image allocations continue to follow the firmware PSRAM preference policy. The previous preview is cleared on job change or when the job ends. A transient MQTT/thumbnail failure is retried no more than once every 30 seconds; a valid method-1045 response with no usable PNG is not retried until the active filename changes. The printer access code is not part of the thumbnail request and is never written to logs.

## Weather and news

Weather and GNews run independently from backend availability once Wi-Fi and required keys are available. System time is synchronized directly by the ESP32-S3 through SNTP before their normal first-start sequence; the PC backend is not a clock source. Both use HTTPS with certificate validation through the ESP-IDF certificate bundle and retain local cached state according to their service implementations.

## EEZ files

- `esp/src/ui/` is generated firmware output. Never edit it directly.
- `esp/eez/pulsmon/` is the EEZ Studio project and saved application state. Modify it only through EEZ Studio.
- Non-generated runtime integration remains outside those generated files.

The generated output retains an unreachable legacy FAN screen and required compatibility bindings. It is not loaded by active navigation and is not backed by a generic FAN API. Remove it only through EEZ Studio followed by regeneration.
