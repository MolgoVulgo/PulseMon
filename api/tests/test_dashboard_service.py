from app.collectors.metric import MetricReading
from app.services.dashboard_service import (
    SnapshotUnavailableError,
    build_dashboard_from_store,
    build_dashboard_live,
)
from app.store import SnapshotStore


def _reading(
    value: float | int | None,
    *,
    source: str,
    unit: str,
    sampled_at: int = 1774256402000,
    mono: float = 10.0,
    read_ok: bool = True,
    error: str | None = None,
) -> MetricReading:
    return MetricReading(
        raw_value=value,
        source=source,
        unit=unit,
        sampled_at_unix_ms=sampled_at,
        sampled_at_monotonic_s=mono,
        read_ok=read_ok,
        read_error=error,
        estimated=False,
    )


def test_build_dashboard_live_contract_minimal() -> None:
    import app.services.dashboard_service as svc

    original_cpu_pct = svc.read_cpu_percent_metric
    original_cpu_temp = svc.read_cpu_temp_metric
    original_cpu_power = svc.read_cpu_power_metric
    original_mem = svc.read_memory
    original_gpu_pct = svc.read_gpu_percent_metric
    original_gpu_temp = svc.read_gpu_temp_metric
    original_gpu_power = svc.read_gpu_power_metric
    original_host = svc.socket.gethostname
    original_time = svc.time.time

    svc.read_cpu_percent_metric = lambda: _reading(12.4, source="cpu", unit="percent")
    svc.read_cpu_temp_metric = lambda: _reading(43.8, source="cpu-temp", unit="celsius")
    svc.read_cpu_power_metric = lambda: _reading(None, source="cpu-power", unit="watt", read_ok=False, error="missing")
    svc.read_memory = lambda: (9123454976, 34359738368, 26.6)
    svc.read_gpu_percent_metric = lambda: _reading(7.0, source="gpu", unit="percent")
    svc.read_gpu_temp_metric = lambda: _reading(39.0, source="gpu-temp", unit="celsius")
    svc.read_gpu_power_metric = lambda: _reading(36.4, source="gpu-power", unit="watt")
    svc.socket.gethostname = lambda: "linux-main"
    svc.time.time = lambda: 1774256402

    try:
        payload = build_dashboard_live(display_ema_alpha=0.25).model_dump()
    finally:
        svc.read_cpu_percent_metric = original_cpu_pct
        svc.read_cpu_temp_metric = original_cpu_temp
        svc.read_cpu_power_metric = original_cpu_power
        svc.read_memory = original_mem
        svc.read_gpu_percent_metric = original_gpu_pct
        svc.read_gpu_temp_metric = original_gpu_temp
        svc.read_gpu_power_metric = original_gpu_power
        svc.socket.gethostname = original_host
        svc.time.time = original_time

    assert payload["v"] == 1
    assert payload["host"] == "linux-main"
    assert payload["cpu"]["pct"]["value_raw"] == 12.4
    assert payload["cpu"]["power_w"]["valid"] is False
    assert payload["gpu"]["power_w"]["value_display"] == 36.4
    assert payload["mem"]["used_b"]["value_raw"] == 9123454976


def test_build_dashboard_from_store_updates_stale_ms() -> None:
    import app.services.dashboard_service as svc

    snapshot = build_dashboard_live()
    store = SnapshotStore()
    store.set_snapshot(snapshot, tick_ms=1_000)

    original_time = svc.time.time
    svc.time.time = lambda: 1.350
    try:
        payload = build_dashboard_from_store(store).model_dump()
    finally:
        svc.time.time = original_time

    assert payload["state"]["ok"] is True
    assert payload["state"]["stale_ms"] == 350


def test_build_dashboard_from_store_raises_when_empty() -> None:
    store = SnapshotStore()

    try:
        build_dashboard_from_store(store)
    except SnapshotUnavailableError:
        return

    raise AssertionError("SnapshotUnavailableError not raised")


def test_gpu_pct_uses_median_then_ema() -> None:
    import app.services.dashboard_service as svc

    original_cpu_pct = svc.read_cpu_percent_metric
    original_cpu_temp = svc.read_cpu_temp_metric
    original_cpu_power = svc.read_cpu_power_metric
    original_mem = svc.read_memory
    original_gpu_pct = svc.read_gpu_percent_metric
    original_gpu_temp = svc.read_gpu_temp_metric
    original_gpu_power = svc.read_gpu_power_metric

    gpu_values = iter([90.0, 30.0, 92.0])

    svc.read_cpu_percent_metric = lambda: _reading(10.0, source="cpu", unit="percent")
    svc.read_cpu_temp_metric = lambda: _reading(40.0, source="cpu-temp", unit="celsius")
    svc.read_cpu_power_metric = lambda: _reading(None, source="cpu-power", unit="watt", read_ok=False, error="missing")
    svc.read_memory = lambda: (1, 1, 1.0)
    svc.read_gpu_percent_metric = lambda: _reading(next(gpu_values), source="gpu", unit="percent")
    svc.read_gpu_temp_metric = lambda: _reading(60.0, source="gpu-temp", unit="celsius")
    svc.read_gpu_power_metric = lambda: _reading(120.0, source="gpu-power", unit="watt")

    try:
        s1 = build_dashboard_live(display_ema_alpha=0.37).model_dump()
        s2 = build_dashboard_live(display_ema_alpha=0.37).model_dump()
        s3 = build_dashboard_live(display_ema_alpha=0.37).model_dump()
    finally:
        svc.read_cpu_percent_metric = original_cpu_pct
        svc.read_cpu_temp_metric = original_cpu_temp
        svc.read_cpu_power_metric = original_cpu_power
        svc.read_memory = original_mem
        svc.read_gpu_percent_metric = original_gpu_pct
        svc.read_gpu_temp_metric = original_gpu_temp
        svc.read_gpu_power_metric = original_gpu_power

    assert s1["gpu"]["pct"]["value_raw"] == 90.0
    assert round(s1["gpu"]["pct"]["value_display"], 3) == 90.0
    assert s2["gpu"]["pct"]["value_raw"] == 30.0
    assert round(s2["gpu"]["pct"]["value_display"], 3) == 78.9
    assert s3["gpu"]["pct"]["value_raw"] == 92.0
    assert round(s3["gpu"]["pct"]["value_display"], 3) == 83.007
