from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState
from app.services.history_service import build_history
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
            pct=metric(2.0, unit="percent"),
            temp_c=metric(30.0, unit="celsius"),
            power_w=metric(20.0, unit="watt"),
        ),
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
    assert payload["ts_ms"] == [10000, 11000, 12000]


def test_build_history_ts_uses_latest_sample_not_request_time() -> None:
    import app.services.history_service as svc

    store = HistoryStore(capacity=10)
    store.push_snapshot(_snapshot(100, 1.0))
    store.push_snapshot(_snapshot(101, 2.0))

    original_time = svc.time.time
    svc.time.time = lambda: 999
    try:
        payload = build_history(history_store=store, window_s=10, step_s=1).model_dump()
    finally:
        svc.time.time = original_time

    assert payload["ts"] == 101


def test_build_history_timeline_is_stable_without_new_points() -> None:
    import app.services.history_service as svc

    store = HistoryStore(capacity=10)
    store.push_snapshot(_snapshot(100, 10.0), tick_ms=100_000)
    store.push_snapshot(_snapshot(101, 20.0), tick_ms=101_000)
    store.push_snapshot(_snapshot(102, 30.0), tick_ms=102_000)

    original_time = svc.time.time
    try:
        svc.time.time = lambda: 200
        first = build_history(history_store=store, window_s=10, step_s=1).model_dump()
        svc.time.time = lambda: 240
        second = build_history(history_store=store, window_s=10, step_s=1).model_dump()
    finally:
        svc.time.time = original_time

    assert first["ts_ms"] == [100000, 101000, 102000]
    assert second["ts_ms"] == first["ts_ms"]
    assert second["series"] == first["series"]
