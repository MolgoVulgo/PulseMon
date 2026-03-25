from pydantic import BaseModel, ConfigDict


class HealthResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    ts: int
    ok: bool
    service: str


class MetricValue(BaseModel):
    model_config = ConfigDict(extra="forbid")

    value_raw: float | int | None
    value_display: float | int | None
    source: str
    unit: str
    sampled_at: int | None
    estimated: bool
    valid: bool


class CpuSnapshot(BaseModel):
    model_config = ConfigDict(extra="forbid")

    pct: MetricValue
    temp_c: MetricValue
    power_w: MetricValue


class MemSnapshot(BaseModel):
    model_config = ConfigDict(extra="forbid")

    used_b: MetricValue
    total_b: MetricValue
    pct: MetricValue


class GpuSnapshot(BaseModel):
    model_config = ConfigDict(extra="forbid")

    pct: MetricValue
    temp_c: MetricValue
    power_w: MetricValue


class SnapshotState(BaseModel):
    model_config = ConfigDict(extra="forbid")

    ok: bool
    stale_ms: int


class DashboardResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    ts: int
    host: str
    cpu: CpuSnapshot
    mem: MemSnapshot
    gpu: GpuSnapshot
    state: SnapshotState


class HistorySeries(BaseModel):
    model_config = ConfigDict(extra="forbid")

    cpu_pct: list[float | None]
    cpu_temp_c: list[float | None]
    gpu_pct: list[float | None]
    gpu_temp_c: list[float | None]


class HistoryResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    ts: int
    ts_ms: list[int]
    window_s: int
    step_s: int
    series: HistorySeries


class MetaResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    host: str
    metrics: list[str]
    history_series: list[str]


class ErrorResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    error: str
    field: str | None = None


V1_METRICS = [
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

V1_HISTORY_SERIES = [
    "cpu_pct",
    "cpu_temp_c",
    "gpu_pct",
    "gpu_temp_c",
]
