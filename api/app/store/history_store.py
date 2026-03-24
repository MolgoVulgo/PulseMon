from __future__ import annotations

from collections import deque
from dataclasses import dataclass

from app.models import DashboardResponse


@dataclass(frozen=True)
class HistoryPoint:
    ts: int
    cpu_pct: float | None
    cpu_temp_c: float | None
    gpu_pct: float | None
    gpu_temp_c: float | None


class HistoryStore:
    def __init__(self, capacity: int = 600) -> None:
        self._points: deque[HistoryPoint] = deque(maxlen=capacity)

    def push_snapshot(self, snapshot: DashboardResponse) -> None:
        self._points.append(
            HistoryPoint(
                ts=snapshot.ts,
                cpu_pct=snapshot.cpu.pct,
                cpu_temp_c=snapshot.cpu.temp_c,
                gpu_pct=snapshot.gpu.pct,
                gpu_temp_c=snapshot.gpu.temp_c,
            )
        )

    def get_points(self, window_s: int, step_s: int, now_ts: int) -> list[HistoryPoint]:
        threshold = now_ts - window_s
        filtered = [point for point in self._points if point.ts >= threshold]
        if step_s <= 1:
            return filtered

        sampled: list[HistoryPoint] = []
        last_ts: int | None = None
        for point in filtered:
            if last_ts is None or (point.ts - last_ts) >= step_s:
                sampled.append(point)
                last_ts = point.ts
        return sampled

    def __len__(self) -> int:
        return len(self._points)
