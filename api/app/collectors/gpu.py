from pathlib import Path

DRM_CLASS_PATH = Path("/sys/class/drm")


def read_gpu_percent() -> float | None:
    device = _find_amd_gpu_device()
    if device is None:
        return None

    return _read_float(device / "gpu_busy_percent")


def read_gpu_temp_c() -> float | None:
    device = _find_amd_gpu_device()
    if device is None:
        return None

    for hwmon_dir in sorted((device / "hwmon").glob("hwmon*")):
        temp_mc = _read_float(hwmon_dir / "temp1_input")
        if temp_mc is not None:
            return temp_mc / 1000.0

    return None


def read_gpu_power_w() -> float | None:
    device = _find_amd_gpu_device()
    if device is None:
        return None

    for hwmon_dir in sorted((device / "hwmon").glob("hwmon*")):
        for filename in ("power1_average", "power1_input"):
            value_uw = _read_float(hwmon_dir / filename)
            if value_uw is not None:
                return value_uw / 1_000_000.0

    return None


def probe_gpu_device_path() -> str | None:
    device = _find_amd_gpu_device()
    if device is None:
        return None
    return str(device)


def _find_amd_gpu_device() -> Path | None:
    if not DRM_CLASS_PATH.exists():
        return None

    for card in sorted(DRM_CLASS_PATH.glob("card*")):
        device = card / "device"
        vendor = (device / "vendor").read_text(encoding="utf-8").strip().lower() if (device / "vendor").exists() else ""
        if vendor == "0x1002":
            return device

    return None


def _read_float(path: Path) -> float | None:
    if not path.exists():
        return None

    try:
        return float(path.read_text(encoding="utf-8").strip())
    except ValueError:
        return None
