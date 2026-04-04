# Plan d'implementation API - Stats Linux vers ESP32-S3

## 1. Objet

Documenter l'etat d'implementation backend et les points de stabilisation restants.

## 2. Etat implemente

### 2.1 Architecture code

Structure effective dans `api/app/`:
- `config.py`
- `models/`
- `collectors/`
- `store/`
- `services/`
- `diagnostics/`
- `main.py` (routes HTTP)

### 2.2 Contrat expose

Implante:
- `/api/v1/health`
- `/api/v1/dashboard`
- `/api/v1/history`
- `/api/v1/meta`
- `/api/v1/gpu/dashboard`
- `/api/v1/gpu/history`
- `/api/v1/gpu/meta`

Le contrat `dashboard` est base sur enveloppes metriques `value_raw/value_display/source/unit/sampled_at/estimated/valid`.

### 2.3 Collecte metriques

CPU:
- `psutil.cpu_percent(interval=None)`
- temperature: priorite `Tdie`, fallback `Tctl`
- power: `k10temp power1_average|power1_input`, sinon `None`

RAM:
- `psutil.virtual_memory()`

GPU AMD:
- selection carte AMD via `/sys/class/drm/card*/device/vendor == 0x1002`
- usage: `gpu_busy_percent`
- temp: `temp*_input` avec priorite labels configurable (`STATS_GPU_TEMP_LABEL_PRIORITY`)
- power: `power1_average|power1_input`
- extension: clocks, VRAM used/total/pct, fan rpm/pct

### 2.4 Sampling/store

- acquisition periodique (`STATS_SAMPLE_INTERVAL_S`, defaut `0.1s`)
- publication store (`STATS_PUBLISH_INTERVAL_S`, defaut `0.5s`)
- `SnapshotStore` + `HistoryStore` (ring buffer)
- `history` bucketise sur timeline ms reelle + support delta (`since_ts_ms`)

## 3. Verification actuelle

Assuree par tests `api/tests/`:
- modele de contrat V1
- validations de parametres
- alignement des series history
- comportement `mode=display|raw`
- comportement delta/resync `since_ts_ms`
- capteurs CPU/GPU (fallbacks)
- endpoints GPU dedies

## 4. Ecarts vs objectifs initiaux

- boucle historique API initialement prevue a 1 Hz -> implementation decouplee 10 Hz acquisition / 2 Hz publication (configurable)
- endpoint GPU dedie ajoute en V1 (`/api/v1/gpu/*`)
- contrat dashboard evolue vers enveloppe metrique riche (pas scalaire simple)

## 5. Priorites de stabilisation restantes

1. garder la stabilite du contrat JSON V1 (tests fixtures).
2. conserver la robustesse des fallback capteurs AMD selon machines.
3. maintenir la compatibilite firmware ESP en cas de metriques absentes.
4. eviter toute lecture capteur lourde en handler HTTP.

## 6. Commandes de validation backend

```bash
cd api
.venv/bin/pytest -q
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## 7. References

- `api/docs/API_CONTRACT_V1.md`
- `api/docs/API_GPU_CONTRACT_V1.md`
- `docs/specs/spec_esp32_integration_stats_linux.md`
- `docs/specs/cahier_gpu_amd_v1.md`
- `docs/plans/plan_implementation_gpu_monitoring_amd.md`
- `docs/specs/cahier_fans_config_v1.md`
- `docs/plans/plan_implementation_fans_config.md`
