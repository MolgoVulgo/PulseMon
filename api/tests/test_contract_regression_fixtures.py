import json
from pathlib import Path

from app import main as main_mod
from app.models import (
    CpuSnapshot,
    DashboardResponse,
    GpuSnapshot,
    HealthResponse,
    HistoryResponse,
    HistorySeries,
    MemSnapshot,
    MetaResponse,
    SnapshotState,
    V1_HISTORY_SERIES,
    V1_METRICS,
)
from app.services.history_service import build_history
from app.store import HistoryStore
from _helpers import metric

FIXTURES_DIR = Path(__file__).parent / "fixtures"


def _fixture(name: str) -> dict:
    return json.loads((FIXTURES_DIR / name).read_text(encoding="utf-8"))


def test_health_fixture_regression() -> None:
    payload = HealthResponse(v=1, ts=1774256402, ok=True, service="stats-linux-api").model_dump()
    assert payload == _fixture("health_v1.json")


def test_dashboard_fixture_regression() -> None:
    payload = DashboardResponse(
        v=1,
        ts=1774256402,
        host="linux-main",
        cpu=CpuSnapshot(
            pct=metric(12.4, unit="percent"),
            temp_c=metric(43.8, unit="celsius"),
            power_w=metric(None, unit="watt", valid=False),
        ),
        mem=MemSnapshot(
            used_b=metric(9123454976, unit="bytes"),
            total_b=metric(34359738368, unit="bytes"),
            pct=metric(26.6, unit="percent"),
        ),
        gpu=GpuSnapshot(
            pct=metric(7.0, unit="percent"),
            temp_c=metric(39.0, unit="celsius"),
            power_w=metric(36.4, unit="watt"),
        ),
        state=SnapshotState(ok=True, stale_ms=0),
    ).model_dump()
    assert payload == _fixture("dashboard_v1.json")


def test_history_fixture_regression() -> None:
    payload = HistoryResponse(
        v=1,
        ts=1774256402,
        ts_ms=[1774256399000, 1774256400000, 1774256401000, 1774256402000],
        window_s=300,
        step_s=1,
        series=HistorySeries(
            cpu_pct=[8.0, 9.4, 11.2, 10.1],
            cpu_temp_c=[41.0, 41.1, 41.3, 41.4],
            gpu_pct=[2.0, 4.0, 7.0, 3.0],
            gpu_temp_c=[38.0, 38.0, 39.0, 39.0],
        ),
    ).model_dump()
    assert payload == _fixture("history_v1.json")


def test_meta_fixture_regression() -> None:
    payload = MetaResponse(v=1, host="linux-main", metrics=V1_METRICS, history_series=V1_HISTORY_SERIES).model_dump()
    assert payload == _fixture("meta_v1.json")


def test_error_fixture_regression() -> None:
    try:
        main_mod.get_history(window=0, step=1)
    except Exception as exc:  # FastAPI HTTPException
        detail = getattr(exc, "detail", None)
        assert detail == _fixture("error_invalid_window_v1.json")
        return

    raise AssertionError("Expected exception not raised")


def test_history_service_fixture_regression() -> None:
    import app.services.history_service as svc

    store = HistoryStore(capacity=20)
    for i, (cpu, cpu_t, gpu, gpu_t) in enumerate(
        [(8.0, 41.0, 2.0, 38.0), (9.4, 41.1, 4.0, 38.0), (11.2, 41.3, 7.0, 39.0), (10.1, 41.4, 3.0, 39.0)],
        start=0,
    ):
        store.push_snapshot(
            DashboardResponse(
                v=1,
                ts=1774256399 + i,
                host="linux-main",
                cpu=CpuSnapshot(
                    pct=metric(cpu, unit="percent"),
                    temp_c=metric(cpu_t, unit="celsius"),
                    power_w=metric(None, unit="watt", valid=False),
                ),
                mem=MemSnapshot(
                    used_b=metric(1, unit="bytes"),
                    total_b=metric(1, unit="bytes"),
                    pct=metric(1.0, unit="percent"),
                ),
                gpu=GpuSnapshot(
                    pct=metric(gpu, unit="percent"),
                    temp_c=metric(gpu_t, unit="celsius"),
                    power_w=metric(1.0, unit="watt"),
                ),
                state=SnapshotState(ok=True, stale_ms=0),
            )
        )

    original_time = svc.time.time
    svc.time.time = lambda: 1774256402
    try:
        payload = build_history(history_store=store, window_s=300, step_s=1).model_dump()
    finally:
        svc.time.time = original_time

    assert payload == _fixture("history_v1.json")
