from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.services.history_service import build_history
from app.store import HistoryStore


def _snapshot(ts: int, cpu_pct: float) -> DashboardResponse:
    return DashboardResponse(
        v=1,
        ts=ts,
        host="linux-main",
        cpu=CpuSnapshot(pct=cpu_pct, temp_c=40.0, power_w=None),
        mem=MemSnapshot(used_b=100, total_b=200, pct=50.0),
        gpu=GpuSnapshot(pct=2.0, temp_c=30.0, power_w=20.0),
        state=SnapshotState(ok=True, stale_ms=0),
    )


def test_build_history_returns_aligned_series() -> None:
    import app.services.history_service as svc

    store = HistoryStore(capacity=10)
    store.push_snapshot(_snapshot(10, 1.0))
    store.push_snapshot(_snapshot(11, 2.0))
    store.push_snapshot(_snapshot(12, 3.0))

    original_time = svc.time.time
    svc.time.time = lambda: 12
    try:
        payload = build_history(history_store=store, window_s=10, step_s=1).model_dump()
    finally:
        svc.time.time = original_time

    assert payload["series"]["cpu_pct"] == [1.0, 2.0, 3.0]
    assert payload["series"]["cpu_temp_c"] == [40.0, 40.0, 40.0]
    assert payload["series"]["gpu_pct"] == [2.0, 2.0, 2.0]
    assert payload["series"]["gpu_temp_c"] == [30.0, 30.0, 30.0]
