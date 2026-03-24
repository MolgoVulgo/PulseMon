from __future__ import annotations

import logging
import threading
import time
from collections.abc import Callable

from app.models import DashboardResponse
from app.store import HistoryStore, SnapshotStore


logger = logging.getLogger(__name__)


class SamplerService:
    def __init__(
        self,
        *,
        snapshot_store: SnapshotStore,
        history_store: HistoryStore,
        sample_func: Callable[[], DashboardResponse],
        interval_s: float = 1.0,
    ) -> None:
        self._snapshot_store = snapshot_store
        self._history_store = history_store
        self._sample_func = sample_func
        self._interval_s = interval_s
        self._stop_event = threading.Event()
        self._thread: threading.Thread | None = None
        self._last_error: str | None = None

    def start(self) -> None:
        if self._thread is not None and self._thread.is_alive():
            return

        self._stop_event.clear()
        self.sample_once()
        self._thread = threading.Thread(target=self._run, name="stats-sampler", daemon=True)
        self._thread.start()
        logger.info("sampler started interval_s=%s", self._interval_s)

    def stop(self) -> None:
        self._stop_event.set()
        if self._thread is not None:
            self._thread.join(timeout=2.0)
        logger.info("sampler stopped")

    def sample_once(self) -> None:
        try:
            snapshot = self._sample_func()
        except Exception as exc:
            error_sig = f"{type(exc).__name__}:{exc}"
            if error_sig != self._last_error:
                logger.warning("sampler sample failed: %s", error_sig)
                self._last_error = error_sig
            return

        self._last_error = None
        now_ms = int(time.time() * 1000)
        self._snapshot_store.set_snapshot(snapshot, tick_ms=now_ms)
        self._history_store.push_snapshot(snapshot)

    def _run(self) -> None:
        while not self._stop_event.is_set():
            self.sample_once()
            self._stop_event.wait(self._interval_s)
