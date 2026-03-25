from __future__ import annotations

import json
import os
from pathlib import Path
import time

import psutil
from app.services.filters import MedianEmaFilter

from app.collectors import (
    probe_gpu_device_path,
    probe_gpu_mappings,
    read_cpu_percent_metric,
    read_cpu_power_metric,
    read_cpu_temp_metric,
    read_gpu_mem_percent_metric,
    read_gpu_percent_metric,
    read_gpu_power_metric,
    read_gpu_temp_metric,
)


RAW_METRIC_READERS = {
    "cpu.pct": read_cpu_percent_metric,
    "cpu.temp_c": read_cpu_temp_metric,
    "cpu.power_w": read_cpu_power_metric,
    "gpu.pct": read_gpu_percent_metric,
    "gpu.mem_pct": read_gpu_mem_percent_metric,
    "gpu.temp_c": read_gpu_temp_metric,
    "gpu.power_w": read_gpu_power_metric,
}


def probe_sources() -> dict[str, object]:
    temperatures = psutil.sensors_temperatures(fahrenheit=False) or {}
    cpu_temp_labels = sorted(
        {
            (getattr(entry, "label", "") or "").strip()
            for entries in temperatures.values()
            for entry in entries
            if (getattr(entry, "label", "") or "").strip()
        }
    )

    return {
        "cpu_temp_labels": cpu_temp_labels,
        "cpu_temp_tdie": any(label.lower() == "tdie" for label in cpu_temp_labels),
        "cpu_temp_tctl": any(label.lower() == "tctl" for label in cpu_temp_labels),
        "gpu_selected_device_path": probe_gpu_device_path(),
        "gpu_mappings": probe_gpu_mappings(),
        "drm_gpu_busy_candidates": _probe_drm_gpu_busy_candidates(),
        "hwmon_power_candidates": _probe_hwmon_power_candidates(),
        "amdgpu_pm_info": _probe_amdgpu_pm_info(),
        "raw_metric_probes": {metric: _reading_to_probe(reader()) for metric, reader in RAW_METRIC_READERS.items()},
    }



def capture_raw_metrics(*, duration_s: int = 60, sample_hz: float = 10.0, output_path: str) -> dict[str, object]:
    if sample_hz <= 0:
        raise ValueError("sample_hz must be > 0")
    if duration_s <= 0:
        raise ValueError("duration_s must be > 0")

    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)

    start_mono = time.monotonic()
    stop_at = start_mono + float(duration_s)
    step_s = 1.0 / sample_hz
    next_tick = start_mono
    rows = 0

    with out.open("w", encoding="utf-8") as handle:
        while True:
            now_mono = time.monotonic()
            if now_mono >= stop_at:
                break

            tick_index = rows
            for metric_name, reader in RAW_METRIC_READERS.items():
                reading = reader()
                row = {
                    "metric": metric_name,
                    "tick_index": tick_index,
                    "timestamp_mono_s": round(reading.sampled_at_monotonic_s, 6),
                    "timestamp_unix_ms": reading.sampled_at_unix_ms,
                    "source": reading.source,
                    "unit": reading.unit,
                    "raw_value": reading.raw_value,
                    "read_ok": reading.read_ok,
                    "read_error": reading.read_error,
                    "estimated": reading.estimated,
                }
                handle.write(json.dumps(row, ensure_ascii=True) + "\n")

            rows += 1
            next_tick += step_s
            sleep_s = max(0.0, next_tick - time.monotonic())
            if sleep_s:
                time.sleep(sleep_s)

    end_mono = time.monotonic()
    return {
        "output_path": str(out),
        "duration_s": duration_s,
        "sample_hz": sample_hz,
        "rows": rows,
        "metrics_per_row": len(RAW_METRIC_READERS),
        "records": rows * len(RAW_METRIC_READERS),
        "elapsed_s": round(end_mono - start_mono, 3),
    }


def capture_gpu_raw_vs_display(
    *,
    duration_s: int = 60,
    sample_hz: float = 10.0,
    alpha: float = 0.25,
    output_path: str,
) -> dict[str, object]:
    if sample_hz <= 0:
        raise ValueError("sample_hz must be > 0")
    if duration_s <= 0:
        raise ValueError("duration_s must be > 0")

    filt = MedianEmaFilter(alpha=alpha, median_window=3)
    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)

    start_mono = time.monotonic()
    stop_at = start_mono + float(duration_s)
    step_s = 1.0 / sample_hz
    next_tick = start_mono
    rows = 0

    with out.open("w", encoding="utf-8") as handle:
        while True:
            if time.monotonic() >= stop_at:
                break

            reading = read_gpu_percent_metric()
            display_value = filt.update(
                float(reading.raw_value) if reading.raw_value is not None else None,
                valid=reading.valid,
            )
            row = {
                "metric": "gpu.pct",
                "tick_index": rows,
                "timestamp_mono_s": round(reading.sampled_at_monotonic_s, 6),
                "timestamp_unix_ms": reading.sampled_at_unix_ms,
                "sampled_at": reading.sampled_at_unix_ms,
                "source": reading.source,
                "value_raw": reading.raw_value,
                "value_display": display_value,
                "valid": reading.valid,
                "read_ok": reading.read_ok,
                "read_error": reading.read_error,
                "ema_alpha": alpha,
                "median_window": 3,
            }
            handle.write(json.dumps(row, ensure_ascii=True) + "\n")
            rows += 1

            next_tick += step_s
            sleep_s = max(0.0, next_tick - time.monotonic())
            if sleep_s:
                time.sleep(sleep_s)

    end_mono = time.monotonic()
    return {
        "output_path": str(out),
        "duration_s": duration_s,
        "sample_hz": sample_hz,
        "rows": rows,
        "records": rows,
        "alpha": alpha,
        "elapsed_s": round(end_mono - start_mono, 3),
    }



def _reading_to_probe(reading) -> dict[str, object]:
    return {
        "source": reading.source,
        "unit": reading.unit,
        "raw_value": reading.raw_value,
        "read_ok": reading.read_ok,
        "read_error": reading.read_error,
        "estimated": reading.estimated,
    }


def _probe_drm_gpu_busy_candidates() -> list[dict[str, object]]:
    candidates: list[dict[str, object]] = []
    for path in sorted(Path("/sys/class/drm").glob("card[0-9]*/device/gpu_busy_percent")):
        candidates.append({"path": str(path), "exists": path.exists(), "readable": os.access(path, os.R_OK)})
    return candidates


def _probe_hwmon_power_candidates() -> list[dict[str, object]]:
    candidates: list[dict[str, object]] = []
    for path in sorted(Path("/sys/class/hwmon").glob("hwmon*/power1_average")):
        candidates.append({"path": str(path), "exists": path.exists(), "readable": os.access(path, os.R_OK)})
    return candidates


def _probe_amdgpu_pm_info() -> dict[str, object]:
    debug_root = Path("/sys/kernel/debug/dri")
    if not debug_root.exists():
        return {
            "root": str(debug_root),
            "available": False,
            "readable": os.access(debug_root, os.R_OK),
            "files": [],
            "error": "debugfs_unavailable_or_not_mounted",
        }

    if not os.access(debug_root, os.R_OK):
        return {
            "root": str(debug_root),
            "available": False,
            "readable": False,
            "files": [],
            "error": "permission_denied",
        }

    files = sorted(str(path) for path in debug_root.glob("*/amdgpu_pm_info"))
    return {
        "root": str(debug_root),
        "available": bool(files),
        "readable": True,
        "files": files,
        "error": None if files else "amdgpu_pm_info_not_found",
    }
