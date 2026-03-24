import socket
import time

from app.collectors import (
    read_cpu_percent,
    read_cpu_temp_c,
    read_gpu_percent,
    read_gpu_power_w,
    read_gpu_temp_c,
    read_memory,
)
from app.models import (
    CpuSnapshot,
    DashboardResponse,
    GpuSnapshot,
    MemSnapshot,
    SnapshotState,
)
from app.store import SnapshotStore


class SnapshotUnavailableError(RuntimeError):
    pass


def build_dashboard_live() -> DashboardResponse:
    cpu_pct = read_cpu_percent()
    cpu_temp_c = read_cpu_temp_c()
    mem_used_b, mem_total_b, mem_pct = read_memory()
    gpu_pct = read_gpu_percent()
    gpu_temp_c = read_gpu_temp_c()
    gpu_power_w = read_gpu_power_w()

    return DashboardResponse(
        v=1,
        ts=int(time.time()),
        host=socket.gethostname(),
        cpu=CpuSnapshot(
            pct=cpu_pct,
            temp_c=cpu_temp_c,
        ),
        mem=MemSnapshot(
            used_b=mem_used_b,
            total_b=mem_total_b,
            pct=mem_pct,
        ),
        gpu=GpuSnapshot(
            pct=gpu_pct,
            temp_c=gpu_temp_c,
            power_w=gpu_power_w,
        ),
        state=SnapshotState(
            ok=True,
            stale_ms=0,
        ),
    )


def build_dashboard_from_store(snapshot_store: SnapshotStore) -> DashboardResponse:
    snapshot = snapshot_store.get_snapshot()
    if snapshot is None:
        raise SnapshotUnavailableError("snapshot_unavailable")

    stale_ms = snapshot_store.get_stale_ms()
    snapshot.state.ok = True
    snapshot.state.stale_ms = stale_ms
    return snapshot
