import os
from dataclasses import dataclass


@dataclass(frozen=True)
class AppConfig:
    bind_host: str = "0.0.0.0"
    bind_port: int = 8000
    sample_interval_s: float = 1.0
    history_capacity: int = 600
    api_key: str | None = None
    api_key_header: str = "X-API-Key"
    log_level: str = "INFO"
    diagnostics_enabled: bool = False



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
        sample_interval_s=float(os.getenv("STATS_SAMPLE_INTERVAL_S", "1.0")),
        history_capacity=int(os.getenv("STATS_HISTORY_CAPACITY", "600")),
        api_key=api_key,
        api_key_header=os.getenv("STATS_API_KEY_HEADER", "X-API-Key"),
        log_level=os.getenv("STATS_LOG_LEVEL", "INFO").upper(),
        diagnostics_enabled=_as_bool(os.getenv("STATS_DIAGNOSTICS", "0"), default=False),
    )
