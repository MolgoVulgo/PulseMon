# PulseMon documentation

This directory is the canonical documentation root for the current `PulseMon.zip` snapshot.

The implementation and configuration files in the snapshot are the source of truth. Documentation must describe the runtime that exists now. Historical, generated or planned behavior must be isolated and labelled explicitly.

English is the default language. The French mirror is stored in `docs/fr/`.

## Index

- `overview.md` — active scope and non-goals.
- `architecture.md` — backend/firmware ownership and data flows.
- `api.md` — current HTTP endpoints and payload contracts.
- `backend.md` — Linux backend runtime.
- `firmware.md` — active firmware screens, polling and EEZ ownership.
- `configuration.md` — backend environment, firmware NVS and compile-time settings.
- `gpu.md` — AMD GPU telemetry.
- `weather-news.md` — current OpenWeather and GNews implementations.
- `web-configuration.md` — local ESP32 configuration server.
- `development.md` — setup, tests, firmware environments and snapshot generation.
- `p8-validation.md` — reproducible build, upload, serial and hardware-validation workflow.
- `troubleshooting.md` — operational checks based on the current implementation.

## Canonical documentation rules

- Keep English and French files aligned.
- Use exact endpoint names, environment variables and build environments.
- Describe the backend as an in-memory metrics service with no persistent database.
- Treat GPU fan RPM and percentage as GPU telemetry, not as a generic FAN subsystem.
- Document the backend host/port as NVS-backed with compiled fallback; the backend API key remains neither stored nor sent.
- Document the configuration HTTP server and captive DNS as setup-AP-scoped services, not permanent LAN services.
- Treat `esp/src/ui/` as generated output and `esp/eez/pulsmon/` as EEZ Studio-owned project data.
- Mention the unreachable generated legacy screen only where the EEZ boundary or troubleshooting requires it.
- Document known limitations as current facts until code changes make them obsolete.
