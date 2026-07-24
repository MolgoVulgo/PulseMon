# Retained FAN subsystem

FAN development has been abandoned as an active PulseMon feature, but the implementation has not been fully removed.

## Current status

Backend code still provides:

- hwmon fan discovery;
- SQLite fan mappings;
- mapping import/bootstrap behavior;
- fan dashboard, metadata, configuration and reference endpoints;
- local database administration routes and backend UI support.

Firmware code still contains:

- fan payload structures and parser functions;
- generated FAN screen objects;
- runtime variables and dormant helper code.

The active firmware behavior excludes FAN:

- `actions.c` maps `SCREEN_ID_FAN` to no target;
- active navigation is Main, GPU and Weather only;
- `pulsemon_poller.c` does not call `pulsemon_fetch_fans_dashboard()`.

## Maintenance rule

Do not delete or repair the retained subsystem opportunistically. Do not document it as an active user feature. Any reactivation, final removal or data migration requires an explicit task with backend, firmware, EEZ and documentation scope.

## Retained API routes

- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /api/v1/fans/config`
- `PUT /api/v1/fans/config`
- `GET /api/v1/fans/reference`
- `GET /api/v1/db/fans`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`
