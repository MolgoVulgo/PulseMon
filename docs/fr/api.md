# API HTTP

Le backend expose des routes JSON versionnées sous `/api/v1` et une UI HTML locale sous `/ui`.

## Règles de contrat

- les modèles de réponse utilisent des champs Pydantic stricts ;
- les payloads versionnés utilisent `v: 1` ;
- la télémétrie indisponible reste présente avec `valid: false`, des valeurs `null` ou des points d’historique `null` ;
- les enveloppes métriques contiennent `value_raw`, `value_display`, `source`, `unit`, `sampled_at`, `estimated` et `valid` ;
- les tableaux d’historique restent alignés avec `ts_ms` ;
- les paramètres de requête invalides produisent une erreur API explicite ;
- l’authentification, si activée, utilise `STATS_API_KEY_HEADER`, par défaut `X-API-Key`.

## Routes actives

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

L’UI HTML locale est exposée via `GET /ui` et ne fait pas partie du contrat JSON versionné.

## Dashboard principal

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

## Dashboard GPU

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

Les champs de ventilateur GPU sont de la télémétrie GPU. Ils n’impliquent aucune API générique de mapping ou de gestion FAN.

## Paramètres de requête

`/api/v1/history` :

- `window` : 1 à 600 secondes, défaut 300 ;
- `step` : 1 à 10 secondes, défaut 1 ;
- `mode` : `display` ou `raw` ;
- `since_ts_ms` : timestamp non négatif optionnel.

`/api/v1/gpu/history` accepte `window`, `step` et `mode` avec les mêmes bornes.

## État et persistance

Aucune route de configuration utilisateur ou de base de données n’est exposée. L’état backend se limite aux snapshots courants et historiques bornés en mémoire.

## Limite d’authentification client

Si `STATS_API_KEY` est activée, toutes les requêtes `/api/v1/*` exigent le header configuré. Le firmware et l’UI backend courants ne l’envoient pas ; l’activation de la clé exige une adaptation correspondante des clients.
