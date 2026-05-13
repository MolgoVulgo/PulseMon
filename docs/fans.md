# Fan monitoring and configuration

PulseMon supports fan telemetry and configurable fan mapping. The backend exposes runtime values and local configuration endpoints for the admin UI.

## Runtime behavior

Fan telemetry can include RPM values and computed percentage values. The computed `pct_fans` value is based on configured `rpm_min` and `rpm_max` when the configuration is valid.

If configuration is missing or invalid, computed percentages must be nullable rather than fabricated.

## Mapping

Fan mapping defines how detected hardware channels are associated with logical fans.

A mapping entry should contain enough information to identify the fan channel, define display metadata, and calculate percentage values from RPM bounds.

When the configuration database is empty, the backend can bootstrap an initial mapping from detected channels or import a legacy JSON mapping if configured.

## API endpoints

- `GET /api/v1/fans/dashboard` — current fan data;
- `GET /api/v1/fans/meta` — detected capabilities and metadata;
- `GET /api/v1/fans/config` — current mapping;
- `PUT /api/v1/fans/config` — update mapping;
- `GET /api/v1/fans/reference` — reference data for UI selection.

Local database administration endpoints can be exposed for the backend admin UI and should not be treated as firmware display dependencies.

## UI behavior

The backend UI can provide fan configuration and reference selection. Polling must not reset active selections in forms.

Firmware fan pages may exist in generated UI sources. Runtime navigation and polling should only expose fan screens that are actually wired to valid data.

## Fan dashboard payload

`GET /api/v1/fans/dashboard` returns only mapped, valid and enabled fans.

Each fan item can contain:

- `label`;
- `role`;
- `rpm`;
- `pwm_pct`;
- `pct_fans`.

`pct_fans` is computed when `rpm` and `rpm_max` are available. If `rpm_min` is absent, the backend uses `0` as effective minimum.

Formula:

```text
pct_fans = clamp(round((rpm - rpm_min_effective) * 100 / (rpm_max - rpm_min_effective)), 0, 100)
```

A detected channel with `rpm = 0` is treated as off and is not shown in the dashboard. During bootstrap, an off channel is stored with `enabled=false`.

## Fan metadata payload

`GET /api/v1/fans/meta` returns technical channels with:

- `channel`;
- `hwmon_name`;
- `hwmon_path`;
- `source`;
- `group`;
- `label`;
- `rpm`;
- `pwm_pct`;
- `connected`;
- `valid`;
- `error`;
- `mapping` fields such as `configured`, `label`, `role`, `order` and `enabled`.

## Fan reference model

The fan reference catalog contains flattened items with:

- `id`, usually built from brand, series and model;
- `brand`;
- `series`;
- `model`;
- `rpm_min`;
- `rpm_max`;
- `pwm`;
- `connector`;
- `size_mm`.
