# Rapport technique — Audit métriques CPU/GPU AMD (2026-03-24)

## 1) Constat technique initial

Problème observé: la courbe GPU affichait des chutes brèves incohérentes avec la charge perçue sous charge soutenue.

Cause principale confirmée:

- la valeur UI utilisait directement un sample instantané (`gpu_busy_percent`) sans séparation `raw`/`display`;
- aucune méta donnée de source/validité n'était exposée dans le JSON;
- en cas d'absence capteur, le contrat ne permettait pas de tracer finement la cause côté client.

## 2) Sources réellement utilisées dans le code (après patch)

### CPU usage

- Source retenue: `psutil.cpu_percent(interval=None)`
- Champ API: `cpu.pct`
- Unité: `percent`

Fallback:

1. lecture `psutil.cpu_percent(interval=None)`
2. si erreur: `value_raw=null`, `valid=false`, `read_error` en diagnostic

### CPU temperature

- Source primaire: `psutil.sensors_temperatures(fahrenheit=False):Tdie`
- Fallback: `...:Tctl`
- Champ API: `cpu.temp_c`
- Unité: `celsius`

Fallback:

1. `Tdie`
2. `Tctl`
3. sinon `null` + `valid=false`

### CPU power

- Source primaire: `/sys/class/hwmon/*(k10temp)/power1_average`
- Fallback: `/sys/class/hwmon/*(k10temp)/power1_input`
- Source d'état: `/sys/class/powercap/*/energy_uj` (présence détectée, mais pas de puissance instantanée)
- Champ API: `cpu.power_w`
- Unité: `watt`

Politique:

- pas d'estimation active en V1 (`estimated=false`)
- si télémétrie instantanée absente: `null`, `valid=false`

### GPU usage

- Source retenue: `/sys/class/drm/cardX/device/gpu_busy_percent` (carte AMD mappée explicitement)
- Champ API: `gpu.pct`
- Unité: `percent`

Fallback:

1. carte AMD sélectionnée + `gpu_busy_percent`
2. sinon `null` + `valid=false`

### GPU memory usage (diagnostic)

- Source: `/sys/class/drm/cardX/device/mem_busy_percent`
- Unité: `percent`
- Exposé dans le log diagnostic raw (pas dans le contrat dashboard V1)

### GPU temperature

- Source primaire: `hwmon temp*_input` label `edge`
- Fallbacks: `junction` puis `mem` puis premier capteur disponible
- Champ API: `gpu.temp_c`
- Unité: `celsius`

### GPU power

- Source primaire: `/sys/class/hwmon/hwmon*/power1_average`
- Fallback: `power1_input`
- Champ API: `gpu.power_w`
- Unité: `watt`

## 3) Vérification Linux AMD et mapping exact

Machine auditée:

- CPU: AMD Ryzen 9 5950X
- GPU: AMD (vendor `0x1002`, device `0x7550`)
- Driver: `amdgpu`

Mapping détecté:

- `card1` -> PCI `0000:08:00.0` -> `device=/sys/devices/.../0000:08:00.0`
- `card1/device/hwmon/hwmon3` -> `name=amdgpu`

Sources présentes:

- `gpu_busy_percent`: présent
- `mem_busy_percent`: présent
- `power1_average`: présent
- `temp1/2/3_input`: présents (`edge`, `junction`, `mem`)

Sources absentes / inexploitables:

- `/sys/kernel/debug/dri/*/amdgpu_pm_info`: non exploitable dans cet environnement (debugfs non accessible)
- CPU power instantanée fiable: absente (`k10temp power1_*` absent)

## 4) Mode diagnostic instrumenté

Nouveau mode ajouté:

- module CLI: `python -m app.diagnostics.raw_capture`
- sortie JSONL dédiée
- timestamp monotonic (`timestamp_mono_s`) + unix (`timestamp_unix_ms`)
- format par record:
  - `metric`
  - `raw_value`
  - `source`
  - `unit`
  - `read_ok`
  - `read_error`
  - `estimated`

