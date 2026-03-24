import psutil

from app.collectors.gpu import probe_gpu_device_path



def probe_sources() -> dict[str, object]:
    temperatures = psutil.sensors_temperatures(fahrenheit=False) or {}

    has_tdie = False
    has_tctl = False
    for entries in temperatures.values():
        for entry in entries:
            label = (getattr(entry, "label", "") or "").lower()
            if label == "tdie":
                has_tdie = True
            elif label == "tctl":
                has_tctl = True

    return {
        "cpu_temp_tdie": has_tdie,
        "cpu_temp_tctl": has_tctl,
        "gpu_device_path": probe_gpu_device_path(),
    }
