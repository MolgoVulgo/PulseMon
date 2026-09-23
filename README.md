# PulseMon

PulseMon is a local monitoring system composed of a Linux backend and an ESP32-S3 display.

The Linux host collects system metrics and exposes them through a FastAPI HTTP API. The ESP32-S3 polls that API, keeps the last valid values in a local cache and renders the active screens with LVGL. The intended deployment is a personal workstation on a private LAN, without a mandatory cloud service or an external broker. The optional Printer screen uses a direct LAN-only HTTP/MQTT link from the ESP32-S3 to the configured printer; it does not introduce a PulseMon broker or cloud dependency.

## Active scope

The current runtime provides:

- CPU usage, temperature and optional power telemetry;
- memory usage and capacity telemetry;
- AMD GPU usage, clocks, VRAM, temperature, power and GPU fan telemetry when available;
- current snapshots and bounded in-memory histories;
- a local backend debug UI under `/ui`;
- active ESP32-S3 screens for Main, GPU and Weather, plus a Printer screen that is navigable only while the configured printer is reachable;
- autonomous OpenWeather weather data and GNews headlines on the ESP32-S3;
- read-only printer telemetry fetched directly by the ESP32-S3 over the local network;
- NVS persistence for the backend endpoint, Wi-Fi, printer, weather and news settings on the ESP32-S3;
- a configuration portal and captive DNS exposed only while the setup AP is active, including an explicit five-second touch hold to open a bounded manual configuration window.

The backend has no persistent database. Current snapshots and histories are rebuilt after restart.

## Repository layout

```text
PulseMon/
├── api/                         # Linux backend
├── esp/                         # ESP32-S3 firmware
│   ├── src/ui/                  # EEZ-generated UI compiled by the firmware
│   └── eez/pulsmon/             # EEZ Studio project and saved application state
├── docs/                        # canonical English documentation
│   └── fr/                      # French mirror
├── tools/                       # reproducible P8 validation runner
├── make-a.sh                    # creates the intentionally filtered PulseMon.zip snapshot
├── README.md
└── README.fr.md
```

`docs/` is the canonical documentation root. In the published source, `api/docs` and `esp/docs` contain the relative target `../docs`; they represent the intended link to the canonical documentation. Google Drive does not prove the original filesystem object type, so that type must be checked in the application worktree before changing either path.

## Backend setup and execution

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

The API is exposed under `/api/v1/*` and the local UI under `/ui`.

Validate the backend environment before startup:

```bash
cd api
python3 -m app.config
```

Backend tests:

```bash
cd api
.venv/bin/pytest -q
```

## Firmware build

Main environment:

```bash
cd esp
pio run -e pulsmon-esp32s3-display
```

Development environment with PulseMon debug flags:

```bash
cd esp
pio run -e pulsmon-esp32s3-display-dev
```

The default PlatformIO environment is `pulsmon-esp32s3-display`.

## P8 validation runner

For Codex to run the complete P8 validation — backend tests, release/debug builds, release flash, serial monitoring and API checks against `192.168.0.10:8000`:

```bash
python3 tools/p8_validate.py --codex-full
```

The first report generates a hardware checklist. Codex must then finalize the same report with `--resume-report`, `--checklist-file` and `--require-hardware`. See `docs/p8-validation.md`.

## Firmware configuration model

The backend host and port, Wi-Fi credentials, Printer endpoint/access code, OpenWeather settings and GNews settings are stored in NVS through the local configuration portal. Printer connectivity uses NVS namespace `printer` with keys `host` and `access_code`; `esp/src/printer_config.h` now contains only fixed protocol ports/timeouts and no credentials.

The backend endpoint uses:

- NVS namespace `pulsemon_api`, keys `host` and `port`;
- compiled fallbacks `PULSEMON_API_DEFAULT_HOST` and `PULSEMON_API_DEFAULT_PORT` from `esp/src/pulsemon_api_config.h`;
- compiled `PULSEMON_HTTP_TIMEOUT_MS` and `PULSEMON_DASHBOARD_POLL_MS` values.

A portal save reloads the backend endpoint immediately and wakes an active Printer service so updated Printer settings are picked up without reboot. Clearing PulseMon configuration removes the backend and Printer NVS settings and restores the compiled backend fallback. The optional backend API key is not stored or sent by the current firmware. The printer service is read-only: it performs the local bootstrap/status protocol needed for display telemetry and exposes no print-control action.

## Security posture

PulseMon targets a local and personal deployment. Security remains proportionate to that context: secrets must not be logged or returned by configuration endpoints, inputs must remain validated and unnecessary LAN exposure should be avoided. The configuration HTTP server starts only with the setup AP and stops when the AP stops; captive DNS is bound to `192.168.4.1` only. Holding the top-left corner of Main, GPU or Weather for five seconds opens `PulseMon-Setup` for a five-minute manual configuration window without clearing credentials or disconnecting a working station link.

OpenWeather and GNews use HTTPS with certificate validation through the ESP-IDF certificate bundle. GNews additionally uses the `X-Api-Key` header. The printer access code is a secret stored in NVS: it must not be logged or returned by the configuration API. `printer_config.h` contains no credential placeholders.

## EEZ ownership and retained generated artifact

- `esp/src/ui/` is generated output compiled by the firmware and must never be edited directly.
- `esp/eez/pulsmon/` is owned by EEZ Studio and must not be modified outside EEZ Studio.
- Runtime integration belongs in the non-generated modules under `esp/src/`.

The generated UI still contains an unreachable legacy FAN screen and the compatibility bindings required to compile that generated output. It is not part of active navigation or the backend contract. Physical removal requires an EEZ Studio change followed by regeneration.

## Project publication and patch delivery

The connected Google Drive `pulsemon/` publication is the default context source for review and patch work. `REPO_INDEX.json` maps the current published source set; targeted files must be both declared in that index and actually readable from Drive. A newer published index replaces the previous context baseline.

Patch archives are delivered separately under `pulsemon/patch/`. That folder is not project source, is excluded from `REPO_INDEX.json` and must not be interpreted as an implicitly applied patch chain.

`make-a.sh` still creates the filtered local export `PulseMon.zip` for workflows that explicitly require a ZIP. It is not the default context source when the Drive publication is available unless the user explicitly designates a specific ZIP as the working base.

## Documentation

- `docs/README.md` — documentation index;
- `docs/overview.md` — active scope and non-goals;
- `docs/architecture.md` — ownership and data flows;
- `docs/api.md` — backend HTTP contract;
- `docs/backend.md` — backend runtime;
- `docs/firmware.md` — firmware runtime and active screens;
- `docs/configuration.md` — backend environment and firmware configuration;
- `docs/weather-news.md` — weather and news implementation;
- `docs/web-configuration.md` — ESP32 local portal;
- `docs/development.md` — build, tests, Drive publication and patch workflow;
- `docs/p8-validation.md` — P8 build and hardware-validation protocol;
- `docs/troubleshooting.md` — operational checks;
- `docs/fr/` — French mirror.
