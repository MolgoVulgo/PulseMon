# AMD GPU monitoring

PulseMon targets AMD GPU telemetry through Linux DRM, hwmon and amdgpu-exposed sysfs paths.

## Metrics

The GPU monitoring scope includes:

- GPU usage percentage;
- GPU temperature;
- GPU power;
- display-ready smoothed values;
- raw values for diagnostics;
- dedicated GPU dashboard data;
- bounded GPU history.

## Source selection

The backend should identify the target AMD GPU through `/sys/class/drm/card*/device` and related hwmon entries. `STATS_GPU_PCI_SLOT` can force a specific device when automatic selection is ambiguous.

Temperature label priority is controlled by `STATS_GPU_TEMP_LABEL_PRIORITY`, with typical labels such as edge, junction and memory.

## Smoothing

GPU usage can be visually noisy. The backend can expose both raw and display values. The display value uses smoothing to stabilize the UI without hiding raw diagnostics.

The firmware should use display values for rendering and raw values only when explicitly needed for diagnostics.

## GPU API

The GPU-specific endpoints are:

- `GET /api/v1/gpu/dashboard`;
- `GET /api/v1/gpu/history`;
- `GET /api/v1/gpu/meta`.

The main dashboard can also contain GPU summary fields.

## Failure policy

If one GPU metric is unavailable:

- keep the metric field present;
- mark it invalid or nullable;
- keep the rest of the GPU payload usable;
- log the selected path and failure reason in diagnostics mode;
- do not synthesize fake telemetry.

## Detailed GPU payload

The GPU dashboard can expose the following metric envelopes:

- `gpu.pct`;
- `gpu.core_clock_mhz`;
- `gpu.mem_clock_mhz`;
- `gpu.vram_used_b`;
- `gpu.vram_total_b`;
- `gpu.vram_pct`;
- `gpu.temp_c`;
- `gpu.power_w`;
- `gpu.fan_rpm`;
- `gpu.fan_pct`.

The GPU history can expose:

- `gpu_pct`;
- `gpu_core_clock_mhz`;
- `gpu_vram_used_b`;
- `gpu_temp_c`;
- `gpu_power_w`;
- `gpu_mem_clock_mhz`;
- `gpu_fan_rpm`.

If the GPU history store is empty, the backend can perform a live warmup read before answering. Unavailable series must return `null` points rather than being silently removed.
