from __future__ import annotations

from app.models import MetricValue


DEFAULT_TS_MS = 1774256402000


def metric(
    value_raw: float | int | None,
    *,
    unit: str,
    source: str = "test-source",
    value_display: float | int | None = None,
    sampled_at: int = DEFAULT_TS_MS,
    estimated: bool = False,
    valid: bool | None = None,
) -> MetricValue:
    if valid is None:
        valid = value_raw is not None
    if value_display is None and valid:
        value_display = value_raw

    return MetricValue(
        value_raw=value_raw,
        value_display=value_display,
        source=source,
        unit=unit,
        sampled_at=sampled_at,
        estimated=estimated,
        valid=valid,
    )
