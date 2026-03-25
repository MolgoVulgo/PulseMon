from __future__ import annotations

import socket
import threading
import time

from app.collectors import (
    MetricReading,
    read_cpu_percent_metric,
    read_cpu_power_metric,
    read_cpu_temp_metric,
    read_gpu_percent_metric,
    read_gpu_power_metric,
    read_gpu_temp_metric,
    read_memory,
)
from app.models import (
    CpuSnapshot,
    DashboardResponse,
    GpuSnapshot,
    MemSnapshot,
    MetricValue,
    SnapshotState,
)
from app.services.filters import MedianEmaFilter
from app.store import SnapshotStore


class SnapshotUnavailableError(RuntimeError):
    pass


_FILTER_LOCK = threading.Lock()
_FILTER_ALPHA: float | None = None
_FILTERS: dict[str, MedianEmaFilter] = {}


def build_dashboard_live(*, display_ema_alpha: float = 0.25) -> DashboardResponse:
    _ensure_filter_config(display_ema_alpha=display_ema_alpha)

    cpu_pct = read_cpu_percent_metric()
    cpu_temp_c = read_cpu_temp_metric()
    cpu_power_w = read_cpu_power_metric()

    mem_used_b, mem_total_b, mem_pct = read_memory()
    mem_used_metric = _memory_metric(value=mem_used_b, unit="bytes")
    mem_total_metric = _memory_metric(value=mem_total_b, unit="bytes")
    mem_pct_metric = _memory_metric(value=mem_pct, unit="percent")

    gpu_pct = read_gpu_percent_metric()
    gpu_temp_c = read_gpu_temp_metric()
    gpu_power_w = read_gpu_power_metric()

    return DashboardResponse(
        v=1,
        ts=int(time.time()),
        host=socket.gethostname(),
        cpu=CpuSnapshot(
            pct=_to_metric_value(key="cpu_pct", reading=cpu_pct, use_filter=True),
            temp_c=_to_metric_value(key="cpu_temp_c", reading=cpu_temp_c, use_filter=False),
            power_w=_to_metric_value(key="cpu_power_w", reading=cpu_power_w, use_filter=False),
        ),
        mem=MemSnapshot(
            used_b=_to_metric_value(key="mem_used_b", reading=mem_used_metric, use_filter=False),
            total_b=_to_metric_value(key="mem_total_b", reading=mem_total_metric, use_filter=False),
            pct=_to_metric_value(key="mem_pct", reading=mem_pct_metric, use_filter=False),
        ),
        gpu=GpuSnapshot(
            pct=_to_metric_value(key="gpu_pct", reading=gpu_pct, use_filter=True),
            temp_c=_to_metric_value(key="gpu_temp_c", reading=gpu_temp_c, use_filter=False),
            power_w=_to_metric_value(key="gpu_power_w", reading=gpu_power_w, use_filter=False),
        ),
        state=SnapshotState(
            ok=True,
            stale_ms=0,
        ),
    )



def build_dashboard_from_store(snapshot_store: SnapshotStore) -> DashboardResponse:
    snapshot = snapshot_store.get_snapshot()
    if snapshot is None:
        raise SnapshotUnavailableError("snapshot_unavailable")

    stale_ms = snapshot_store.get_stale_ms()
    snapshot.state.ok = True
    snapshot.state.stale_ms = stale_ms
    return snapshot



def _memory_metric(*, value: float | int, unit: str) -> MetricReading:
    now_ms = int(time.time() * 1000)
    now_mono = time.monotonic()
    return MetricReading(
        raw_value=value,
        source="psutil.virtual_memory()",
        unit=unit,
        sampled_at_unix_ms=now_ms,
        sampled_at_monotonic_s=now_mono,
        read_ok=True,
        read_error=None,
        estimated=False,
    )



def _to_metric_value(*, key: str, reading: MetricReading, use_filter: bool) -> MetricValue:
    display: float | int | None = None
    if reading.valid and reading.raw_value is not None:
        if use_filter:
            display = _update_filter(key=key, value=float(reading.raw_value), valid=True)
        else:
            display = reading.raw_value
    elif use_filter:
        display = _update_filter(key=key, value=None, valid=False)

    return MetricValue(
        value_raw=reading.raw_value,
        value_display=display,
        source=reading.source,
        unit=reading.unit,
        sampled_at=reading.sampled_at_unix_ms,
        estimated=reading.estimated,
        valid=reading.valid,
    )



def _ensure_filter_config(*, display_ema_alpha: float) -> None:
    global _FILTER_ALPHA, _FILTERS

    alpha = round(display_ema_alpha, 6)
    with _FILTER_LOCK:
        if _FILTER_ALPHA == alpha:
            return

        _FILTER_ALPHA = alpha
        _FILTERS = {
            "cpu_pct": MedianEmaFilter(alpha=alpha, median_window=3),
            "gpu_pct": MedianEmaFilter(alpha=alpha, median_window=3),
        }



def _update_filter(*, key: str, value: float | None, valid: bool) -> float | None:
    with _FILTER_LOCK:
        filt = _FILTERS.get(key)
        if filt is None:
            filt = MedianEmaFilter(alpha=0.25, median_window=3)
            _FILTERS[key] = filt
        return filt.update(value, valid=valid)
