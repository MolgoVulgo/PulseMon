from pathlib import Path

from app.collectors import fans as fans_mod


def _write(path: Path, data: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(data, encoding="utf-8")


def test_list_fan_channels_reads_hwmon_rpm_and_pwm(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "1132\n")
    _write(hwmon / "hwmon0" / "fan1_label", "CPU Fan\n")
    _write(hwmon / "hwmon0" / "pwm1", "128\n")
    _write(hwmon / "hwmon0" / "pwm1_max", "255\n")

    original = fans_mod.HWMON_CLASS_PATH
    fans_mod.HWMON_CLASS_PATH = hwmon
    try:
        channels = fans_mod.list_fan_channels()
    finally:
        fans_mod.HWMON_CLASS_PATH = original

    assert len(channels) == 1
    channel = channels[0]
    assert channel.hwmon_name == "it8628"
    assert channel.channel == "fan1"
    assert channel.rpm == 1132
    assert channel.pwm_pct == 50
    assert channel.valid is True
    assert channel.group == "motherboard"


def test_list_fan_channels_marks_amdgpu_group(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    _write(hwmon / "hwmon7" / "name", "amdgpu\n")
    _write(hwmon / "hwmon7" / "fan1_input", "0\n")

    original = fans_mod.HWMON_CLASS_PATH
    fans_mod.HWMON_CLASS_PATH = hwmon
    try:
        channels = fans_mod.list_fan_channels()
    finally:
        fans_mod.HWMON_CLASS_PATH = original

    assert len(channels) == 1
    assert channels[0].group == "gpu"
    assert channels[0].valid is True
    assert channels[0].rpm == 0
