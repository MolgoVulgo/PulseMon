from __future__ import annotations

import json
import logging
import os
from dataclasses import dataclass
from pathlib import Path
import socket
import time

from app.collectors import FanChannelReading, list_fan_channels
from app.models import (
    FanChannelInfo,
    FanItem,
    FanMappingInfo,
    FansDashboardResponse,
    FansMetaResponse,
)


logger = logging.getLogger(__name__)
_DEFAULT_MAPPING_FILE = str(Path.home() / ".config" / "pulsemon" / "fans_mapping.json")
_REPO_MAPPING_FALLBACK_FILE = Path(__file__).resolve().parents[2] / "config" / "fans_mapping.json"
_TMP_MAPPING_FALLBACK_FILE = Path("/tmp/pulsemon/fans_mapping.json")
_DEFAULT_REFERENCE_SEED_FILE = Path(__file__).resolve().parents[3] / "tmp" / "fan_reference_seed.json"
ALLOWED_FAN_ROLES = ("cpu", "case", "pump", "gpu", "radiator", "unknown")


@dataclass(frozen=True)
class _MatchRule:
    hwmon_name: str | None = None
    channel: str | None = None
    group: str | None = None
    hwmon_path_contains: str | None = None


@dataclass(frozen=True)
class _FanMapping:
    label: str
    role: str
    order: int
    enabled: bool
    rpm_min: int | None
    rpm_max: int | None
    match: _MatchRule


def get_fans_mapping_config() -> dict[str, object]:
    channels = list_fan_channels()
    path = _resolve_mapping_path()
    if not path.exists():
        _bootstrap_mapping_file(path, channels)
        logger.info("fans mapping file bootstrapped path=%s entries=%s", path, len(channels))

    raw = _read_mapping_json(path)
    if raw is None:
        return {"mapping_path": str(path), "allowed_roles": list(ALLOWED_FAN_ROLES), "mappings": []}
    return {
        "mapping_path": str(path),
        "allowed_roles": list(ALLOWED_FAN_ROLES),
        "mappings": raw.get("mappings", []),
    }


def save_fans_mapping_config(payload: dict[str, object]) -> dict[str, object]:
    path = _resolve_mapping_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return {
        "mapping_path": str(path),
        "allowed_roles": list(ALLOWED_FAN_ROLES),
        "mappings": payload.get("mappings", []),
    }


def get_fans_reference_catalog() -> dict[str, object]:
    path = _resolve_reference_seed_path()
    if not path.exists():
        logger.warning("fans reference seed missing path=%s", path)
        return {"generated_at": None, "items": []}

    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        logger.warning("fans reference seed unreadable path=%s error=%s:%s", path, type(exc).__name__, exc)
        return {"generated_at": None, "items": []}

    generated_at = raw.get("generated_at") if isinstance(raw.get("generated_at"), str) else None
    data = raw.get("data")
    if not isinstance(data, dict):
        return {"generated_at": generated_at, "items": []}

    items: list[dict[str, object]] = []
    for brand, by_series in data.items():
        if not isinstance(brand, str) or not isinstance(by_series, dict):
            continue
        for series, models in by_series.items():
            if not isinstance(series, str) or not isinstance(models, list):
                continue
            for model_row in models:
                if not isinstance(model_row, dict):
                    continue
                model = _as_text(model_row.get("model"))
                if not model:
                    continue
                ref_id = f"{brand}|{series}|{model}"
                items.append(
                    {
                        "id": ref_id,
                        "brand": brand,
                        "series": series,
                        "model": model,
                        "rpm_min": _as_nullable_int(model_row.get("rpm_min")),
                        "rpm_max": _as_nullable_int(model_row.get("rpm_max")),
                        "pwm": model_row.get("pwm") if isinstance(model_row.get("pwm"), bool) else None,
                        "connector": _as_text(model_row.get("connector")),
                        "size_mm": _as_nullable_int(model_row.get("size_mm")),
                    }
                )

    items_sorted = sorted(items, key=lambda row: (str(row["brand"]).lower(), str(row["series"]).lower(), str(row["model"]).lower()))
    return {"generated_at": generated_at, "items": items_sorted}


