from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import threading
import time
import logging

from app.models import DashboardResponse

logger = logging.getLogger(__name__)

@dataclass(frozen=True)
class HistoryPoint:
    ts: int
    ts_ms: int
    cpu_pct: float | None
    cpu_temp_c: float | None
    gpu_pct: float | None
    gpu_temp_c: float | None


class HistoryStore:
    def __init__(self, capacity: int = 600) -> None:
        self._lock = threading.Lock()
        self._points: deque[HistoryPoint] = deque(maxlen=capacity)

    def push_snapshot(self, snapshot: DashboardResponse, *, tick_ms: int | None = None) -> None:
        point_ts_ms = _resolve_point_ts_ms(snapshot=snapshot, tick_ms=tick_ms)
        point = HistoryPoint(
            ts=point_ts_ms // 1000,
            ts_ms=point_ts_ms,
            cpu_pct=_as_float_or_none(snapshot.cpu.pct.value_display),
            cpu_temp_c=_as_float_or_none(snapshot.cpu.temp_c.value_display),
            gpu_pct=_as_float_or_none(snapshot.gpu.pct.value_display),
            gpu_temp_c=_as_float_or_none(snapshot.gpu.temp_c.value_display),
        )
        with self._lock:
            self._points.append(point)

    def get_points(self, window_s: int, step_s: int, now_ts: int) -> list[HistoryPoint]:
        del now_ts  # history is anchored to the latest sampled point, not request wall clock

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
        points_by_bucket: dict[int, HistoryPoint] = {}

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

        sampled: list[HistoryPoint] = []
        for bucket in range(range_start_bucket, end_bucket + 1):
            point = points_by_bucket.get(bucket)
            if point is not None:
                sampled.append(point)
                continue

            slot_ts_ms = bucket * step_ms
            sampled.append(
                HistoryPoint(
                    ts=slot_ts_ms // 1000,
                    ts_ms=slot_ts_ms,
                    cpu_pct=None,
                    cpu_temp_c=None,
                    gpu_pct=None,
                    gpu_temp_c=None,
                )
            )

        if logger.isEnabledFor(logging.DEBUG):
            missing = sum(1 for point in sampled if point.cpu_pct is None and point.gpu_pct is None)
            logger.debug(
                "history sampled window_s=%s step_s=%s raw_points=%s buckets=%s returned=%s missing_slots=%s start_bucket=%s end_bucket=%s",
                window_s,
                step_s,
                len(points),
                len(points_by_bucket),
                len(sampled),
                missing,
                range_start_bucket,
                end_bucket,
            )

        return sampled

    def __len__(self) -> int:
        with self._lock:
            return len(self._points)


def _as_float_or_none(value: float | int | None) -> float | None:
    if value is None:
        return None
    return float(value)


def _resolve_point_ts_ms(*, snapshot: DashboardResponse, tick_ms: int | None) -> int:
    if tick_ms is not None:
        return int(tick_ms)

    snapshot_ts_ms = int(snapshot.ts) * 1000 if snapshot.ts > 0 else None
    metric_ts_ms = _extract_metric_ts_ms(snapshot)

    if snapshot_ts_ms is not None:
        return snapshot_ts_ms

    if metric_ts_ms is not None:
        return metric_ts_ms
    return int(time.time() * 1000)


def _extract_metric_ts_ms(snapshot: DashboardResponse) -> int | None:
    candidates = [
        snapshot.cpu.pct.sampled_at,
        snapshot.cpu.temp_c.sampled_at,
        snapshot.cpu.power_w.sampled_at,
        snapshot.mem.used_b.sampled_at,
        snapshot.mem.total_b.sampled_at,
        snapshot.mem.pct.sampled_at,
        snapshot.gpu.pct.sampled_at,
        snapshot.gpu.temp_c.sampled_at,
        snapshot.gpu.power_w.sampled_at,
    ]
    valid = [int(ts_ms) for ts_ms in candidates if ts_ms is not None and ts_ms > 0]
    if not valid:
        return None
    return max(valid)
