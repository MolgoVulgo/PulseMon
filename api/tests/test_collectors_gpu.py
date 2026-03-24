from pathlib import Path

from app.collectors import gpu as gpu_mod


def _write(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8")


def test_gpu_usage_reads_gpu_busy_percent(tmp_path: Path) -> None:
    drm = tmp_path / "drm"
    _write(drm / "card0" / "device" / "vendor", "0x1002\n")
    _write(drm / "card0" / "device" / "gpu_busy_percent", "7\n")

    original = gpu_mod.DRM_CLASS_PATH
    gpu_mod.DRM_CLASS_PATH = drm
    try:
        assert gpu_mod.read_gpu_percent() == 7.0
    finally:
        gpu_mod.DRM_CLASS_PATH = original


def test_gpu_temp_reads_temp1_input_mc(tmp_path: Path) -> None:
    drm = tmp_path / "drm"
    _write(drm / "card0" / "device" / "vendor", "0x1002\n")
    _write(drm / "card0" / "device" / "hwmon" / "hwmon2" / "temp1_input", "39000\n")

    original = gpu_mod.DRM_CLASS_PATH
    gpu_mod.DRM_CLASS_PATH = drm
    try:
        assert gpu_mod.read_gpu_temp_c() == 39.0
    finally:
        gpu_mod.DRM_CLASS_PATH = original


def test_gpu_power_reads_power1_average_uw(tmp_path: Path) -> None:
    drm = tmp_path / "drm"
    _write(drm / "card0" / "device" / "vendor", "0x1002\n")
    _write(drm / "card0" / "device" / "hwmon" / "hwmon2" / "power1_average", "36400000\n")

    original = gpu_mod.DRM_CLASS_PATH
    gpu_mod.DRM_CLASS_PATH = drm
    try:
        assert gpu_mod.read_gpu_power_w() == 36.4
    finally:
        gpu_mod.DRM_CLASS_PATH = original


def test_gpu_probe_device_path(tmp_path: Path) -> None:
    drm = tmp_path / "drm"
    _write(drm / "card2" / "device" / "vendor", "0x1002\n")

    original = gpu_mod.DRM_CLASS_PATH
    gpu_mod.DRM_CLASS_PATH = drm
    try:
        path = gpu_mod.probe_gpu_device_path()
        assert path is not None
        assert path.endswith("/card2/device")
    finally:
        gpu_mod.DRM_CLASS_PATH = original


def test_gpu_collectors_return_none_when_no_amd_card(tmp_path: Path) -> None:
    drm = tmp_path / "drm"
    _write(drm / "card0" / "device" / "vendor", "0x8086\n")

    original = gpu_mod.DRM_CLASS_PATH
    gpu_mod.DRM_CLASS_PATH = drm
    try:
        assert gpu_mod.read_gpu_percent() is None
        assert gpu_mod.read_gpu_temp_c() is None
        assert gpu_mod.read_gpu_power_w() is None
        assert gpu_mod.probe_gpu_device_path() is None
    finally:
        gpu_mod.DRM_CLASS_PATH = original
