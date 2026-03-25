from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.store import HistoryStore
from _helpers import metric


def _snapshot(ts: int, cpu_pct: float) -> DashboardResponse:
    return DashboardResponse(
        v=1,
        ts=ts,
        host="linux-main",
        cpu=CpuSnapshot(
            pct=metric(cpu_pct, unit="percent"),
            temp_c=metric(40.0, unit="celsius"),
            power_w=metric(None, unit="watt", valid=False),
        ),
        mem=MemSnapshot(
            used_b=metric(100, unit="bytes"),
            total_b=metric(200, unit="bytes"),
            pct=metric(50.0, unit="percent"),
        ),
        gpu=GpuSnapshot(
            pct=metric(1.0, unit="percent"),
            temp_c=metric(30.0, unit="celsius"),
            power_w=metric(20.0, unit="watt"),
        ),
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


def test_history_store_bucket_by_real_timestamp() -> None:
    store = HistoryStore(capacity=10)
    store.push_snapshot(_snapshot(10, 1.0), tick_ms=10_000)
    store.push_snapshot(_snapshot(10, 2.0), tick_ms=10_500)
    store.push_snapshot(_snapshot(11, 3.0), tick_ms=11_100)
    store.push_snapshot(_snapshot(12, 4.0), tick_ms=12_000)

    points = store.get_points(window_s=2, step_s=1, now_ts=12)
    assert [p.cpu_pct for p in points] == [2.0, 3.0, 4.0]


def test_history_store_explicit_gap_returns_null_bucket() -> None:
    store = HistoryStore(capacity=10)
    store.push_snapshot(_snapshot(10, 1.0), tick_ms=10_000)
    store.push_snapshot(_snapshot(12, 3.0), tick_ms=12_000)

    points = store.get_points(window_s=2, step_s=1, now_ts=12)
    assert [p.ts for p in points] == [10, 11, 12]
    assert [p.cpu_pct for p in points] == [1.0, None, 3.0]
