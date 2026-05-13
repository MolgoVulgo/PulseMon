# PulseMon documentation

This directory is the canonical documentation root for the project.

English is the default language. French documentation is stored in `/docs/fr`.

## Index

- `overview.md` — project purpose, scope and runtime model.
- `architecture.md` — backend, firmware, transport and data ownership.
- `api.md` — HTTP API contract and payloads.
- `backend.md` — Linux backend implementation behavior.
- `firmware.md` — ESP32-S3 firmware behavior and screen logic.
- `configuration.md` — backend, ESP32, weather and news configuration.
- `gpu.md` — AMD GPU telemetry and smoothing behavior.
- `fans.md` — fan monitoring, mapping and configuration.
- `weather-news.md` — autonomous weather and GNews modules.
- `web-configuration.md` — ESP32 captive portal and configuration API.
- `development.md` — install, build, tests and contribution workflow.
- `troubleshooting.md` — diagnostics, common faults and validation paths.

## Documentation rules

- Keep behavior documentation in `/docs`.
- Keep French translations in `/docs/fr`.
- Keep root-level README files short and operational.
- Keep API field names and endpoint paths exact.
- Do not duplicate secrets, API keys or private local values.
