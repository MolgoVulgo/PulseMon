# PulseMon agent context

## Source of truth

Use the supplied `PulseMon.zip` snapshot and the files actually present in it. A newly supplied snapshot replaces any previous snapshot and patch chain. Do not use remote repositories and do not reconstruct intentionally filtered files from assumptions.

Read first:

1. `README.md` or `README.fr.md`;
2. `docs/README.md` and the relevant canonical document;
3. `api/app/main.py` for active backend routes;
4. `api/app/config.py` for the complete 20-variable `STATS_*` contract;
5. `esp/platformio.ini` for firmware environments;
6. `esp/src/main.c`, `pulsemon_api_settings.*`, `pulsemon_api_config.h`, `pulsemon_poller.c` and the relevant firmware modules;
7. the exact files involved in the requested change.

## Active architecture

```text
Linux backend: api/
ESP32-S3 firmware: esp/
Canonical documentation: docs/ and docs/fr/
```

Backend entry point: `api/app/main.py`.
Firmware entry point: `esp/src/main.c`.

## Current product contract

- monitoring: CPU, memory and AMD GPU, including GPU fan telemetry when exposed by the driver;
- backend state: current snapshots and bounded histories in memory only;
- backend HTTP API: seven monitoring routes under `/api/v1` plus `/ui`;
- firmware screens: Main, GPU and Weather;
- external content: OpenWeather and GNews fetched directly by the ESP32-S3;
- firmware persistence: backend host/port, Wi-Fi, weather and news settings in NVS;
- backend endpoint: NVS namespace `pulsemon_api`, with compiled fallback in `esp/src/pulsemon_api_config.h`;
- backend API key: optional server capability not currently sent by the firmware or backend UI;
- configuration portal: HTTP server and captive DNS active only while the setup AP is active, with DNS bound to `192.168.4.1`;
- local trigger: five-second hold in the top-left corner of Main/GPU/Weather, opening a ten-minute manual AP window while preserving station connectivity.

There is no active backend FAN subsystem, database layer, user-configuration API or administration UI. Do not reintroduce these contracts implicitly.

## Generated UI boundary

- Never edit `esp/src/ui/` directly.
- Never edit `esp/eez/pulsmon/` outside EEZ Studio.
- Runtime integration changes belong in the owning non-generated modules.

The generated output retains an unreachable legacy FAN screen and compatibility bindings. They are not an active feature. Their physical removal requires EEZ Studio and regeneration.

## Transport and secrets

- OpenWeather and GNews use HTTPS with certificate validation through the ESP-IDF certificate bundle.
- GNews uses `X-Api-Key`.
- Do not log or return Wi-Fi passwords or API keys.
- Treat the deployment as local and personal, but avoid unnecessary LAN exposure.
- Do not make the configuration portal or captive DNS permanent station-LAN services.

## Snapshot rules

`make-a.sh` intentionally creates a filtered `PulseMon.zip`. Excluded directories such as `tmp/`, build trees, caches and local SDK files may exist outside the snapshot. Do not classify an excluded file as missing unless the active contract requires it to be present in the snapshot.

## Validation rules

Backend:

```bash
cd api
python3 -m app.config
python3 -m pytest -q
```

All backend environment variables are validated centrally before runtime startup. Do not add direct `STATS_*` reads elsewhere in the Python runtime; the launcher may read validated bind/port/logging values to build the Uvicorn command.

Firmware build, flash or monitor must not be run without explicit user instruction. When requested, use an environment defined in `esp/platformio.ini`.

For the explicitly authorized P8 phase, Codex must run `python3 tools/p8_validate.py --codex-full` from the project root. This profile tests the backend, builds both firmware environments, flashes release firmware, captures 180 seconds of serial output and checks `http://192.168.0.10:8000`. Preserve `report.json`, `report.md` and `hardware-checklist.json`, then finalize the same evidence with `--resume-report ... --checklist-file ... --require-hardware`. Never claim hardware validation while any manual check is not `pass`.

Documentation changes must keep English and French aligned and must describe current behavior rather than intended future behavior.

Do not create commits, push changes or access remote repositories without explicit instruction.
