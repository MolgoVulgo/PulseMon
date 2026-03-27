# PulseMon

PulseMon est un systeme de supervision locale compose de deux briques:
- un backend Linux en Python (FastAPI) qui collecte CPU/RAM/GPU et expose une API HTTP V1 stable;
- un firmware ESP32-S3 (ESP-IDF + LVGL) qui interroge cette API et affiche les metriques en temps reel.

## Ce que le projet fait

- collecte locale des metriques Linux (psutil + sources AMD sysfs/hwmon/DRM);
- publication d'un snapshot JSON versionne via `/api/v1/dashboard`;
- publication d'un historique court aligne via `/api/v1/history`;
- exposition d'etat service et metadonnees (`/health`, `/meta`);
- affichage embarque sur ESP32-S3 avec polling HTTP, cache local et rendu LVGL.

## Pourquoi cette architecture

- decouplage clair collecte Linux / affichage embarque;
- contrat JSON stable pour limiter la complexite cote ESP32;
- pas de broker ni cloud en V1 (maintenance plus simple, debug local);
- comportement deterministe en cas de metrique absente (`null`, champ conserve).

## Comment c'est organise

```text
PulseMon/
├── api/      # backend Linux FastAPI + tests + contrat API
├── esp/      # firmware ESP32-S3 (ESP-IDF via PlatformIO)
├── docs/     # specs/plans/adr/reports
├── pulsemon-api.service  # unite systemd
├── pulsemon-api.conf     # variables d'environnement runtime
└── PKGBUILD              # packaging Arch Linux (pulsemon-api)
```

## Demarrage rapide backend (developpement)

Prerequis:
- Python 3.11+

Commandes:

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Verification rapide:
- UI locale: `http://127.0.0.1:8000/ui`
- API:
  - `GET /api/v1/health`
  - `GET /api/v1/dashboard`
  - `GET /api/v1/history?window=300&step=1&mode=display`
  - `GET /api/v1/meta`

Tests backend:

```bash
cd api
.venv/bin/pytest -q
```

## Firmware ESP32-S3

Le firmware est dans `esp/` et utilise PlatformIO avec framework ESP-IDF.

Commande de build cible:

```bash
cd esp
pio run -e LVGL-320-480
```

Responsabilites firmware:
- connexion Wi-Fi;
- appel HTTP periodique du backend;
- parsing JSON;
- cache local;
- rendu LVGL et signalement etat stale/degrade.

## Contrat API et verite documentaire

Contrat HTTP de reference:
- `api/docs/API_CONTRACT_V1.md`

References projet:
- `docs/specs/cahier_des_charges_stats_linux_esp_32.md`
- `docs/specs/cahier_fonctionnel_stats_linux_esp_32.md`
- `docs/plans/plan_implementation_api_stats_linux.md`
- `docs/specs/spec_esp32_integration_stats_linux.md`

Regle: en cas de divergence entre texte et implementation, la verite d'interface est le contrat API V1 et les tests backend.

## Configuration runtime backend

Variables principales (voir aussi `pulsemon-api.conf`):
- `STATS_BIND_HOST`, `STATS_BIND_PORT`
- `STATS_SAMPLE_INTERVAL_S`, `STATS_PUBLISH_INTERVAL_S`
- `STATS_HISTORY_CAPACITY`
- `STATS_DISPLAY_EMA_ALPHA`
- `STATS_API_KEY` (optionnelle)

## Hors perimetre V1

- MQTT
- cloud/broker
- multi-machines
- persistance longue duree
- structure JSON variable selon la machine

