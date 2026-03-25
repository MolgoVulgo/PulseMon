import os
from dataclasses import dataclass


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



def _as_bool(value: str | None, default: bool = False) -> bool:
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "on"}



def load_config() -> AppConfig:
    api_key_raw = os.getenv("STATS_API_KEY")
    api_key = api_key_raw if api_key_raw else None

    return AppConfig(
        bind_host=os.getenv("STATS_BIND_HOST", "0.0.0.0"),
        bind_port=int(os.getenv("STATS_BIND_PORT", "8000")),
        sample_interval_s=float(os.getenv("STATS_SAMPLE_INTERVAL_S", "0.1")),
        publish_interval_s=float(os.getenv("STATS_PUBLISH_INTERVAL_S", "0.5")),
        history_capacity=int(os.getenv("STATS_HISTORY_CAPACITY", "600")),
        api_key=api_key,
        api_key_header=os.getenv("STATS_API_KEY_HEADER", "X-API-Key"),
        log_level=os.getenv("STATS_LOG_LEVEL", "INFO").upper(),
        diagnostics_enabled=_as_bool(os.getenv("STATS_DIAGNOSTICS", "0"), default=False),
        diagnostics_raw_capture=_as_bool(os.getenv("STATS_DIAG_RAW_CAPTURE", "0"), default=False),
        diagnostics_raw_hz=float(os.getenv("STATS_DIAG_RAW_HZ", "8.0")),
        diagnostics_raw_duration_s=int(os.getenv("STATS_DIAG_RAW_DURATION_S", "60")),
        diagnostics_raw_log_path=os.getenv("STATS_DIAG_RAW_LOG_PATH", "api/diagnostics/raw_metrics.jsonl"),
        diagnostics_compare_capture=_as_bool(os.getenv("STATS_DIAG_COMPARE_CAPTURE", "0"), default=False),
        diagnostics_compare_hz=float(os.getenv("STATS_DIAG_COMPARE_HZ", "10.0")),
        diagnostics_compare_duration_s=int(os.getenv("STATS_DIAG_COMPARE_DURATION_S", "60")),
        diagnostics_compare_log_path=os.getenv(
            "STATS_DIAG_COMPARE_LOG_PATH",
            "api/diagnostics/raw_vs_display_gpu_pct.jsonl",
        ),
        display_ema_alpha=float(os.getenv("STATS_DISPLAY_EMA_ALPHA", "0.25")),
    )
