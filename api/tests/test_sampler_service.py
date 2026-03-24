from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.services import SamplerService
from app.store import HistoryStore, SnapshotStore


def _snapshot(ts: int) -> DashboardResponse:
    return DashboardResponse(
        v=1,
        ts=ts,
        host="linux-main",
        cpu=CpuSnapshot(pct=10.0, temp_c=40.0, power_w=None),
        mem=MemSnapshot(used_b=100, total_b=200, pct=50.0),
        gpu=GpuSnapshot(pct=1.0, temp_c=30.0, power_w=20.0),
        state=SnapshotState(ok=True, stale_ms=0),
    )


def test_sampler_sample_once_updates_snapshot_and_history() -> None:
    snapshot_store = SnapshotStore()
    history_store = HistoryStore(capacity=10)

    sampler = SamplerService(
        snapshot_store=snapshot_store,
        history_store=history_store,
        sample_func=lambda: _snapshot(100),
        interval_s=1.0,
    )

    sampler.sample_once()

    snap = snapshot_store.get_snapshot()
    assert snap is not None
    assert snap.ts == 100
    assert len(history_store) == 1


def test_sampler_sample_once_swallow_errors() -> None:
    snapshot_store = SnapshotStore()
    history_store = HistoryStore(capacity=10)

    def boom() -> DashboardResponse:
        raise RuntimeError("sensor failed")

    sampler = SamplerService(
        snapshot_store=snapshot_store,
        history_store=history_store,
        sample_func=boom,
        interval_s=1.0,
    )

    sampler.sample_once()

    assert snapshot_store.get_snapshot() is None
    assert len(history_store) == 0