def build_fans_dashboard() -> FansDashboardResponse:
    channels = list_fan_channels()
    mappings = _load_mappings(channels)

    fans: list[tuple[int, FanItem]] = []
    fallback_mode = len(mappings) == 0
    for channel in channels:
        mapping = _resolve_mapping(channel, mappings)
        if fallback_mode:
            if not _is_active(channel):
                continue
            fans.append(
                (
                    1000,
                    FanItem(
                        label=channel.label or channel.channel,
                        role="gpu" if channel.group == "gpu" else "unknown",
                        rpm=channel.rpm,
                        pwm_pct=channel.pwm_pct,
                        pct_fans=None,
                    ),
                )
            )
            continue
        if mapping is None or not mapping.enabled or not _is_active(channel):
            continue
        fans.append(
            (
                mapping.order,
                FanItem(
                    label=mapping.label,
                    role=mapping.role,
                    rpm=channel.rpm,
                    pwm_pct=channel.pwm_pct,
                    pct_fans=_compute_pct_fans(channel.rpm, mapping.rpm_min, mapping.rpm_max),
                ),
            )
        )

    sorted_fans = [item for _, item in sorted(fans, key=lambda pair: (pair[0], pair[1].role, pair[1].label))]
    return FansDashboardResponse(
        v=1,
        ts=int(time.time()),
        host=socket.gethostname(),
        fans=sorted_fans,
    )

def _is_active(channel: FanChannelReading) -> bool:
    return channel.valid and channel.rpm is not None and channel.rpm > 0


def build_fans_meta() -> FansMetaResponse:
    channels = list_fan_channels()
    mappings = _load_mappings(channels)

    channels_payload: list[FanChannelInfo] = []
    display_labels: set[str] = set()

    for channel in channels:
        mapping = _resolve_mapping(channel, mappings)
        mapping_info = FanMappingInfo(
            configured=mapping is not None,
            label=mapping.label if mapping else None,
            role=mapping.role if mapping else None,
            order=mapping.order if mapping else None,
            enabled=mapping.enabled if mapping else None,
        )
        if mapping and mapping.enabled and channel.valid:
            display_labels.add(mapping.label)
        channels_payload.append(
            FanChannelInfo(
                channel=channel.channel,
                hwmon_name=channel.hwmon_name,
                hwmon_path=channel.hwmon_path,
                source=channel.source,
                group=channel.group,
                label=channel.label,
                rpm=channel.rpm,
                pwm_pct=channel.pwm_pct,
                connected=channel.connected,
                valid=channel.valid,
                error=channel.error,
                mapping=mapping_info,
            )
        )

    return FansMetaResponse(
        v=1,
        ts=int(time.time()),
        host=socket.gethostname(),
        channels=channels_payload,
        display_labels=sorted(display_labels),
    )


def _load_mappings(channels: list[FanChannelReading]) -> list[_FanMapping]:
    path = _resolve_mapping_path()
    if not path.exists():
        _bootstrap_mapping_file(path, channels)
        logger.info("fans mapping file bootstrapped path=%s entries=%s", path, len(channels))

    raw = _read_mapping_json(path)
    if raw is None:
        return []

    rows = raw.get("mappings")
    if not isinstance(rows, list):
        logger.warning("fans mapping invalid format path=%s", path)
        return []

    mappings: list[_FanMapping] = []
    for row in rows:
        if not isinstance(row, dict):
            continue
        label = _as_text(row.get("label"))
        role = _as_text(row.get("role"))
        if not label or not role:
            continue
        if role not in ALLOWED_FAN_ROLES:
            role = "unknown"
        order_value = row.get("order", 100)
        enabled_value = row.get("enabled", True)
        match = row.get("match", {})
        if not isinstance(match, dict):
            match = {}
        mappings.append(
            _FanMapping(
                label=label,
                role=role,
                order=_as_int(order_value, default=100),
                enabled=bool(enabled_value),
                rpm_min=_as_nullable_int(row.get("rpm_min")),
                rpm_max=_as_nullable_int(row.get("rpm_max")),
                match=_MatchRule(
                    hwmon_name=_as_text(match.get("hwmon_name")),
                    channel=_as_text(match.get("channel")),
                    group=_as_text(match.get("group")),
                    hwmon_path_contains=_as_text(match.get("hwmon_path_contains")),
                ),
            )
        )

    return mappings


