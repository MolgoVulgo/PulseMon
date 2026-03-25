import time
import logging
from typing import Literal

from app.models import HistoryResponse, HistorySeries
from app.store import HistoryStore

logger = logging.getLogger(__name__)

HistoryMode = Literal["display", "raw"]


def build_history(
    *,
    history_store: HistoryStore,
    window_s: int,
    step_s: int,
    mode: HistoryMode = "display",
    since_ts_ms: int | None = None,
) -> HistoryResponse:
    now_ts = int(time.time())
    latest_store_ts = history_store.get_latest_ts()
    points = history_store.get_points(
        window_s=window_s,
        step_s=step_s,
        now_ts=now_ts,
        mode=mode,
        since_ts_ms=since_ts_ms,
    )
    response_ts = points[-1].ts if points else (latest_store_ts if latest_store_ts is not None else now_ts)

    if logger.isEnabledFor(logging.DEBUG):
        logger.debug(
            "history response mode=%s since_ts_ms=%s ts=%s window_s=%s step_s=%s points=%s",
            mode,
            since_ts_ms,
            response_ts,
            window_s,
            step_s,
            len(points),
        )

    return HistoryResponse(
        v=1,
        ts=response_ts,
        ts_ms=[point.ts_ms for point in points],
        window_s=window_s,
        step_s=step_s,
        series=HistorySeries(
            cpu_pct=[point.cpu_pct for point in points],
            cpu_temp_c=[point.cpu_temp_c for point in points],
            gpu_pct=[point.gpu_pct for point in points],
            gpu_temp_c=[point.gpu_temp_c for point in points],
        ),
    )
