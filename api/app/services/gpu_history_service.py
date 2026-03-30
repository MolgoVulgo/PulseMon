import time

from app.models import GpuHistoryResponse, GpuHistorySeries
from app.store import GpuHistoryStore


def build_gpu_history(*, history_store: GpuHistoryStore, window_s: int, step_s: int, mode: str = "display") -> GpuHistoryResponse:
    points = history_store.get_points(window_s=window_s, step_s=step_s, mode="raw" if mode == "raw" else "display")
    latest_store_ts = history_store.get_latest_ts()
    now_ts = int(time.time())
    response_ts = points[-1].ts if points else (latest_store_ts if latest_store_ts is not None else now_ts)

    return GpuHistoryResponse(
        v=1,
        ts=response_ts,
        ts_ms=[point.ts_ms for point in points],
        window_s=window_s,
        step_s=step_s,
        series=GpuHistorySeries(
            gpu_pct=[point.gpu_pct for point in points],
            gpu_core_clock_mhz=[point.gpu_core_clock_mhz for point in points],
            gpu_vram_used_b=[point.gpu_vram_used_b for point in points],
            gpu_temp_c=[point.gpu_temp_c for point in points],
            gpu_power_w=[point.gpu_power_w for point in points],
            gpu_mem_clock_mhz=[point.gpu_mem_clock_mhz for point in points],
            gpu_fan_rpm=[point.gpu_fan_rpm for point in points],
        ),
    )
