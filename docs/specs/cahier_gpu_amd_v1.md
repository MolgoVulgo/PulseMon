# Cahier des charges GPU AMD V1 (adapte)

## Portee
- Extension GPU specialisee sans rupture du contrat V1 existant.
- Backend Linux: API FastAPI + collecte amdgpu/sysfs/hwmon.
- Client ESP32-S3: consommation HTTP + affichage LVGL.

## Regles de compatibilite
- Les endpoints V1 existants restent inchanges:
  - `GET /api/v1/dashboard`
  - `GET /api/v1/history`
  - `GET /api/v1/meta`
- Extension GPU via endpoints dedies:
  - `GET /api/v1/gpu/dashboard`
  - `GET /api/v1/gpu/history`
  - `GET /api/v1/gpu/meta`
- JSON versionne `"v": 1`.
- Champs stables, metrique absente = `null` + `valid=false`.

## Metriques GPU V1
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

## Historique GPU V1
- `gpu_pct`
- `gpu_core_clock_mhz`
- `gpu_vram_used_b`
- `gpu_temp_c`
- `gpu_power_w`
- `gpu_mem_clock_mhz`
- `gpu_fan_rpm`

## UI ESP32
- Page GPU dediee deja generee.
- Navigation tactile: slide droite -> gauche pour passer Main -> GPU, slide inverse pour retour.
- Pas de logique metier/capteur cote UI.

## Hors perimetre V1
- MQTT
- multi-GPU
- controle OC/ventilateurs
- cloud/persistance longue duree
