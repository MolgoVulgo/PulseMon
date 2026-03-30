from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import threading
import time
from typing import Literal

from app.models import GpuDashboardResponse

GpuHistoryMode = Literal["display", "raw"]


@dataclass(frozen=True)
class GpuHistoryPoint:
    ts: int
    ts_ms: int
    gpu_pct: float | None
    gpu_core_clock_mhz: float | None
    gpu_vram_used_b: float | None
    gpu_temp_c: float | None
    gpu_power_w: float | None
    gpu_mem_clock_mhz: float | None
    gpu_fan_rpm: float | None


@dataclass(frozen=True)
class _StoredGpuHistoryPoint:
    ts: int
    ts_ms: int
    gpu_pct_raw: float | None
    gpu_pct_display: float | None
    gpu_core_clock_mhz: float | None
    gpu_vram_used_b: float | None
    gpu_temp_c: float | None
    gpu_power_w: float | None
    gpu_mem_clock_mhz: float | None
    gpu_fan_rpm: float | None


class GpuHistoryStore:
    def __init__(self, capacity: int = 600) -> None:
        self._lock = threading.Lock()
        self._points: deque[_StoredGpuHistoryPoint] = deque(maxlen=capacity)

    def push_snapshot(self, snapshot: GpuDashboardResponse, *, tick_ms: int | None = None) -> None:
        point_ts_ms = int(tick_ms if tick_ms is not None else time.time() * 1000)
        point = _StoredGpuHistoryPoint(
            ts=point_ts_ms // 1000,
            ts_ms=point_ts_ms,
            gpu_pct_raw=_as_float_or_none(snapshot.gpu.pct.value_raw),
            gpu_pct_display=_as_float_or_none(snapshot.gpu.pct.value_display),
            gpu_core_clock_mhz=_as_float_or_none(snapshot.gpu.core_clock_mhz.value_display),
            gpu_vram_used_b=_as_float_or_none(snapshot.gpu.vram_used_b.value_display),
            gpu_temp_c=_as_float_or_none(snapshot.gpu.temp_c.value_display),
            gpu_power_w=_as_float_or_none(snapshot.gpu.power_w.value_display),
            gpu_mem_clock_mhz=_as_float_or_none(snapshot.gpu.mem_clock_mhz.value_display),
            gpu_fan_rpm=_as_float_or_none(snapshot.gpu.fan_rpm.value_display),
        )
        with self._lock:
            self._points.append(point)

    def get_points(self, window_s: int, step_s: int, *, mode: GpuHistoryMode = "display") -> list[GpuHistoryPoint]:
        with self._lock:
            points = list(self._points)

        if not points:
            return []

        step_ms = max(1, step_s * 1000)
        window_ms = max(0, window_s * 1000)
        end_ts_ms = points[-1].ts_ms
        end_bucket = end_ts_ms // step_ms
        bucket_span = window_ms // step_ms
        start_bucket = max(0, end_bucket - bucket_span)

        points_by_bucket: dict[int, _StoredGpuHistoryPoint] = {}
        for point in points:
            bucket = point.ts_ms // step_ms
            if bucket < start_bucket or bucket > end_bucket:
                continue
            existing = points_by_bucket.get(bucket)
            if existing is None or point.ts_ms >= existing.ts_ms:
                points_by_bucket[bucket] = point

        if not points_by_bucket:
            return []

        first_bucket = min(points_by_bucket.keys())
        range_start_bucket = max(start_bucket, first_bucket)

        sampled: list[GpuHistoryPoint] = []
        for bucket in range(range_start_bucket, end_bucket + 1):
            point = points_by_bucket.get(bucket)
            if point is None:
                ts_ms = bucket * step_ms
                sampled.append(
                    GpuHistoryPoint(
                        ts=ts_ms // 1000,
                        ts_ms=ts_ms,
                        gpu_pct=None,
                        gpu_core_clock_mhz=None,
                        gpu_vram_used_b=None,
                        gpu_temp_c=None,
                        gpu_power_w=None,
                        gpu_mem_clock_mhz=None,
                        gpu_fan_rpm=None,
                    )
                )
                continue

            sampled.append(
                GpuHistoryPoint(
                    ts=point.ts,
                    ts_ms=point.ts_ms,
                    gpu_pct=point.gpu_pct_raw if mode == "raw" else point.gpu_pct_display,
                    gpu_core_clock_mhz=point.gpu_core_clock_mhz,
                    gpu_vram_used_b=point.gpu_vram_used_b,
                    gpu_temp_c=point.gpu_temp_c,
                    gpu_power_w=point.gpu_power_w,
                    gpu_mem_clock_mhz=point.gpu_mem_clock_mhz,
                    gpu_fan_rpm=point.gpu_fan_rpm,
                )
            )

        return sampled

    def get_latest_ts(self) -> int | None:
        with self._lock:
            if not self._points:
                return None
            return self._points[-1].ts

    def __len__(self) -> int:
        with self._lock:
            return len(self._points)


def _as_float_or_none(value: float | int | None) -> float | None:
    if value is None:
        return None
    return float(value)
