from __future__ import annotations

from dataclasses import dataclass
import ipaddress
import math
import os
from pathlib import Path
import re
import sys


BACKEND_ENV_VARS: tuple[str, ...] = (
    "STATS_BIND_HOST",
    "STATS_BIND_PORT",
    "STATS_SAMPLE_INTERVAL_S",
    "STATS_PUBLISH_INTERVAL_S",
    "STATS_HISTORY_CAPACITY",
    "STATS_API_KEY",
    "STATS_API_KEY_HEADER",
    "STATS_LOG_LEVEL",
    "STATS_DIAGNOSTICS",
    "STATS_DIAG_RAW_CAPTURE",
    "STATS_DIAG_RAW_HZ",
    "STATS_DIAG_RAW_DURATION_S",
    "STATS_DIAG_RAW_LOG_PATH",
    "STATS_DIAG_COMPARE_CAPTURE",
    "STATS_DIAG_COMPARE_HZ",
    "STATS_DIAG_COMPARE_DURATION_S",
    "STATS_DIAG_COMPARE_LOG_PATH",
    "STATS_DISPLAY_EMA_ALPHA",
    "STATS_GPU_PCI_SLOT",
    "STATS_GPU_TEMP_LABEL_PRIORITY",
)

DEFAULT_GPU_TEMP_LABEL_PRIORITY: tuple[str, ...] = ("edge", "junction", "mem", "unknown")

_BOOL_TRUE = frozenset({"1", "true", "yes", "on"})
_BOOL_FALSE = frozenset({"0", "false", "no", "off"})
_HTTP_TOKEN_RE = re.compile(r"^[!#$%&'*+\-.^_`|~0-9A-Za-z]+$")
_HOST_LABEL_RE = re.compile(r"^[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?$")
_PCI_SLOT_RE = re.compile(r"^(?:[0-9A-Fa-f]{4}:)?[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}\.[0-7]$")
_GPU_TEMP_LABEL_RE = re.compile(r"^[a-z0-9][a-z0-9_-]{0,31}$")
_LOG_LEVEL_ALIASES = {"WARN": "WARNING", "FATAL": "CRITICAL"}
_LOG_LEVELS = frozenset({"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG"})


class ConfigError(ValueError):
    """Raised when a backend environment variable violates the runtime contract."""


@dataclass(frozen=True)
class AppConfig:
    bind_host: str = "0.0.0.0"
    bind_port: int = 8000
    sample_interval_s: float = 0.1
    publish_interval_s: float = 0.5
    history_capacity: int = 600
    api_key: str | None = None
    api_key_header: str = "X-API-Key"
    log_level: str = "INFO"
    diagnostics_enabled: bool = False
    diagnostics_raw_capture: bool = False
    diagnostics_raw_hz: float = 8.0
    diagnostics_raw_duration_s: int = 60
    diagnostics_raw_log_path: str = "api/diagnostics/raw_metrics.jsonl"
    diagnostics_compare_capture: bool = False
    diagnostics_compare_hz: float = 10.0
    diagnostics_compare_duration_s: int = 60
    diagnostics_compare_log_path: str = "api/diagnostics/raw_vs_display_gpu_pct.jsonl"
    display_ema_alpha: float = 0.25
    gpu_pci_slot: str | None = None
    gpu_temp_label_priority: tuple[str, ...] = DEFAULT_GPU_TEMP_LABEL_PRIORITY


