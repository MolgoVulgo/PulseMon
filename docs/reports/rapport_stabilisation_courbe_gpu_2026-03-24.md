# Rapport court — Stabilisation courbe GPU (2026-03-24)

## Pourquoi la courbe était trop nerveuse

1. Le pipeline affichait un échantillon instantané `gpu_busy_percent` presque tel quel.
2. Ce capteur est naturellement bruité à court terme (micro-variations scheduler/queue GPU).
3. L'acquisition et la publication n'étaient pas suffisamment découplées: le rendu héritait trop directement des pics/creux instantanés.

Conséquence: des chutes visuelles brèves apparaissaient alors que la charge réelle restait globalement haute.

## Correctif appliqué

1. Acquisition capteur portée à 10 Hz (`STATS_SAMPLE_INTERVAL_S=0.1`).
2. Publication dashboard/history limitée à 2 Hz (`STATS_PUBLISH_INTERVAL_S=0.5`).
3. Pipeline d'affichage GPU usage:
   - `value_raw` = valeur capteur instantanée AMD (`gpu_busy_percent`)
   - préfiltre médian sur 3 points
   - `value_display` = EMA du signal médian (`alpha=0.25` par défaut, `STATS_DISPLAY_EMA_ALPHA`)
4. En cas d'échec de lecture: `value_raw=null`, `value_display=null`, `valid=false` (jamais de 0 injecté).

## Diagnostic raw vs display

Mode ajouté pour comparer brute et lissée sur 60 s:

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

Chaque ligne contient au minimum:
- `value_raw`
- `value_display`
- `source`
- `timestamp_unix_ms`
- `valid`

Validation rapide (capture locale 60 s @ 10 Hz):
- raw: `min=65`, `max=100`, `mean=84.9`
- display: `min=78.84`, `max=89.05`, `mean=84.74`

Interprétation:
- la mesure brute reste fidèle au capteur;
- la valeur affichée devient beaucoup plus stable visuellement, sans masquer les tendances de charge.
