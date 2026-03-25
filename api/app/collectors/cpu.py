from __future__ import annotations

from collections.abc import Iterable
from pathlib import Path

import psutil

from .metric import MetricReading, failed_reading, ok_reading


HWMON_CLASS_PATH = Path("/sys/class/hwmon")
POWERCAP_CLASS_PATH = Path("/sys/class/powercap")


def read_cpu_percent_metric() -> MetricReading:
    source = "psutil.cpu_percent(interval=None)"
    try:
        value = float(psutil.cpu_percent(interval=None))
        return ok_reading(value=value, source=source, unit="percent")
    except Exception as exc:  # pragma: no cover - defensive path
        return failed_reading(source=source, unit="percent", error=f"read_error:{type(exc).__name__}:{exc}")


def read_cpu_temp_metric() -> MetricReading:
    source = "psutil.sensors_temperatures(fahrenheit=False)"
    try:
        temperatures = psutil.sensors_temperatures(fahrenheit=False)
    except Exception as exc:  # pragma: no cover - defensive path
        return failed_reading(source=source, unit="celsius", error=f"read_error:{type(exc).__name__}:{exc}")

    if not temperatures:
        return failed_reading(source=source, unit="celsius", error="sensor_missing:no_temperatures")

    # Prefer AMD k10temp family first, then scan all sensors.
    sensor_groups: list[Iterable] = []
    for name, entries in temperatures.items():
        if "k10temp" in name.lower():
            sensor_groups.append(entries)
    sensor_groups.extend(entries for entries in temperatures.values())

    tdie = _find_label_temp(sensor_groups, "tdie")
    if tdie is not None:
        return ok_reading(value=tdie, source=f"{source}:Tdie", unit="celsius")

    tctl = _find_label_temp(sensor_groups, "tctl")
    if tctl is not None:
        return ok_reading(value=tctl, source=f"{source}:Tctl", unit="celsius")

    return failed_reading(source=source, unit="celsius", error="sensor_missing:Tdie_Tctl")


def read_cpu_power_metric() -> MetricReading:
    # Keep deterministic behavior: no synthetic value when telemetry is missing.
    direct = _read_cpu_power_from_hwmon()
    if direct is not None:
        return direct

    if POWERCAP_CLASS_PATH.exists():
        energy_paths = sorted(POWERCAP_CLASS_PATH.glob("*/energy_uj"))
        if energy_paths:
            return failed_reading(
                source="/sys/class/powercap/*/energy_uj",
                unit="watt",
                error="source_present_but_no_instantaneous_power",
            )

    return failed_reading(
        source="/sys/class/hwmon/*(k10temp)/power1_average",
        unit="watt",
        error="source_unavailable",
    )


def read_cpu_percent() -> float:
    metric = read_cpu_percent_metric()
    return float(metric.raw_value) if metric.raw_value is not None else 0.0


def read_cpu_temp_c() -> float | None:
    metric = read_cpu_temp_metric()
    return float(metric.raw_value) if metric.raw_value is not None else None


def _find_label_temp(groups: Iterable[Iterable], label: str) -> float | None:
    wanted = label.lower()
    for entries in groups:
        for entry in entries:
            entry_label = (getattr(entry, "label", "") or "").lower()
            current = getattr(entry, "current", None)
            if entry_label == wanted and current is not None:
                return float(current)
    return None


def _read_cpu_power_from_hwmon() -> MetricReading | None:
    if not HWMON_CLASS_PATH.exists():
        return None

    for hwmon_dir in sorted(HWMON_CLASS_PATH.glob("hwmon*")):
        name_file = hwmon_dir / "name"
        if not name_file.exists():
            continue

        try:
            name = name_file.read_text(encoding="utf-8").strip().lower()
        except OSError:
            continue
        if name != "k10temp":
            continue

        for filename in ("power1_average", "power1_input"):
            power_path = hwmon_dir / filename
            if not power_path.exists():
                continue
            try:
                value_uw = float(power_path.read_text(encoding="utf-8").strip())
            except ValueError:
                return failed_reading(
                    source=str(power_path),
                    unit="watt",
                    error="invalid_value",
                )
            except OSError as exc:
                return failed_reading(
                    source=str(power_path),
                    unit="watt",
                    error=f"read_error:{type(exc).__name__}:{exc}",
                )
            return ok_reading(value=value_uw / 1_000_000.0, source=str(power_path), unit="watt")

    return None
