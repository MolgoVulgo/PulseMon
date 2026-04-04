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
_DEFAULT_MAPPING_FILE = str(Path(__file__).resolve().parents[2] / "config" / "fans_mapping.json")


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
    match: _MatchRule


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
    path = Path(os.getenv("STATS_FANS_MAPPING_FILE", _DEFAULT_MAPPING_FILE))
    if not path.exists():
        _bootstrap_mapping_file(path, channels)
        logger.info("fans mapping file bootstrapped path=%s entries=%s", path, len(channels))

    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        logger.warning("fans mapping file unreadable path=%s error=%s:%s", path, type(exc).__name__, exc)
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
