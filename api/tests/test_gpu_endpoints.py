from app import main as main_mod
from app.models import GpuDashboardResponse, GpuExtendedSnapshot, SnapshotState
from _helpers import metric


def _gpu_snapshot(ts: int = 100) -> GpuDashboardResponse:
    return GpuDashboardResponse(
        v=1,
        ts=ts,
        host="linux-main",
        gpu=GpuExtendedSnapshot(
            pct=metric(10.0, unit="percent"),
            core_clock_mhz=metric(2400, unit="mhz"),
            mem_clock_mhz=metric(1000, unit="mhz"),
            vram_used_b=metric(1024, unit="bytes"),
            vram_total_b=metric(2048, unit="bytes"),
            vram_pct=metric(50.0, unit="percent"),
            temp_c=metric(44.0, unit="celsius"),
            power_w=metric(80.0, unit="watt"),
            fan_rpm=metric(1200, unit="rpm"),
            fan_pct=metric(40.0, unit="percent"),
        ),
        state=SnapshotState(ok=True, stale_ms=0),
    )


def test_get_gpu_dashboard_contract_shape() -> None:
    main_mod.gpu_snapshot_store.set_snapshot(_gpu_snapshot(), tick_ms=1774256402000)

    payload = main_mod.get_gpu_dashboard().model_dump()

    assert payload["v"] == 1
    assert "gpu" in payload
    assert "state" in payload
    assert "pct" in payload["gpu"]
    assert "core_clock_mhz" in payload["gpu"]
    assert "mem_clock_mhz" in payload["gpu"]
    assert "vram_used_b" in payload["gpu"]
    assert "vram_total_b" in payload["gpu"]
    assert "vram_pct" in payload["gpu"]
    assert "fan_rpm" in payload["gpu"]
    assert "fan_pct" in payload["gpu"]


def test_get_gpu_dashboard_without_snapshot_returns_503() -> None:
    store = main_mod.gpu_snapshot_store
    original_store = main_mod.gpu_snapshot_store
    main_mod.gpu_snapshot_store = type(store)()
    try:
        try:
            main_mod.get_gpu_dashboard()
        except Exception as exc:
            assert getattr(exc, "status_code", None) == 503
            assert getattr(exc, "detail", None) == {
                "v": 1,
                "error": "snapshot_unavailable",
                "field": None,
            }
            return
    finally:
        main_mod.gpu_snapshot_store = original_store

    raise AssertionError("Expected HTTPException")


def test_get_gpu_history_invalid_parameter() -> None:
    try:
        main_mod.get_gpu_history(window=0, step=1)
    except Exception as exc:
        assert getattr(exc, "status_code", None) == 400
        assert getattr(exc, "detail", None) == {
            "v": 1,
            "error": "invalid_parameter",
            "field": "window",
        }
        return

    raise AssertionError("Expected HTTPException")


def test_get_gpu_meta_contract_shape() -> None:
    payload = main_mod.get_gpu_meta().model_dump()

    assert payload["v"] == 1
    assert "gpu_name" in payload
    assert "metrics" in payload
    assert "history_series" in payload
    assert "caps" in payload
    assert "fan_pct" in payload["caps"]
