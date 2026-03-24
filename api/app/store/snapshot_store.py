from __future__ import annotations

import threading
import time

from app.models import DashboardResponse


class SnapshotStore:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._snapshot: DashboardResponse | None = None
        self._last_tick_ms: int | None = None

    def set_snapshot(self, snapshot: DashboardResponse, tick_ms: int | None = None) -> None:
        now_ms = tick_ms if tick_ms is not None else int(time.time() * 1000)
        with self._lock:
            self._snapshot = snapshot.model_copy(deep=True)
            self._last_tick_ms = now_ms

    def get_snapshot(self) -> DashboardResponse | None:
        with self._lock:
            if self._snapshot is None:
                return None
            return self._snapshot.model_copy(deep=True)

    def get_stale_ms(self, now_ms: int | None = None) -> int:
        now = now_ms if now_ms is not None else int(time.time() * 1000)
        with self._lock:
            if self._last_tick_ms is None:
                return 0
            return max(0, now - self._last_tick_ms)

    def has_snapshot(self) -> bool:
        with self._lock:
            return self._snapshot is not None