def _bootstrap_mapping_file(path: Path, channels: list[FanChannelReading]) -> None:
    template: dict[str, object] = {"mappings": []}
    mappings_rows: list[dict[str, object]] = []
    for idx, channel in enumerate(channels):
        role = "gpu" if channel.group == "gpu" else "unknown"
        mappings_rows.append(
            {
                "label": channel.label or channel.channel,
                "role": role,
                "order": 100 + idx,
                "enabled": _is_active(channel),
                "rpm_min": None,
                "rpm_max": None,
                "match": {
                    "hwmon_name": channel.hwmon_name,
                    "channel": channel.channel,
                    "group": channel.group,
                },
            }
        )
    template["mappings"] = mappings_rows

    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(template, indent=2) + "\n", encoding="utf-8")
    except OSError as exc:
        logger.warning("failed to bootstrap fans mapping file path=%s error=%s:%s", path, type(exc).__name__, exc)


def _resolve_mapping_path() -> Path:
    configured = os.getenv("STATS_FANS_MAPPING_FILE")
    if configured:
        return Path(configured)
    candidates = [Path(_DEFAULT_MAPPING_FILE), _REPO_MAPPING_FALLBACK_FILE, _TMP_MAPPING_FALLBACK_FILE]

    for candidate in candidates:
        if candidate.exists():
            return candidate

    for candidate in candidates:
        if _ensure_parent_writable(candidate):
            if candidate != Path(_DEFAULT_MAPPING_FILE):
                logger.warning("fans mapping default path not writable, fallback_path=%s", candidate)
            return candidate

    return Path(_DEFAULT_MAPPING_FILE)


def _ensure_parent_writable(path: Path) -> bool:
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
    except OSError:
        return False
    return os.access(path.parent, os.W_OK)


def _resolve_reference_seed_path() -> Path:
    configured = os.getenv("STATS_FANS_REFERENCE_SEED_FILE")
    if configured:
        return Path(configured)
    return _DEFAULT_REFERENCE_SEED_FILE


def _read_mapping_json(path: Path) -> dict[str, object] | None:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        logger.warning("fans mapping file unreadable path=%s error=%s:%s", path, type(exc).__name__, exc)
        return None
    if not isinstance(raw, dict):
        logger.warning("fans mapping invalid root type path=%s", path)
        return None
    return raw


def _resolve_mapping(channel: FanChannelReading, mappings: list[_FanMapping]) -> _FanMapping | None:
    for mapping in mappings:
        if _mapping_matches(mapping, channel):
            return mapping
    return None


def _mapping_matches(mapping: _FanMapping, channel: FanChannelReading) -> bool:
    rule = mapping.match
    if rule.hwmon_name and channel.hwmon_name.lower() != rule.hwmon_name.lower():
        return False
    if rule.channel and channel.channel.lower() != rule.channel.lower():
        return False
    if rule.group and channel.group.lower() != rule.group.lower():
        return False
    if rule.hwmon_path_contains and rule.hwmon_path_contains not in channel.hwmon_path:
        return False
    return True


def _as_text(value: object) -> str | None:
    return value.strip() if isinstance(value, str) and value.strip() else None


def _as_int(value: object, *, default: int) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def _as_nullable_int(value: object) -> int | None:
    try:
        return int(value) if value is not None else None
    except (TypeError, ValueError):
        return None


def _compute_pct_fans(rpm: int | None, rpm_min: int | None, rpm_max: int | None) -> int | None:
    if rpm is None or rpm_max is None:
        return None
    min_value = 0 if rpm_min is None else rpm_min
    if rpm_max <= min_value:
        return None
    pct = round((rpm - min_value) * 100.0 / (rpm_max - min_value))
    return max(0, min(100, int(pct)))