Commande exécutée:

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

Résultat:

- `duration_s=60`
- `sample_hz=10`
- `rows=480`
- `records=3360`
- fichier: `api/diagnostics/raw_metrics_20260324.jsonl`

Exemple de logs:

```json
{"metric":"cpu.power_w","tick_index":0,"timestamp_mono_s":51333.869515,"timestamp_unix_ms":1774379316122,"source":"/sys/class/powercap/*/energy_uj","unit":"watt","raw_value":null,"read_ok":false,"read_error":"source_present_but_no_instantaneous_power","estimated":false}
{"metric":"gpu.pct","tick_index":0,"timestamp_mono_s":51333.870998,"timestamp_unix_ms":1774379316123,"source":"/sys/devices/.../gpu_busy_percent","unit":"percent","raw_value":56.0,"read_ok":true,"read_error":null,"estimated":false}
{"metric":"gpu.power_w","tick_index":0,"timestamp_mono_s":51333.873725,"timestamp_unix_ms":1774379316126,"source":"/sys/devices/.../hwmon/hwmon3/power1_average","unit":"watt","raw_value":166.0,"read_ok":true,"read_error":null,"estimated":false}
```

## 5) Calcul d'affichage robuste (raw vs displayed)

Implémentation:

- `value_raw`: valeur capteur directe
- `value_display`: préfiltre médian 3 points puis EMA (`alpha=0.25` par défaut)
- acquisition capteur à 10 Hz, publication API à 2 Hz (découplage acquisition/rendu)

Paramètres runtime:

- `STATS_SAMPLE_INTERVAL_S` (default `0.1`)
- `STATS_PUBLISH_INTERVAL_S` (default `0.5`)
- `STATS_DISPLAY_EMA_ALPHA` (default `0.25`)
- `STATS_GPU_TEMP_LABEL_PRIORITY` (default `edge,junction,mem,unknown`)

Règle d'échec:

- aucune substitution par `0`
- si lecture échoue: `value_raw=null`, `value_display=null`, `valid=false`

## 6) Politique watts

- GPU watts: télémétrie réelle (`power1_average`/`power1_input`) quand disponible.
- CPU watts: pas de faux watts. Si pas de télémétrie instantanée fiable: `null`, `valid=false`, `estimated=false`.
- Si estimation future: elle devra forcer `estimated=true` + formule explicitée.

## 7) Contrat JSON (dashboard)

Le contrat dashboard distingue maintenant explicitement:

- `value_raw`
- `value_display`
- `source`
- `unit`
- `sampled_at`
- `estimated`
- `valid`

Document mis à jour: `api/docs/API_CONTRACT_V1.md`

## 8) Validation exécutée

Tests backend:

```bash
cd api
.venv/bin/pytest -q
```

Résultat: `39 passed`.

Validation quantitative sur capture 60 s (`gpu.pct`):

- brut: `min=18`, `mean=85.61`, `p05=52`, `p95=100`, chutes transitoires détectées: `13`
- affiché (median3 + EMA): `min=56.0`, `mean=85.69`, `p05=77.33`, `p95=93.58`, chutes transitoires: `0`
- emulation cadence service `1 Hz` (raw une fois/seconde): brut `min=47`, affiché `min=56`, chutes transitoires `1 -> 0`

Lecture: la courbe affichée devient nettement plus stable et reste proche du niveau de charge perçu, tout en conservant la trace brute et la provenance.

## 9) Limites connues

- `amdgpu_pm_info` non utilisé ici à cause des permissions debugfs.
- CPU power réelle non disponible sur cette machine via les sources standards inspectées.
- Le smoothing median3+EMA améliore fortement les spikes courts, mais n'annule pas les baisses réelles prolongées.
- En multi-GPU AMD, la sélection se fait par carte AMD avec `gpu_busy_percent` (override possible via `STATS_GPU_PCI_SLOT`).
