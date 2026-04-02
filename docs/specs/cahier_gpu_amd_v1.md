# Cahier des charges GPU AMD V1

## 1. Portee

Extension GPU dediee sans rupture du contrat V1 principal.

Backend:
- FastAPI
- collecte amdgpu/sysfs/hwmon

Firmware:
- consommation HTTP
- affichage LVGL

## 2. Compatibilite V1

Endpoints principaux conserves:
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`

Endpoints GPU dedies:
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

Regles:
- JSON versionne `"v": 1`;
- metrique absente => enveloppe avec `value_raw/value_display = null` et `valid=false`;
- pas de champ supprime selon la machine.

## 3. Metriques GPU exposees

- `gpu.pct`
- `gpu.core_clock_mhz`
- `gpu.mem_clock_mhz`
- `gpu.vram_used_b`
- `gpu.vram_total_b`
- `gpu.vram_pct`
- `gpu.temp_c`
- `gpu.power_w`
- `gpu.fan_rpm`
- `gpu.fan_pct`

## 4. Series history GPU exposees

- `gpu_pct`
- `gpu_core_clock_mhz`
- `gpu_vram_used_b`
- `gpu_temp_c`
- `gpu_power_w`
- `gpu_mem_clock_mhz`
- `gpu_fan_rpm`

## 5. Etat firmware actuel

Le firmware consomme actuellement:
- `/api/v1/gpu/dashboard`

Le firmware ne consomme pas encore:
- `/api/v1/gpu/history`
- `/api/v1/gpu/meta`

Les graphes GPU sont alimentees localement depuis les snapshots recus.

## 6. Hors perimetre V1

- MQTT
- multi-GPU
- controle OC/ventilateurs
- cloud
- persistance longue duree
