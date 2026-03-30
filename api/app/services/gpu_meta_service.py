import socket

from app.collectors import read_gpu_fan_percent_metric, read_gpu_name
from app.models import GpuMetaCaps, GpuMetaResponse, V1_GPU_HISTORY_SERIES, V1_GPU_METRICS


def build_gpu_meta() -> GpuMetaResponse:
    fan_pct = read_gpu_fan_percent_metric()
    return GpuMetaResponse(
        v=1,
        host=socket.gethostname(),
        gpu_name=read_gpu_name(),
        metrics=V1_GPU_METRICS,
        history_series=V1_GPU_HISTORY_SERIES,
        caps=GpuMetaCaps(
            fan_pct=fan_pct.valid,
            hotspot_temp=False,
            mem_temp=False,
        ),
    )
