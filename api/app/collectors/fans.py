from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re


HWMON_CLASS_PATH = Path("/sys/class/hwmon")
_FAN_INPUT_RE = re.compile(r"^fan([0-9]+)_input$")


@dataclass(frozen=True)
class FanChannelReading:
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


def list_fan_channels() -> list[FanChannelReading]:
    if not HWMON_CLASS_PATH.exists():
        return []

    channels: list[FanChannelReading] = []
    for hwmon_dir in sorted(HWMON_CLASS_PATH.glob("hwmon*")):
        hwmon_name = _read_text(hwmon_dir / "name") or "unknown"
        group = "gpu" if hwmon_name.lower() == "amdgpu" else "motherboard"
        hwmon_path = str(hwmon_dir.resolve())

        for fan_input in sorted(hwmon_dir.glob("fan*_input")):
            match = _FAN_INPUT_RE.match(fan_input.name)
            if match is None:
                continue
            index = int(match.group(1))
            channel = f"fan{index}"
            rpm, rpm_error = _read_int(fan_input)
            pwm_pct = _read_pwm_percent(hwmon_dir, index=index)
            label = _read_text(hwmon_dir / f"fan{index}_label")

            channels.append(
                FanChannelReading(
                    channel=channel,
                    hwmon_name=hwmon_name,
                    hwmon_path=hwmon_path,
                    source=str(fan_input),
                    group=group,
                    label=label,
                    rpm=rpm,
                    pwm_pct=pwm_pct,
                    connected=rpm is not None,
                    valid=rpm is not None,
                    error=rpm_error,
                )
            )

    return channels


def _read_pwm_percent(hwmon_dir: Path, *, index: int) -> int | None:
    pwm_raw, _ = _read_int(hwmon_dir / f"pwm{index}")
    if pwm_raw is None:
        return None

    pwm_max, _ = _read_int(hwmon_dir / f"pwm{index}_max")
    max_value = pwm_max if pwm_max is not None else 255
    if max_value <= 0:
        return None

    pct = int(round((float(pwm_raw) * 100.0) / float(max_value)))
    return max(0, min(100, pct))


def _read_int(path: Path) -> tuple[int | None, str | None]:
    if not path.exists():
        return None, "source_missing"
    try:
        return int(path.read_text(encoding="utf-8").strip()), None
    except ValueError:
        return None, "invalid_value"
    except OSError as exc:
        return None, f"read_error:{type(exc).__name__}:{exc}"


def _read_text(path: Path) -> str | None:
    if not path.exists():
        return None
    try:
        return path.read_text(encoding="utf-8").strip()
    except OSError:
        return None
