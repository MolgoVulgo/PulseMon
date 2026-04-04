# PulseMon API (backend Linux)

Backend FastAPI qui collecte les metriques Linux et expose un contrat HTTP V1 stable pour l'UI web locale et le firmware ESP32-S3.

## Setup

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Run

```bash
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## Test

```bash
.venv/bin/pytest -q
```

## Endpoints exposes

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history?window=300&step=1&mode=display`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history?window=300&step=1&mode=display`
- `GET /api/v1/gpu/meta`
- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /ui` (UI locale de debug)

## Contrat principal (resume)

`/api/v1/dashboard` retourne des metriques enveloppees:
- `value_raw`
- `value_display`
- `source`
- `unit`
- `sampled_at`
- `estimated`
- `valid`

`/api/v1/history` retourne des series alignees avec timeline explicite `ts_ms`.

## Parametres `/api/v1/history`

- `window`: min `1`, max `600`, defaut `300`
- `step`: min `1`, max `10`, defaut `1`
- `mode`: `display|raw`, defaut `display`
- `since_ts_ms`: optionnel (`>=0`), mode delta (`ts_ms > since_ts_ms`)

Erreur de validation:

```json
{
  "v": 1,
  "error": "invalid_parameter",
  "field": "window"
}
```

## Comportement runtime

- Acquisition capteurs: `STATS_SAMPLE_INTERVAL_S` (defaut `0.1` s, 10 Hz)
- Publication snapshot/history: `STATS_PUBLISH_INTERVAL_S` (defaut `0.5` s, 2 Hz)
- Historique memoire: `STATS_HISTORY_CAPACITY` (defaut `600`)
- Lissage affichage `%`: `Median(3)` puis `EMA(alpha)` avec `STATS_DISPLAY_EMA_ALPHA` (defaut `0.25`)

`/api/v1/dashboard` lit uniquement le store en memoire. Pas de lecture capteur lourde dans le handler.

## Configuration (env)

- `STATS_BIND_HOST` (defaut `0.0.0.0`)
- `STATS_BIND_PORT` (defaut `8000`)
- `STATS_SAMPLE_INTERVAL_S` (defaut `0.1`)
- `STATS_PUBLISH_INTERVAL_S` (defaut `0.5`)
- `STATS_HISTORY_CAPACITY` (defaut `600`)
- `STATS_DISPLAY_EMA_ALPHA` (defaut `0.25`)
- `STATS_API_KEY` (optionnel)
- `STATS_API_KEY_HEADER` (defaut `X-API-Key`)
- `STATS_LOG_LEVEL` (defaut `INFO`)
- `STATS_GPU_PCI_SLOT` (optionnel, forcage GPU AMD)
- `STATS_GPU_TEMP_LABEL_PRIORITY` (defaut `edge,junction,mem,unknown`)
- `STATS_DIAGNOSTICS` (defaut `0`)
- `STATS_DIAG_RAW_CAPTURE` (defaut `0`)
- `STATS_DIAG_RAW_HZ` (defaut `8.0`)
- `STATS_DIAG_RAW_DURATION_S` (defaut `60`)
- `STATS_DIAG_RAW_LOG_PATH` (defaut `api/diagnostics/raw_metrics.jsonl`)
- `STATS_DIAG_COMPARE_CAPTURE` (defaut `0`)
- `STATS_DIAG_COMPARE_HZ` (defaut `10.0`)
- `STATS_DIAG_COMPARE_DURATION_S` (defaut `60`)
- `STATS_DIAG_COMPARE_LOG_PATH` (defaut `api/diagnostics/raw_vs_display_gpu_pct.jsonl`)
- `STATS_FANS_MAPPING_FILE` (defaut `api/config/fans_mapping.json`)

Comportement fans sans mapping:
- si `STATS_FANS_MAPPING_FILE` est absent, l'API cree automatiquement un fichier de mapping initial a partir des canaux detectes.
- si `STATS_FANS_MAPPING_FILE` est invalide, `/api/v1/fans/dashboard` retourne uniquement les ventilateurs actifs detectes (`rpm > 0`).

## Auth optionnelle

Si `STATS_API_KEY` est definie, toutes les routes `/api/v1/*` exigent le header configure.

## Diagnostics CLI

Capture compare `gpu.pct` raw/display:

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

Capture brute multi-metriques:

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```

## References

- `api/docs/API_CONTRACT_V1.md`
- `api/docs/API_GPU_CONTRACT_V1.md`
- `api/docs/API_FANS_CONTRACT_V1.md`
