from app.models import (
    CpuSnapshot,
    DashboardResponse,
    ErrorResponse,
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


def test_health_contract_snapshot() -> None:
    payload = HealthResponse(v=1, ts=1774256402, ok=True, service="stats-linux-api").model_dump()

    assert payload == {
        "v": 1,
        "ts": 1774256402,
        "ok": True,
        "service": "stats-linux-api",
    }


def test_dashboard_contract_snapshot() -> None:
    payload = DashboardResponse(
        v=1,
        ts=1774256402,
        host="linux-main",
        cpu=CpuSnapshot(pct=12.4, temp_c=43.8),
        mem=MemSnapshot(used_b=9123454976, total_b=34359738368, pct=26.6),
        gpu=GpuSnapshot(pct=7.0, temp_c=39.0, power_w=36.4),
        state=SnapshotState(ok=True, stale_ms=0),
    ).model_dump()

    assert payload == {
        "v": 1,
        "ts": 1774256402,
        "host": "linux-main",
        "cpu": {"pct": 12.4, "temp_c": 43.8},
        "mem": {"used_b": 9123454976, "total_b": 34359738368, "pct": 26.6},
        "gpu": {"pct": 7.0, "temp_c": 39.0, "power_w": 36.4},
        "state": {"ok": True, "stale_ms": 0},
    }


def test_history_contract_snapshot() -> None:
    payload = HistoryResponse(
        v=1,
        ts=1774256402,
        window_s=300,
        step_s=1,
        series=HistorySeries(
            cpu_pct=[8.0, 9.4, 11.2, 10.1],
            cpu_temp_c=[41.0, 41.1, 41.3, 41.4],
            gpu_pct=[2.0, 4.0, 7.0, 3.0],
            gpu_temp_c=[38.0, 38.0, 39.0, 39.0],
        ),
    ).model_dump()

    assert payload == {
        "v": 1,
        "ts": 1774256402,
        "window_s": 300,
        "step_s": 1,
        "series": {
            "cpu_pct": [8.0, 9.4, 11.2, 10.1],
            "cpu_temp_c": [41.0, 41.1, 41.3, 41.4],
            "gpu_pct": [2.0, 4.0, 7.0, 3.0],
            "gpu_temp_c": [38.0, 38.0, 39.0, 39.0],
        },
    }


def test_meta_contract_snapshot() -> None:
    payload = MetaResponse(v=1, host="linux-main", metrics=V1_METRICS, history_series=V1_HISTORY_SERIES).model_dump()

    assert payload == {
        "v": 1,
        "host": "linux-main",
        "metrics": [
            "cpu.pct",
            "cpu.temp_c",
            "mem.used_b",
            "mem.total_b",
            "mem.pct",
            "gpu.pct",
            "gpu.temp_c",
            "gpu.power_w",
        ],
        "history_series": [
            "cpu_pct",
            "cpu_temp_c",
            "gpu_pct",
            "gpu_temp_c",
        ],
    }


def test_error_contract_snapshot() -> None:
    payload = ErrorResponse(v=1, error="invalid_parameter", field="window").model_dump()

    assert payload == {
        "v": 1,
        "error": "invalid_parameter",
        "field": "window",
    }


def test_error_contract_nullable_field() -> None:
    payload = ErrorResponse(v=1, error="invalid_parameter").model_dump()

    assert payload == {
        "v": 1,
        "error": "invalid_parameter",
        "field": None,
    }
