from app.services.dashboard_service import (
    SnapshotUnavailableError,
    build_dashboard_from_store,
    build_dashboard_live,
)
from app.store import SnapshotStore


def test_build_dashboard_live_contract_minimal() -> None:
    import app.services.dashboard_service as svc

    original_cpu_pct = svc.read_cpu_percent
    original_cpu_temp = svc.read_cpu_temp_c
    original_mem = svc.read_memory
    original_gpu_pct = svc.read_gpu_percent
    original_gpu_temp = svc.read_gpu_temp_c
    original_gpu_power = svc.read_gpu_power_w
    original_host = svc.socket.gethostname
    original_time = svc.time.time

    svc.read_cpu_percent = lambda: 12.4
    svc.read_cpu_temp_c = lambda: 43.8
    svc.read_memory = lambda: (9123454976, 34359738368, 26.6)
    svc.read_gpu_percent = lambda: 7.0
    svc.read_gpu_temp_c = lambda: 39.0
    svc.read_gpu_power_w = lambda: 36.4
    svc.socket.gethostname = lambda: "linux-main"
    svc.time.time = lambda: 1774256402

    try:
        payload = build_dashboard_live().model_dump()
    finally:
        svc.read_cpu_percent = original_cpu_pct
        svc.read_cpu_temp_c = original_cpu_temp
        svc.read_memory = original_mem
        svc.read_gpu_percent = original_gpu_pct
        svc.read_gpu_temp_c = original_gpu_temp
        svc.read_gpu_power_w = original_gpu_power
        svc.socket.gethostname = original_host
        svc.time.time = original_time

    assert payload == {
        "v": 1,
        "ts": 1774256402,
        "host": "linux-main",
        "cpu": {
            "pct": 12.4,
            "temp_c": 43.8,
            "power_w": None,
        },
        "mem": {
            "used_b": 9123454976,
            "total_b": 34359738368,
            "pct": 26.6,
        },
        "gpu": {
            "pct": 7.0,
            "temp_c": 39.0,
            "power_w": 36.4,
        },
        "state": {
            "ok": True,
            "stale_ms": 0,
        },
    }


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
