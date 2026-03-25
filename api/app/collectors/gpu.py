from __future__ import annotations

from dataclasses import asdict, dataclass
import os
from pathlib import Path
import re

from .metric import MetricReading, failed_reading, ok_reading


DRM_CLASS_PATH = Path("/sys/class/drm")
CARD_NAME_RE = re.compile(r"^card[0-9]+$")
AMD_VENDOR_ID = "0x1002"


@dataclass(frozen=True)
class GpuMapping:
    card_name: str
    card_path: str
    device_path: str
    pci_slot: str | None
    vendor_id: str
    device_id: str | None
    class_id: str | None
    driver_module: str | None
    hwmon_path: str | None
    has_gpu_busy_percent: bool
    has_mem_busy_percent: bool
    has_power1_average: bool



def read_gpu_percent_metric() -> MetricReading:
    mapping = _select_amd_gpu_mapping()
    if mapping is None:
        return failed_reading(source="/sys/class/drm/card*/device/gpu_busy_percent", unit="percent", error="no_amd_gpu")

    return _read_float_metric(Path(mapping.device_path) / "gpu_busy_percent", unit="percent")



def read_gpu_mem_percent_metric() -> MetricReading:
    mapping = _select_amd_gpu_mapping()
    if mapping is None:
        return failed_reading(source="/sys/class/drm/card*/device/mem_busy_percent", unit="percent", error="no_amd_gpu")

    return _read_float_metric(Path(mapping.device_path) / "mem_busy_percent", unit="percent")



def read_gpu_temp_metric() -> MetricReading:
    mapping = _select_amd_gpu_mapping()
    if mapping is None:
        return failed_reading(source="/sys/class/drm/card*/device/hwmon/hwmon*/temp*_input", unit="celsius", error="no_amd_gpu")

    hwmon_path = Path(mapping.hwmon_path) if mapping.hwmon_path else None
    if hwmon_path is None:
        return failed_reading(source=f"{mapping.device_path}/hwmon", unit="celsius", error="hwmon_missing")

    temp_candidates = _find_temp_candidates(hwmon_path)
    if not temp_candidates:
        return failed_reading(source=f"{hwmon_path}/temp*_input", unit="celsius", error="temp_sensor_missing")

    preferred = _choose_preferred_temp_candidate(temp_candidates)
    source_path = preferred["path"]
    read = _read_float_metric(source_path, unit="millicelsius")
    if not read.valid:
        return failed_reading(source=str(source_path), unit="celsius", error=read.read_error or "read_error")

    assert read.raw_value is not None
    return ok_reading(
        value=float(read.raw_value) / 1000.0,
        source=f"{source_path}:{preferred['label']}",
        unit="celsius",
    )



def read_gpu_power_metric() -> MetricReading:
    mapping = _select_amd_gpu_mapping()
    if mapping is None:
        return failed_reading(source="/sys/class/drm/card*/device/hwmon/hwmon*/power1_average", unit="watt", error="no_amd_gpu")

    hwmon_path = Path(mapping.hwmon_path) if mapping.hwmon_path else None
    if hwmon_path is None:
        return failed_reading(source=f"{mapping.device_path}/hwmon", unit="watt", error="hwmon_missing")

    for filename in ("power1_average", "power1_input"):
        raw = _read_float_metric(hwmon_path / filename, unit="microwatt")
        if raw.valid:
            assert raw.raw_value is not None
            return ok_reading(value=float(raw.raw_value) / 1_000_000.0, source=raw.source, unit="watt")
        if raw.read_error and raw.read_error != "source_missing":
            return failed_reading(source=raw.source, unit="watt", error=raw.read_error)

    return failed_reading(source=f"{hwmon_path}/power1_average", unit="watt", error="source_missing")



def read_gpu_percent() -> float | None:
    metric = read_gpu_percent_metric()
    return float(metric.raw_value) if metric.raw_value is not None else None



def read_gpu_temp_c() -> float | None:
    metric = read_gpu_temp_metric()
    return float(metric.raw_value) if metric.raw_value is not None else None



def read_gpu_power_w() -> float | None:
    metric = read_gpu_power_metric()
    return float(metric.raw_value) if metric.raw_value is not None else None



def probe_gpu_device_path() -> str | None:
    mapping = _select_amd_gpu_mapping()
    if mapping is None:
        return None
    return mapping.device_path



def probe_gpu_mappings() -> list[dict[str, object]]:
    return [asdict(mapping) for mapping in list_amd_gpu_mappings()]



