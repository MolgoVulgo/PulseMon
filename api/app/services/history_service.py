import time

from app.models import HistoryResponse, HistorySeries
from app.store import HistoryStore


def build_history(*, history_store: HistoryStore, window_s: int, step_s: int) -> HistoryResponse:
    now_ts = int(time.time())
    points = history_store.get_points(window_s=window_s, step_s=step_s, now_ts=now_ts)

    return HistoryResponse(
        v=1,
        ts=now_ts,
        window_s=window_s,
        step_s=step_s,
        series=HistorySeries(
            cpu_pct=[point.cpu_pct for point in points],
            cpu_temp_c=[point.cpu_temp_c for point in points],
            gpu_pct=[point.gpu_pct for point in points],
            gpu_temp_c=[point.gpu_temp_c for point in points],
        ),
    )
