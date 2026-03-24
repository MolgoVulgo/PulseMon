from collections.abc import Iterable

import psutil


def read_cpu_percent() -> float:
    return float(psutil.cpu_percent(interval=None))


def read_cpu_temp_c() -> float | None:
    temperatures = psutil.sensors_temperatures(fahrenheit=False)
    if not temperatures:
        return None

    # Prefer AMD k10temp family first, then scan all sensors.
    sensor_groups: list[Iterable] = []
    for name, entries in temperatures.items():
        if "k10temp" in name.lower():
            sensor_groups.append(entries)
    sensor_groups.extend(entries for entries in temperatures.values())

    tdie = _find_label_temp(sensor_groups, "tdie")
    if tdie is not None:
        return tdie

    tctl = _find_label_temp(sensor_groups, "tctl")
    if tctl is not None:
        return tctl

    return None


def _find_label_temp(groups: Iterable[Iterable], label: str) -> float | None:
    wanted = label.lower()
    for entries in groups:
        for entry in entries:
            entry_label = (getattr(entry, "label", "") or "").lower()
            current = getattr(entry, "current", None)
            if entry_label == wanted and current is not None:
                return float(current)
    return None
