# API GPU Contract V1

Prefixe contractuel: `/api/v1/gpu`

Ce contrat decrit l'extension GPU exposee en V1 sans rupture du contrat general (`/api/v1/dashboard`, `/api/v1/history`, `/api/v1/meta`).

## Endpoints

- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history?window=300&step=1&mode=display`
- `GET /api/v1/gpu/meta`

## `GET /api/v1/gpu/dashboard`

Retourne un snapshot GPU etendu:
- `v`, `ts`, `host`, `state`
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

Chaque champ GPU est une enveloppe metrique identique a celle du contrat principal (`value_raw`, `value_display`, `source`, `unit`, `sampled_at`, `estimated`, `valid`).

Comportement implementation:
- l'endpoint lit les capteurs en direct a chaque requete;
- puis met a jour le store GPU interne (snapshot + historique) pour coherence avec `/gpu/history`.

## `GET /api/v1/gpu/history`

Parametres:
- `window`: `1..600` (defaut `300`)
- `step`: `1..10` (defaut `1`)
- `mode`: `display|raw` (defaut `display`)

Reponse:
- `v`, `ts`, `ts_ms`, `window_s`, `step_s`
- `series.gpu_pct`
- `series.gpu_core_clock_mhz`
- `series.gpu_vram_used_b`
- `series.gpu_temp_c`
- `series.gpu_power_w`
- `series.gpu_mem_clock_mhz`
- `series.gpu_fan_rpm`

Regles:
- bucketisation temporelle comme pour `/api/v1/history`;
- `mode=display` prend les valeurs display, `mode=raw` prend `gpu_pct` brut;
- les series non disponibles retournent `null`;
- si le store est vide, un warmup est fait via une lecture live avant de repondre.

## `GET /api/v1/gpu/meta`

Reponse:
- `v`, `host`, `gpu_name`
- `metrics` (liste des metriques GPU exposees)
- `history_series` (liste des series GPU exposees)
- `caps`:
  - `fan_pct`
  - `hotspot_temp`
  - `mem_temp`

Etat actuel des caps:
- `fan_pct`: detecte dynamiquement (lecture `pwm1`)
- `hotspot_temp`: `false`
- `mem_temp`: `false`
