from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.store import SnapshotStore


def _snapshot(ts: int) -> DashboardResponse:
    return DashboardResponse(
        v=1,
        ts=ts,
        host="linux-main",
        cpu=CpuSnapshot(pct=10.0, temp_c=40.0, power_w=None),
        mem=MemSnapshot(used_b=100, total_b=200, pct=50.0),
        gpu=GpuSnapshot(pct=None, temp_c=None, power_w=None),
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
