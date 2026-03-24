from fastapi import HTTPException

from app import main as main_mod
from app.models import CpuSnapshot, DashboardResponse, GpuSnapshot, MemSnapshot, SnapshotState


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


def test_get_history_invalid_window_raises_400_contract() -> None:
    try:
        main_mod.get_history(window=5, step=1)
    except HTTPException as exc:
        assert exc.status_code == 400
        assert exc.detail == {
            "v": 1,
            "error": "invalid_parameter",
            "field": "window",
        }
        return

    raise AssertionError("HTTPException not raised")


def test_get_history_invalid_step_raises_400_contract() -> None:
    try:
        main_mod.get_history(window=300, step=11)
    except HTTPException as exc:
        assert exc.status_code == 400
        assert exc.detail == {
            "v": 1,
            "error": "invalid_parameter",
            "field": "step",
        }
        return

    raise AssertionError("HTTPException not raised")


def test_get_history_valid_returns_model() -> None:
    original_store = main_mod.history_store
    try:
        test_store = main_mod.HistoryStore(capacity=10)
        test_store.push_snapshot(_snapshot(100))
        main_mod.history_store = test_store

        payload = main_mod.get_history(window=300, step=1).model_dump()
    finally:
        main_mod.history_store = original_store

    assert payload["v"] == 1
    assert "series" in payload


def test_get_meta_returns_model() -> None:
    payload = main_mod.get_meta().model_dump()

    assert payload["v"] == 1
    assert "metrics" in payload
    assert "history_series" in payload


def test_is_api_key_required_false_when_not_configured() -> None:
    original = main_mod.config
    main_mod.config = main_mod.AppConfig(api_key=None)
    try:
        assert main_mod._is_api_key_required() is False
    finally:
        main_mod.config = original


def test_is_api_key_required_true_when_configured() -> None:
    original = main_mod.config
    main_mod.config = main_mod.AppConfig(api_key="secret")
    try:
        assert main_mod._is_api_key_required() is True
    finally:
        main_mod.config = original
