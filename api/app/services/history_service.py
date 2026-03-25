import time
import logging

from app.models import HistoryResponse, HistorySeries
from app.store import HistoryStore

logger = logging.getLogger(__name__)


def build_history(*, history_store: HistoryStore, window_s: int, step_s: int) -> HistoryResponse:
    now_ts = int(time.time())
    points = history_store.get_points(window_s=window_s, step_s=step_s, now_ts=now_ts)
    response_ts = points[-1].ts if points else now_ts

    if logger.isEnabledFor(logging.DEBUG):
        logger.debug(
            "history response ts=%s window_s=%s step_s=%s points=%s",
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
