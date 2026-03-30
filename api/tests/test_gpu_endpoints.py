from app import main as main_mod


def test_get_gpu_dashboard_contract_shape() -> None:
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
