# API Contract V1

Prefixe contractuel: `/api/v1`

Extension GPU dediee: `api/docs/API_GPU_CONTRACT_V1.md`

## Portee

Ce document decrit le contrat HTTP effectivement implemente par `api/app/main.py`.

Garantie de compatibilite V1:
- `"v": 1` sur tous les payloads de contrat;
- noms de champs stables;
- champs conserves meme si la metrique est absente (`null` + `valid=false` dans l'enveloppe metrique);
- `history` retourne des series alignees sur une timeline `ts_ms` explicite.

## Enveloppe metrique

Chaque metrique de `dashboard` utilise cette forme:

```json
{
  "value_raw": 87.0,
  "value_display": 91.6,
  "source": "/sys/class/drm/card1/device/gpu_busy_percent",
  "unit": "percent",
  "sampled_at": 1774256402000,
  "estimated": false,
  "valid": true
}
```

Champs:
- `value_raw`: valeur capteur brute, ou `null`.
- `value_display`: valeur prete a afficher, ou `null`.
- `source`: source retenue (signature logique ou chemin sysfs).
- `unit`: unite fixe (`percent`, `celsius`, `watt`, `bytes`).
- `sampled_at`: timestamp Unix en millisecondes.
- `estimated`: reserve aux valeurs estimees (actuellement `false` dans le backend).
- `valid`: `true` si une valeur exploitable a ete lue.

Politique `value_display` actuelle:
- `cpu.pct` et `gpu.pct`: `Median(3)` puis `EMA(alpha)`.
- autres metriques: `value_display == value_raw` quand la metrique est valide.

## Cadence backend

Cadence par defaut (configurable via variables d'environnement):
- acquisition capteurs: `STATS_SAMPLE_INTERVAL_S=0.1` (10 Hz);
- publication snapshot/history: `STATS_PUBLISH_INTERVAL_S=0.5` (2 Hz).

## Endpoints

### `GET /api/v1/health`

Retourne un etat minimal du service.

```json
{
  "v": 1,
  "ts": 1774256402,
  "ok": true,
  "service": "stats-linux-api"
}
```

Note: l'implementation actuelle retourne `ok=true`.

### `GET /api/v1/dashboard`

Retourne le dernier snapshot publie par le sampler.

Structure racine:
- `v`, `ts`, `host`
- `cpu`: `pct`, `temp_c`, `power_w` (enveloppes metrique)
- `mem`: `used_b`, `total_b`, `pct` (enveloppes metrique)
- `gpu`: `pct`, `temp_c`, `power_w` (enveloppes metrique)
- `state`: `ok`, `stale_ms`

Si aucun snapshot n'est disponible: HTTP `503` + erreur `snapshot_unavailable`.

### `GET /api/v1/history?window=300&step=1&mode=display`

Parametres:
- `window`: `1..600` (defaut `300`).
- `step`: `1..10` (defaut `1`).
- `mode`: `display|raw` (defaut `display`).
- `since_ts_ms`: optionnel, entier `>=0`, active le mode delta.

Reponse:

```json
{
  "v": 1,
  "ts": 1774256402,
  "ts_ms": [1774256399000, 1774256400000, 1774256401000, 1774256402000],
  "window_s": 300,
  "step_s": 1,
  "series": {
    "cpu_pct": [8.0, 9.4, 11.2, 10.1],
    "cpu_temp_c": [41.0, 41.1, 41.3, 41.4],
    "gpu_pct": [2.0, 4.0, 7.0, 3.0],
    "gpu_temp_c": [38.0, 38.0, 39.0, 39.0]
  }
}
```

Regles `history`:
- bucketisation sur grille absolue (`floor(ts_ms/step_ms)`);
- `ts_ms` est l'axe X de reference;
- buckets manquants => points `null` (pas d'interpolation metier);
- `mode=display` utilise `value_display`, `mode=raw` utilise `value_raw`;
- avec `since_ts_ms`, seules les points `ts_ms > since_ts_ms` sont retournes;
- si `since_ts_ms` est plus ancien que la fenetre retenue, le backend force une reponse fenetre complete (resync client).

### `GET /api/v1/meta`

Retourne la liste normative exposee par l'API:
- `metrics`: `cpu.pct`, `cpu.temp_c`, `cpu.power_w`, `mem.used_b`, `mem.total_b`, `mem.pct`, `gpu.pct`, `gpu.temp_c`, `gpu.power_w`
- `history_series`: `cpu_pct`, `cpu_temp_c`, `gpu_pct`, `gpu_temp_c`

## Erreurs

Payload d'erreur V1:

```json
{
  "v": 1,
  "error": "invalid_parameter",
  "field": "window"
}
```

Erreurs contractuelles principales:
- `invalid_parameter` (`window`, `step`, `mode`, `since_ts_ms`) -> HTTP 400.
- `snapshot_unavailable` -> HTTP 503.
- `unauthorized` -> HTTP 401 (si cle API active).

## Authentification optionnelle

Si `STATS_API_KEY` est definie, toutes les routes `/api/v1/*` exigent le header configure (defaut `X-API-Key`).
Le champ `field` de l'erreur `unauthorized` est retourne en minuscule (ex: `x-api-key`).
