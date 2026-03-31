# API GPU Contract V1

Prefixe: `/api/v1/gpu`

## Endpoints
- `GET /dashboard`
- `GET /history?window=300&step=1&mode=display`
- `GET /meta`

## Dashboard

```json
{
  "v": 1,
  "ts": 1774256402,
  "host": "linux-main",
  "gpu": {
    "pct": {"value_raw": 10.0, "value_display": 9.5, "source": "...", "unit": "percent", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "core_clock_mhz": {"value_raw": 739, "value_display": 739, "source": "...", "unit": "mhz", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "mem_clock_mhz": {"value_raw": 1300, "value_display": 1300, "source": "...", "unit": "mhz", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "vram_used_b": {"value_raw": 2040109465, "value_display": 2040109465, "source": "...", "unit": "bytes", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "vram_total_b": {"value_raw": 17080198144, "value_display": 17080198144, "source": "...", "unit": "bytes", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "vram_pct": {"value_raw": 12.0, "value_display": 12.0, "source": "...", "unit": "percent", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "temp_c": {"value_raw": 56.0, "value_display": 56.0, "source": "...", "unit": "celsius", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "power_w": {"value_raw": 49.0, "value_display": 49.0, "source": "...", "unit": "watt", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "fan_rpm": {"value_raw": 0, "value_display": 0, "source": "...", "unit": "rpm", "sampled_at": 1774256402000, "estimated": false, "valid": true},
    "fan_pct": {"value_raw": null, "value_display": null, "source": "...", "unit": "percent", "sampled_at": 1774256402000, "estimated": false, "valid": false}
  },
  "state": {"ok": true, "stale_ms": 0}
}
```

## History

```json
{
  "v": 1,
  "ts": 1774256402,
  "ts_ms": [1774256399000, 1774256400000, 1774256401000],
  "window_s": 300,
  "step_s": 1,
  "series": {
    "gpu_pct": [7.0, 8.0, 10.0],
    "gpu_core_clock_mhz": [650, 680, 739],
    "gpu_vram_used_b": [1986422374, 2013265920, 2040109465],
    "gpu_temp_c": [55.0, 55.0, 56.0],
    "gpu_power_w": [47.0, 48.0, 49.0],
    "gpu_mem_clock_mhz": [1300, 1300, 1300],
    "gpu_fan_rpm": [0, 0, 0]
  }
}
```

## Meta
- `metrics`: liste des metriques GPU exposees.
- `history_series`: liste des series GPU exposees.
- `caps`: capacites detectees (`fan_pct`, `hotspot_temp`, `mem_temp`).
