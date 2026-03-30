from __future__ import annotations

import socket
import threading
import time

from app.collectors import (
    MetricReading,
    read_gpu_core_clock_metric,
    read_gpu_fan_percent_metric,
    read_gpu_fan_rpm_metric,
    read_gpu_mem_clock_metric,
    read_gpu_percent_metric,
    read_gpu_power_metric,
    read_gpu_temp_metric,
    read_gpu_vram_percent_metric,
    read_gpu_vram_total_metric,
    read_gpu_vram_used_metric,
)
from app.models import GpuDashboardResponse, GpuExtendedSnapshot, MetricValue, SnapshotState
from app.services.filters import MedianEmaFilter
from app.store import GpuSnapshotStore


class GpuSnapshotUnavailableError(RuntimeError):
    pass


_FILTER_LOCK = threading.Lock()
_GPU_PCT_FILTER = MedianEmaFilter(alpha=0.25, median_window=3)


def build_gpu_dashboard_live() -> GpuDashboardResponse:
    gpu_pct = read_gpu_percent_metric()
    core_clock = read_gpu_core_clock_metric()
    mem_clock = read_gpu_mem_clock_metric()
    vram_used = read_gpu_vram_used_metric()
    vram_total = read_gpu_vram_total_metric()
    vram_pct = read_gpu_vram_percent_metric()
    gpu_temp = read_gpu_temp_metric()
    gpu_power = read_gpu_power_metric()
    fan_rpm = read_gpu_fan_rpm_metric()
    fan_pct = read_gpu_fan_percent_metric()

    return GpuDashboardResponse(
        v=1,
        ts=int(time.time()),
        host=socket.gethostname(),
        gpu=GpuExtendedSnapshot(
            pct=_to_metric_value(gpu_pct, use_filter=True),
            core_clock_mhz=_to_metric_value(core_clock, use_filter=False),
            mem_clock_mhz=_to_metric_value(mem_clock, use_filter=False),
            vram_used_b=_to_metric_value(vram_used, use_filter=False),
            vram_total_b=_to_metric_value(vram_total, use_filter=False),
            vram_pct=_to_metric_value(vram_pct, use_filter=False),
            temp_c=_to_metric_value(gpu_temp, use_filter=False),
            power_w=_to_metric_value(gpu_power, use_filter=False),
            fan_rpm=_to_metric_value(fan_rpm, use_filter=False),
            fan_pct=_to_metric_value(fan_pct, use_filter=False),
        ),
        state=SnapshotState(ok=True, stale_ms=0),
    )


def build_gpu_dashboard_from_store(snapshot_store: GpuSnapshotStore) -> GpuDashboardResponse:
    snapshot = snapshot_store.get_snapshot()
    if snapshot is None:
        raise GpuSnapshotUnavailableError("gpu_snapshot_unavailable")
    snapshot.state.ok = True
    snapshot.state.stale_ms = snapshot_store.get_stale_ms()
    return snapshot


def _to_metric_value(reading: MetricReading, *, use_filter: bool) -> MetricValue:
    display: float | int | None = None
    if reading.valid and reading.raw_value is not None:
        if use_filter:
            display = _update_gpu_pct_filter(float(reading.raw_value), valid=True)
        else:
            display = reading.raw_value
    elif use_filter:
        display = _update_gpu_pct_filter(None, valid=False)

    return MetricValue(
        value_raw=reading.raw_value,
        value_display=display,
        source=reading.source,
        unit=reading.unit,
        sampled_at=reading.sampled_at_unix_ms,
        estimated=reading.estimated,
        valid=reading.valid,
    )


def _update_gpu_pct_filter(value: float | None, *, valid: bool) -> float | None:
    with _FILTER_LOCK:
        return _GPU_PCT_FILTER.update(value, valid=valid)
