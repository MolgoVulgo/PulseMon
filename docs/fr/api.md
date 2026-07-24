# API HTTP

Le backend expose des routes JSON versionnées sous `/api/v1` et une UI HTML locale sous `/ui`.

## Règles de contrat

- les modèles de réponse utilisent des champs Pydantic stricts ;
- les payloads versionnés utilisent `v: 1` ;
- la télémétrie indisponible reste présente avec `valid: false`, des valeurs `null` ou des points d’historique `null` ;
- les enveloppes métriques contiennent `value_raw`, `value_display`, `source`, `unit`, `sampled_at`, `estimated` et `valid` ;
- les tableaux d’historique restent alignés avec `ts_ms` ;
- l’authentification, si activée, utilise `STATS_API_KEY_HEADER`, par défaut `X-API-Key`.

## Routes de supervision actives

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

Métriques dashboard principal :

```text
cpu.pct, cpu.temp_c, cpu.power_w
mem.used_b, mem.total_b, mem.pct
gpu.pct, gpu.temp_c, gpu.power_w
state.ok, state.stale_ms
```

Séries historiques principales :

```text
cpu_pct, cpu_temp_c, gpu_pct, gpu_temp_c
```

Métriques dashboard GPU :

```text
gpu.pct, gpu.core_clock_mhz, gpu.mem_clock_mhz
gpu.vram_used_b, gpu.vram_total_b, gpu.vram_pct
gpu.temp_c, gpu.power_w, gpu.fan_rpm, gpu.fan_pct
```

Séries historiques GPU :

```text
gpu_pct, gpu_core_clock_mhz, gpu_vram_used_b, gpu_temp_c,
gpu_power_w, gpu_mem_clock_mhz, gpu_fan_rpm
```

## Routes utilisateur et base

- `GET /api/v1/user/config`
- `PUT /api/v1/user/config`
- `GET /api/v1/db/data`

`user/config` stocke un objet JSON libre en SQLite.

## Routes FAN conservées

Ces routes restent implémentées mais ne sont pas consommées par le flux firmware actif :

- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /api/v1/fans/config`
- `PUT /api/v1/fans/config`
- `GET /api/v1/fans/reference`
- `GET /api/v1/db/fans`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`

## Limite d’authentification client

Si `STATS_API_KEY` est activée, toutes les requêtes `/api/v1/*` exigent le header configuré. Le firmware et l’UI backend courants ne l’envoient pas ; l’activation de la clé nécessite donc une modification des clients.
