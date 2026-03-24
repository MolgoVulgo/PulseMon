from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.store import HistoryStore


def _snapshot(ts: int, cpu_pct: float) -> DashboardResponse:
    return DashboardResponse(
        v=1,
        ts=ts,
        host="linux-main",
        cpu=CpuSnapshot(pct=cpu_pct, temp_c=40.0, power_w=None),
        mem=MemSnapshot(used_b=100, total_b=200, pct=50.0),
        gpu=GpuSnapshot(pct=1.0, temp_c=30.0, power_w=20.0),
        state=SnapshotState(ok=True, stale_ms=0),
    )


def test_history_store_rotation() -> None:
    store = HistoryStore(capacity=3)
    store.push_snapshot(_snapshot(1, 1.0))
    store.push_snapshot(_snapshot(2, 2.0))
    store.push_snapshot(_snapshot(3, 3.0))
    store.push_snapshot(_snapshot(4, 4.0))

    points = store.get_points(window_s=10, step_s=1, now_ts=4)
    assert [p.ts for p in points] == [2, 3, 4]


def test_history_store_window_and_step() -> None:
    store = HistoryStore(capacity=10)
    for ts in range(1, 8):
        store.push_snapshot(_snapshot(ts, float(ts)))

    points = store.get_points(window_s=4, step_s=2, now_ts=7)
    assert [p.ts for p in points] == [3, 5, 7]
