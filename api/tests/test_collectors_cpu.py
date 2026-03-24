from collections import namedtuple

from app.collectors.cpu import read_cpu_temp_c


TempEntry = namedtuple("TempEntry", ["label", "current"])


def test_cpu_temp_prefers_tdie() -> None:
    sample = {
        "k10temp": [
            TempEntry(label="Tctl", current=61.2),
            TempEntry(label="Tdie", current=58.4),
        ]
    }

    import app.collectors.cpu as cpu_mod

    original = cpu_mod.psutil.sensors_temperatures
    cpu_mod.psutil.sensors_temperatures = lambda fahrenheit=False: sample
    try:
        assert read_cpu_temp_c() == 58.4
    finally:
        cpu_mod.psutil.sensors_temperatures = original


def test_cpu_temp_fallbacks_to_tctl() -> None:
    sample = {
        "k10temp": [
            TempEntry(label="Tctl", current=62.0),
        ]
    }

    import app.collectors.cpu as cpu_mod

    original = cpu_mod.psutil.sensors_temperatures
    cpu_mod.psutil.sensors_temperatures = lambda fahrenheit=False: sample
    try:
        assert read_cpu_temp_c() == 62.0
    finally:
        cpu_mod.psutil.sensors_temperatures = original


def test_cpu_temp_none_when_missing() -> None:
    sample = {
        "k10temp": [
            TempEntry(label="edge", current=44.0),
        ]
    }

    import app.collectors.cpu as cpu_mod

    original = cpu_mod.psutil.sensors_temperatures
    cpu_mod.psutil.sensors_temperatures = lambda fahrenheit=False: sample
    try:
        assert read_cpu_temp_c() is None
    finally:
        cpu_mod.psutil.sensors_temperatures = original
