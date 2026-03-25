from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import threading
import time
import logging
from typing import Literal

from app.models import DashboardResponse

logger = logging.getLogger(__name__)


HistoryMode = Literal["display", "raw"]


@dataclass(frozen=True)
class HistoryPoint:
    ts: int
    ts_ms: int
    cpu_pct: float | None
    cpu_temp_c: float | None
    gpu_pct: float | None
    gpu_temp_c: float | None


@dataclass(frozen=True)
class _StoredHistoryPoint:
    ts: int
    ts_ms: int
    cpu_pct_raw: float | None
    cpu_pct_display: float | None
    cpu_temp_c_raw: float | None
    cpu_temp_c_display: float | None
    gpu_pct_raw: float | None
    gpu_pct_display: float | None
    gpu_temp_c_raw: float | None
    gpu_temp_c_display: float | None


class HistoryStore:
    def __init__(self, capacity: int = 600) -> None:
        self._lock = threading.Lock()
        self._points: deque[_StoredHistoryPoint] = deque(maxlen=capacity)

    def push_snapshot(self, snapshot: DashboardResponse, *, tick_ms: int | None = None) -> None:
        point_ts_ms = _resolve_point_ts_ms(snapshot=snapshot, tick_ms=tick_ms)
        point = _StoredHistoryPoint(
            ts=point_ts_ms // 1000,
            ts_ms=point_ts_ms,
            cpu_pct_raw=_as_float_or_none(snapshot.cpu.pct.value_raw),
            cpu_pct_display=_as_float_or_none(snapshot.cpu.pct.value_display),
            cpu_temp_c_raw=_as_float_or_none(snapshot.cpu.temp_c.value_raw),
            cpu_temp_c_display=_as_float_or_none(snapshot.cpu.temp_c.value_display),
            gpu_pct_raw=_as_float_or_none(snapshot.gpu.pct.value_raw),
            gpu_pct_display=_as_float_or_none(snapshot.gpu.pct.value_display),
            gpu_temp_c_raw=_as_float_or_none(snapshot.gpu.temp_c.value_raw),
            gpu_temp_c_display=_as_float_or_none(snapshot.gpu.temp_c.value_display),
        )
        with self._lock:
            self._points.append(point)

    def get_points(
        self,
        window_s: int,
        step_s: int,
        now_ts: int,
        *,
        mode: HistoryMode = "display",
        since_ts_ms: int | None = None,
    ) -> list[HistoryPoint]:
        del now_ts  # history is anchored to the latest sampled point, not request wall clock
        if mode not in {"display", "raw"}:
            raise ValueError(f"unsupported history mode: {mode}")

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
        points_by_bucket: dict[int, _StoredHistoryPoint] = {}

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

        sampled_stored: list[_StoredHistoryPoint] = []
        for bucket in range(range_start_bucket, end_bucket + 1):
            point = points_by_bucket.get(bucket)
            if point is not None:
                sampled_stored.append(point)
                continue

            slot_ts_ms = bucket * step_ms
            sampled_stored.append(
                _StoredHistoryPoint(
                    ts=slot_ts_ms // 1000,
                    ts_ms=slot_ts_ms,
                    cpu_pct_raw=None,
                    cpu_pct_display=None,
                    cpu_temp_c_raw=None,
                    cpu_temp_c_display=None,
                    gpu_pct_raw=None,
                    gpu_pct_display=None,
                    gpu_temp_c_raw=None,
                    gpu_temp_c_display=None,
                )
            )

        sampled = [_to_history_point(point, mode=mode) for point in sampled_stored]
        response_kind = "full_window"
        if since_ts_ms is not None:
            if sampled and since_ts_ms < sampled[0].ts_ms:
                response_kind = "resync_full_window"
            else:
                sampled = [point for point in sampled if point.ts_ms > since_ts_ms]
                response_kind = "delta"

        if logger.isEnabledFor(logging.DEBUG):
            missing = sum(1 for point in sampled if point.cpu_pct is None and point.gpu_pct is None)
            logger.debug(
                "history sampled mode=%s since_ts_ms=%s response_kind=%s window_s=%s step_s=%s raw_points=%s buckets=%s returned=%s missing_slots=%s start_bucket=%s end_bucket=%s",
                mode,
                since_ts_ms,
                response_kind,
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

    def get_latest_ts(self) -> int | None:
        with self._lock:
            if not self._points:
                return None
            return self._points[-1].ts


def _as_float_or_none(value: float | int | None) -> float | None:
    if value is None:
        return None
    return float(value)


def _to_history_point(point: _StoredHistoryPoint, *, mode: HistoryMode) -> HistoryPoint:
    return HistoryPoint(
        ts=point.ts,
        ts_ms=point.ts_ms,
        cpu_pct=_select_value(raw=point.cpu_pct_raw, display=point.cpu_pct_display, mode=mode),
        cpu_temp_c=_select_value(raw=point.cpu_temp_c_raw, display=point.cpu_temp_c_display, mode=mode),
        gpu_pct=_select_value(raw=point.gpu_pct_raw, display=point.gpu_pct_display, mode=mode),
        gpu_temp_c=_select_value(raw=point.gpu_temp_c_raw, display=point.gpu_temp_c_display, mode=mode),
    )


def _select_value(*, raw: float | None, display: float | None, mode: HistoryMode) -> float | None:
    return raw if mode == "raw" else display


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
