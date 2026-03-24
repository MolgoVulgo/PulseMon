from app.services.meta_service import build_meta


def test_build_meta_contract() -> None:
    import app.services.meta_service as svc

    original_host = svc.socket.gethostname
    svc.socket.gethostname = lambda: "linux-main"
    try:
        payload = build_meta().model_dump()
    finally:
        svc.socket.gethostname = original_host

    assert payload["v"] == 1
    assert payload["host"] == "linux-main"
    assert payload["metrics"] == [
        "cpu.pct",
        "cpu.temp_c",
        "cpu.power_w",
        "mem.used_b",
        "mem.total_b",
        "mem.pct",
        "gpu.pct",
        "gpu.temp_c",
        "gpu.power_w",
    ]
    assert payload["history_series"] == [
        "cpu_pct",
        "cpu_temp_c",
        "gpu_pct",
        "gpu_temp_c",
    ]