def load_config() -> AppConfig:
    return AppConfig(
        bind_host=_parse_bind_host("STATS_BIND_HOST", "0.0.0.0"),
        bind_port=_parse_int("STATS_BIND_PORT", 8000, minimum=1, maximum=65_535),
        sample_interval_s=_parse_float(
            "STATS_SAMPLE_INTERVAL_S",
            0.1,
            minimum=0.01,
            maximum=60.0,
        ),
        publish_interval_s=_parse_float(
            "STATS_PUBLISH_INTERVAL_S",
            0.5,
            minimum=0.01,
            maximum=3_600.0,
        ),
        history_capacity=_parse_int(
            "STATS_HISTORY_CAPACITY",
            600,
            minimum=1,
            maximum=1_000_000,
        ),
        api_key=_parse_api_key("STATS_API_KEY"),
        api_key_header=_parse_http_token("STATS_API_KEY_HEADER", "X-API-Key"),
        log_level=_parse_log_level("STATS_LOG_LEVEL", "INFO"),
        diagnostics_enabled=_parse_bool("STATS_DIAGNOSTICS", False),
        diagnostics_raw_capture=_parse_bool("STATS_DIAG_RAW_CAPTURE", False),
        diagnostics_raw_hz=_parse_float(
            "STATS_DIAG_RAW_HZ",
            8.0,
            minimum=0.1,
            maximum=1_000.0,
        ),
        diagnostics_raw_duration_s=_parse_int(
            "STATS_DIAG_RAW_DURATION_S",
            60,
            minimum=1,
            maximum=86_400,
        ),
        diagnostics_raw_log_path=_parse_path(
            "STATS_DIAG_RAW_LOG_PATH",
            "api/diagnostics/raw_metrics.jsonl",
        ),
        diagnostics_compare_capture=_parse_bool("STATS_DIAG_COMPARE_CAPTURE", False),
        diagnostics_compare_hz=_parse_float(
            "STATS_DIAG_COMPARE_HZ",
            10.0,
            minimum=0.1,
            maximum=1_000.0,
        ),
        diagnostics_compare_duration_s=_parse_int(
            "STATS_DIAG_COMPARE_DURATION_S",
            60,
            minimum=1,
            maximum=86_400,
        ),
        diagnostics_compare_log_path=_parse_path(
            "STATS_DIAG_COMPARE_LOG_PATH",
            "api/diagnostics/raw_vs_display_gpu_pct.jsonl",
        ),
        display_ema_alpha=_parse_float(
            "STATS_DISPLAY_EMA_ALPHA",
            0.25,
            minimum=0.0,
            maximum=1.0,
            minimum_exclusive=True,
        ),
        gpu_pci_slot=_parse_pci_slot("STATS_GPU_PCI_SLOT"),
        gpu_temp_label_priority=_parse_gpu_temp_label_priority(
            "STATS_GPU_TEMP_LABEL_PRIORITY",
            DEFAULT_GPU_TEMP_LABEL_PRIORITY,
        ),
    )


def _raw_env(name: str, default: str | None = None) -> str | None:
    value = os.getenv(name)
    return default if value is None else value


def _parse_int(name: str, default: int, *, minimum: int, maximum: int) -> int:
    raw = _raw_env(name, str(default))
    assert raw is not None
    try:
        value = int(raw, 10)
    except ValueError as exc:
        raise ConfigError(f"{name} must be an integer in [{minimum}, {maximum}]") from exc
    if value < minimum or value > maximum:
        raise ConfigError(f"{name} must be in [{minimum}, {maximum}]")
    return value


def _parse_float(
    name: str,
    default: float,
    *,
    minimum: float,
    maximum: float,
    minimum_exclusive: bool = False,
) -> float:
    raw = _raw_env(name, str(default))
    assert raw is not None
    try:
        value = float(raw)
    except ValueError as exc:
        raise ConfigError(f"{name} must be a finite number") from exc
    if not math.isfinite(value):
        raise ConfigError(f"{name} must be a finite number")
    minimum_invalid = value <= minimum if minimum_exclusive else value < minimum
    if minimum_invalid or value > maximum:
        left = "(" if minimum_exclusive else "["
        raise ConfigError(f"{name} must be in {left}{minimum}, {maximum}]")
    return value


