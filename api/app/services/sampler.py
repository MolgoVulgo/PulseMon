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
        publish_interval_s: float | None = None,
    ) -> None:
        self._snapshot_store = snapshot_store
        self._history_store = history_store
        self._sample_func = sample_func
        self._acquire_interval_s = max(0.01, interval_s)
        self._publish_interval_s = max(0.01, publish_interval_s if publish_interval_s is not None else interval_s)
        self._stop_event = threading.Event()
        self._thread: threading.Thread | None = None
        self._last_error: str | None = None
        self._last_publish_mono_s: float | None = None
        self._publish_count = 0

    def start(self) -> None:
        if self._thread is not None and self._thread.is_alive():
            return

        self._stop_event.clear()
        self.sample_once(force_publish=True)
        self._thread = threading.Thread(target=self._run, name="stats-sampler", daemon=True)
        self._thread.start()
        logger.info(
            "sampler started acquire_interval_s=%s publish_interval_s=%s",
            self._acquire_interval_s,
            self._publish_interval_s,
        )

    def stop(self) -> None:
        self._stop_event.set()
        if self._thread is not None:
            self._thread.join(timeout=2.0)
        logger.info("sampler stopped")

    def sample_once(self, *, force_publish: bool = False) -> None:
        try:
            snapshot = self._sample_func()
        except Exception as exc:
            error_sig = f"{type(exc).__name__}:{exc}"
            if error_sig != self._last_error:
                logger.warning("sampler sample failed: %s", error_sig)
                self._last_error = error_sig
            return

        self._last_error = None
        now_mono_s = time.monotonic()
        if not force_publish and not self._should_publish(now_mono_s):
            return

        publish_delta_ms: int | None = None
        if self._last_publish_mono_s is not None:
            publish_delta_ms = int((now_mono_s - self._last_publish_mono_s) * 1000)
        self._last_publish_mono_s = now_mono_s
        now_ms = int(time.time() * 1000)
        self._snapshot_store.set_snapshot(snapshot, tick_ms=now_ms)
        self._history_store.push_snapshot(snapshot, tick_ms=now_ms)
        self._publish_count += 1

        logger.debug(
            "sampler publish #%s snapshot_ts=%s tick_ms=%s publish_delta_ms=%s",
            self._publish_count,
            snapshot.ts,
            now_ms,
            publish_delta_ms,
        )

    def _run(self) -> None:
        next_tick = time.monotonic()
        while not self._stop_event.is_set():
            self.sample_once()
            next_tick += self._acquire_interval_s
            now = time.monotonic()
            if next_tick <= now:
                next_tick = now + self._acquire_interval_s
            self._stop_event.wait(max(0.0, next_tick - now))

    def _should_publish(self, now_mono_s: float) -> bool:
        if self._last_publish_mono_s is None:
            return True
        return (now_mono_s - self._last_publish_mono_s) >= self._publish_interval_s
