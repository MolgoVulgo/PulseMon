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


class GpuExtendedSnapshot(BaseModel):
    model_config = ConfigDict(extra="forbid")

    pct: MetricValue
    core_clock_mhz: MetricValue
    mem_clock_mhz: MetricValue
    vram_used_b: MetricValue
    vram_total_b: MetricValue
    vram_pct: MetricValue
    temp_c: MetricValue
    power_w: MetricValue
    fan_rpm: MetricValue
    fan_pct: MetricValue


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


class GpuDashboardResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    ts: int
    host: str
    gpu: GpuExtendedSnapshot
    state: SnapshotState


class GpuHistorySeries(BaseModel):
    model_config = ConfigDict(extra="forbid")

    gpu_pct: list[float | None]
    gpu_core_clock_mhz: list[float | None]
    gpu_vram_used_b: list[float | None]
    gpu_temp_c: list[float | None]
    gpu_power_w: list[float | None]
    gpu_mem_clock_mhz: list[float | None]
    gpu_fan_rpm: list[float | None]


class GpuHistoryResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    ts: int
    ts_ms: list[int]
    window_s: int
    step_s: int
    series: GpuHistorySeries


class GpuMetaCaps(BaseModel):
    model_config = ConfigDict(extra="forbid")

    fan_pct: bool
    hotspot_temp: bool
    mem_temp: bool


class GpuMetaResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    host: str
    gpu_name: str
    metrics: list[str]
    history_series: list[str]
    caps: GpuMetaCaps


class FanItem(BaseModel):
    model_config = ConfigDict(extra="forbid")

    label: str
    role: str
    rpm: int | None
    pwm_pct: int | None


class FansDashboardResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    ts: int
    host: str
    fans: list[FanItem]


class FanMappingInfo(BaseModel):
    model_config = ConfigDict(extra="forbid")

    configured: bool
    label: str | None
    role: str | None
    order: int | None
    enabled: bool | None


class FanChannelInfo(BaseModel):
    model_config = ConfigDict(extra="forbid")

    channel: str
    hwmon_name: str
    hwmon_path: str
    source: str
    group: str
    label: str | None
    rpm: int | None
    pwm_pct: int | None
    connected: bool
    valid: bool
    error: str | None
    mapping: FanMappingInfo


class FansMetaResponse(BaseModel):
    model_config = ConfigDict(extra="forbid")

    v: int
    ts: int
    host: str
    channels: list[FanChannelInfo]
    display_labels: list[str]


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

V1_GPU_METRICS = [
    "gpu.pct",
    "gpu.core_clock_mhz",
    "gpu.mem_clock_mhz",
    "gpu.vram_used_b",
    "gpu.vram_total_b",
    "gpu.vram_pct",
    "gpu.temp_c",
    "gpu.power_w",
    "gpu.fan_rpm",
    "gpu.fan_pct",
]

V1_GPU_HISTORY_SERIES = [
    "gpu_pct",
    "gpu_core_clock_mhz",
    "gpu_vram_used_b",
    "gpu_temp_c",
    "gpu_power_w",
    "gpu_mem_clock_mhz",
    "gpu_fan_rpm",
]

V1_FANS_DASHBOARD_FIELDS = [
    "label",
    "role",
    "rpm",
    "pwm_pct",
]