def _parse_bool(name: str, default: bool) -> bool:
    raw = _raw_env(name)
    if raw is None:
        return default
    normalized = raw.strip().lower()
    if normalized in _BOOL_TRUE:
        return True
    if normalized in _BOOL_FALSE:
        return False
    raise ConfigError(f"{name} must be one of 1/0, true/false, yes/no or on/off")


def _parse_bind_host(name: str, default: str) -> str:
    raw = _raw_env(name, default)
    assert raw is not None
    if raw != raw.strip() or not raw or len(raw) > 253 or any(char.isspace() for char in raw):
        raise ConfigError(f"{name} must be a valid IP address or hostname")

    try:
        ipaddress.ip_address(raw)
        return raw
    except ValueError:
        pass

    hostname = raw[:-1] if raw.endswith(".") else raw
    labels = hostname.split(".")
    if not hostname or any(not _HOST_LABEL_RE.fullmatch(label) for label in labels):
        raise ConfigError(f"{name} must be a valid IP address or hostname")
    return raw


def _parse_api_key(name: str) -> str | None:
    raw = _raw_env(name)
    if raw is None or raw == "":
        return None
    if raw != raw.strip() or len(raw) > 4_096 or any(ord(char) < 32 or ord(char) == 127 for char in raw):
        raise ConfigError(f"{name} contains invalid surrounding whitespace or control characters")
    return raw


def _parse_http_token(name: str, default: str) -> str:
    raw = _raw_env(name, default)
    assert raw is not None
    if len(raw) > 128 or not _HTTP_TOKEN_RE.fullmatch(raw):
        raise ConfigError(f"{name} must be a valid HTTP header token")
    return raw


def _parse_log_level(name: str, default: str) -> str:
    raw = _raw_env(name, default)
    assert raw is not None
    normalized = raw.strip().upper()
    normalized = _LOG_LEVEL_ALIASES.get(normalized, normalized)
    if normalized not in _LOG_LEVELS:
        allowed = ", ".join(sorted(_LOG_LEVELS))
        raise ConfigError(f"{name} must be one of {allowed}")
    return normalized


def _parse_path(name: str, default: str) -> str:
    raw = _raw_env(name, default)
    assert raw is not None
    if raw != raw.strip() or not raw or len(raw) > 4_096 or any(ord(char) < 32 or ord(char) == 127 for char in raw):
        raise ConfigError(f"{name} must be a non-empty filesystem path without control characters")
    try:
        Path(raw)
    except (TypeError, ValueError) as exc:
        raise ConfigError(f"{name} must be a valid filesystem path") from exc
    return raw


def _parse_pci_slot(name: str) -> str | None:
    raw = _raw_env(name)
    if raw is None or raw == "":
        return None
    if raw != raw.strip() or not _PCI_SLOT_RE.fullmatch(raw):
        raise ConfigError(f"{name} must use PCI BDF form dddd:bb:ss.f or bb:ss.f")
    return raw.lower()


def _parse_gpu_temp_label_priority(
    name: str,
    default: tuple[str, ...],
) -> tuple[str, ...]:
    raw = _raw_env(name)
    if raw is None:
        return default
    if not raw.strip():
        raise ConfigError(f"{name} must contain at least one comma-separated label")

    parts = raw.split(",")
    labels: list[str] = []
    seen: set[str] = set()
    for part in parts:
        label = part.strip().lower()
        if not label or not _GPU_TEMP_LABEL_RE.fullmatch(label):
            raise ConfigError(f"{name} contains an invalid temperature label")
        if label in seen:
            raise ConfigError(f"{name} must not contain duplicate labels")
        labels.append(label)
        seen.add(label)

    if len(labels) > 32:
        raise ConfigError(f"{name} must contain at most 32 labels")
    if "unknown" not in seen:
        labels.append("unknown")
    return tuple(labels)


def main() -> int:
    try:
        load_config()
    except ConfigError as exc:
        print(f"PulseMon backend configuration error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":  # pragma: no cover - exercised by wrapper/preflight
    raise SystemExit(main())
