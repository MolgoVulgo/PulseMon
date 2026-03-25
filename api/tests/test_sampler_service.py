from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.services import SamplerService
from app.store import HistoryStore, SnapshotStore
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
            pct=metric(1.0, unit="percent"),
            temp_c=metric(30.0, unit="celsius"),
            power_w=metric(20.0, unit="watt"),
        ),
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


def test_sampler_decouples_acquire_and_publish() -> None:
    import app.services.sampler as sampler_mod

    snapshot_store = SnapshotStore()
    history_store = HistoryStore(capacity=10)
    ticks = iter([10.0, 10.1, 10.2, 10.6])

    original_monotonic = sampler_mod.time.monotonic
    sampler_mod.time.monotonic = lambda: next(ticks)
    try:
        sampler = SamplerService(
            snapshot_store=snapshot_store,
            history_store=history_store,
            sample_func=lambda: _snapshot(100),
            interval_s=0.1,
            publish_interval_s=0.5,
        )

        sampler.sample_once(force_publish=True)  # publish at t=10.0
        sampler.sample_once()  # acquire at t=10.1, no publish
        sampler.sample_once()  # acquire at t=10.2, no publish
        sampler.sample_once()  # acquire at t=10.6, publish
    finally:
        sampler_mod.time.monotonic = original_monotonic

    assert len(history_store) == 2
