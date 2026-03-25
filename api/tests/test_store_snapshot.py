from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.store import SnapshotStore
from _helpers import metric


def _snapshot(ts: int) -> DashboardResponse:
    return DashboardResponse(
        v=1,
        ts=ts,
        host="linux-main",
        cpu=CpuSnapshot(
            pct=metric(10.0, unit="percent"),
            temp_c=metric(40.0, unit="celsius"),
            power_w=metric(None, unit="watt", valid=False),
        ),
        mem=MemSnapshot(
            used_b=metric(100, unit="bytes"),
            total_b=metric(200, unit="bytes"),
            pct=metric(50.0, unit="percent"),
        ),
        gpu=GpuSnapshot(
            pct=metric(None, unit="percent", valid=False),
            temp_c=metric(None, unit="celsius", valid=False),
            power_w=metric(None, unit="watt", valid=False),
        ),
        state=SnapshotState(ok=True, stale_ms=0),
    )


def test_snapshot_store_set_get_copy_and_stale() -> None:
    store = SnapshotStore()
    store.set_snapshot(_snapshot(1), tick_ms=2_000)

    read = store.get_snapshot()
    assert read is not None
    assert read.host == "linux-main"
    assert store.get_stale_ms(now_ms=2_450) == 450


def test_snapshot_store_returns_none_when_empty() -> None:
    store = SnapshotStore()

    assert store.get_snapshot() is None
    assert store.has_snapshot() is False
