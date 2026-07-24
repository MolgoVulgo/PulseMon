# PulseMon agent context

## Source of truth

Use the supplied project snapshot and the files actually present in it. Do not replace missing or filtered content with assumptions.

Read first:

1. `README.md` or `README.fr.md`;
2. `docs/README.md` and the relevant document;
3. `api/app/main.py` for backend routes;
4. `api/app/config.py` and direct `STATS_*` usages for configuration;
5. `esp/platformio.ini` for firmware environments;
6. `esp/src/main.c`, `pulsemon_api_config.h`, `pulsemon_poller.c` and relevant firmware modules;
7. the exact files involved in the requested change.

## Active architecture

```text
Linux backend: api/
ESP32-S3 firmware: esp/
Canonical documentation: docs/ and docs/fr/
```

Backend entry point: `api/app/main.py`.
Firmware entry point: `esp/src/main.c`.

## Current product scope

Active monitoring covers CPU, memory and AMD GPU. Active firmware screens are Main, GPU and Weather. Weather and GNews are fetched directly by the ESP32-S3.

FAN is retained but inactive: backend routes and code remain, firmware remnants remain, but the FAN screen is not in active navigation and is not polled.

## Firmware configuration facts

- Build environments: `pulsmon-esp32s3-display` and `pulsmon-esp32s3-display-dev`.
- Backend address: compile-time settings in `esp/src/pulsemon_api_config.h`.
- NVS: Wi-Fi, weather and GNews settings only; no backend address or backend API key.
- OpenWeather currently uses plain HTTP and is a known defect.
- GNews uses HTTPS and `X-Api-Key`.

## EEZ rules

- Never edit `esp/src/ui/` directly.
- Never edit `esp/eez/pulsmon/` outside EEZ Studio.
- Make runtime integration changes only in the owning non-generated modules.

## Snapshot rules

`make-a.sh` intentionally creates a filtered `PulseMon.zip`. Excluded directories such as `tmp/`, build trees, caches and local SDK files may exist outside the snapshot. Do not classify an excluded file as missing without an explicit requirement that it belong to the snapshot.

## Validation rules

Backend:

```bash
cd api
python3 -m pytest -q
```

Firmware build, flash or monitor must not be run without explicit user instruction. When requested, use the environment defined in `esp/platformio.ini`.

Documentation changes must keep English and French aligned and must describe existing behavior rather than intended future behavior.

Do not create commits, push changes or access remote repositories without explicit instruction.
