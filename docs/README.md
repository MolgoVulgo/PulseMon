# PulseMon documentation

This directory is the canonical documentation root for the current `PulseMon.zip` snapshot.

The implementation and configuration files in the snapshot are the source of truth. Documentation must describe the runtime that exists now; planned, dormant or abandoned behavior must be labelled explicitly.

English is the default language. The French mirror is stored in `docs/fr/`.

## Index

- `overview.md` — active scope, retained components and non-goals.
- `architecture.md` — backend/firmware ownership and data flows.
- `api.md` — current HTTP endpoints and payload contracts.
- `backend.md` — Linux backend runtime.
- `firmware.md` — active firmware screens, polling and EEZ ownership.
- `configuration.md` — backend environment, SQLite and firmware NVS/compile-time settings.
- `gpu.md` — AMD GPU telemetry.
- `fans.md` — retained but inactive FAN subsystem.
- `weather-news.md` — current OpenWeather and GNews implementations.
- `web-configuration.md` — local ESP32 configuration server.
- `development.md` — setup, tests, firmware environments and snapshot generation.
- `troubleshooting.md` — operational checks based on the current implementation.

## Documentation rules

- Keep English and French files aligned.
- Use exact endpoint names, environment variables and build environments.
- Do not present FAN as an active firmware feature.
- Do not state that the backend address or backend API key is stored in firmware NVS.
- Treat `esp/src/ui/` as generated output and `esp/eez/pulsmon/` as EEZ Studio-owned project data.
- Document known defects as current facts until code changes make them obsolete.
