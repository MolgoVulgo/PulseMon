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
from _helpers import metric


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

    assert payload["cpu"]["pct"]["value_raw"] == 12.4
    assert payload["cpu"]["power_w"]["valid"] is False
    assert payload["gpu"]["power_w"]["value_display"] == 36.4
    assert payload["mem"]["used_b"]["unit"] == "bytes"


def test_history_contract_snapshot() -> None:
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

    assert payload == {
        "v": 1,
        "ts": 1774256402,
        "ts_ms": [1774256399000, 1774256400000, 1774256401000, 1774256402000],
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
            "cpu.power_w",
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