def list_amd_gpu_mappings() -> list[GpuMapping]:
    if not DRM_CLASS_PATH.exists():
        return []

    mappings: list[GpuMapping] = []
    for card in sorted(DRM_CLASS_PATH.iterdir()):
        if not card.is_dir() or not CARD_NAME_RE.match(card.name):
            continue

        device_link = card / "device"
        if not device_link.exists():
            continue

        device = device_link.resolve()
        vendor = _read_text(device / "vendor")
        if vendor is None or vendor.lower() != AMD_VENDOR_ID:
            continue

        hwmon_path: str | None = None
        hwmon_dir = device / "hwmon"
        if hwmon_dir.exists():
            hwmon_entries = sorted(hwmon_dir.glob("hwmon*"))
            if hwmon_entries:
                hwmon_path = str(hwmon_entries[0].resolve())

        mapping = GpuMapping(
            card_name=card.name,
            card_path=str(card),
            device_path=str(device),
            pci_slot=_read_pci_slot(device),
            vendor_id=vendor.lower(),
            device_id=_read_text(device / "device"),
            class_id=_read_text(device / "class"),
            driver_module=_read_driver_module(device),
            hwmon_path=hwmon_path,
            has_gpu_busy_percent=(device / "gpu_busy_percent").exists(),
            has_mem_busy_percent=(device / "mem_busy_percent").exists(),
            has_power1_average=(Path(hwmon_path) / "power1_average").exists() if hwmon_path else False,
        )
        mappings.append(mapping)

    return mappings



def _select_amd_gpu_mapping() -> GpuMapping | None:
    mappings = list_amd_gpu_mappings()
    if not mappings:
        return None

    forced_slot = os.getenv("STATS_GPU_PCI_SLOT")
    if forced_slot:
        for mapping in mappings:
            if mapping.pci_slot == forced_slot:
                return mapping

    for mapping in mappings:
        if mapping.has_gpu_busy_percent:
            return mapping

    return mappings[0]



def _read_text(path: Path) -> str | None:
    if not path.exists():
        return None
    try:
        return path.read_text(encoding="utf-8").strip()
    except OSError:
        return None



def _read_driver_module(device: Path) -> str | None:
    module_link = device / "driver" / "module"
    if not module_link.exists():
        return None
    try:
        return module_link.resolve().name
    except OSError:
        return None



def _read_pci_slot(device: Path) -> str | None:
    uevent = device / "uevent"
    if not uevent.exists():
        return device.name if ":" in device.name else None

    try:
        lines = uevent.read_text(encoding="utf-8").splitlines()
    except OSError:
        return device.name if ":" in device.name else None

    for line in lines:
        if line.startswith("PCI_SLOT_NAME="):
            return line.split("=", maxsplit=1)[1]
    return device.name if ":" in device.name else None



def _read_float_metric(path: Path, unit: str) -> MetricReading:
    if not path.exists():
        return failed_reading(source=str(path), unit=unit, error="source_missing")

    try:
        return ok_reading(value=float(path.read_text(encoding="utf-8").strip()), source=str(path), unit=unit)
    except ValueError:
        return failed_reading(source=str(path), unit=unit, error="invalid_value")
    except OSError as exc:
        return failed_reading(source=str(path), unit=unit, error=f"read_error:{type(exc).__name__}:{exc}")



def _find_temp_candidates(hwmon_path: Path) -> list[dict[str, object]]:
    candidates: list[dict[str, object]] = []
    for temp_input in sorted(hwmon_path.glob("temp*_input")):
        prefix = temp_input.name.split("_", maxsplit=1)[0]
        label_file = hwmon_path / f"{prefix}_label"
        if label_file.exists():
            try:
                label = label_file.read_text(encoding="utf-8").strip().lower()
            except OSError:
                label = "unknown"
        else:
            label = "unknown"
        candidates.append({"path": temp_input, "label": label})
    return candidates



def _choose_preferred_temp_candidate(candidates: list[dict[str, object]]) -> dict[str, object]:
    priority = _gpu_temp_label_priority()
    for wanted in priority:
        for candidate in candidates:
            if candidate["label"] == wanted:
                return candidate
    return candidates[0]


def _gpu_temp_label_priority() -> list[str]:
    raw = os.getenv("STATS_GPU_TEMP_LABEL_PRIORITY", "edge,junction,mem,unknown")
    labels = [part.strip().lower() for part in raw.split(",") if part.strip()]
    if "unknown" not in labels:
        labels.append("unknown")
    return labels
