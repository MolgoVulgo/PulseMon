# PulseMon

PulseMon est un systeme de supervision locale en deux briques:
- backend Linux en Python/FastAPI;
- firmware ESP32-S3 en ESP-IDF/LVGL.

## Ce que le code fait aujourd'hui

### Backend (`api/`)

- collecte CPU/RAM/GPU (psutil + sysfs/hwmon/DRM amdgpu);
- publie un snapshot V1 via `GET /api/v1/dashboard`;
- publie un historique court via `GET /api/v1/history`;
- expose une extension GPU dediee (`/api/v1/gpu/*`);
- fournit une UI web locale de debug (`/ui`).

Le contrat `dashboard` est base sur une enveloppe metrique (`value_raw`, `value_display`, `source`, `unit`, `sampled_at`, `valid`).

### Firmware (`esp/`)

- connecte le module au Wi-Fi;
- interroge l'API toutes les secondes;
- met a jour l'ecran LVGL avec:
  - `/api/v1/dashboard` (page Main)
  - `/api/v1/gpu/dashboard` (page GPU)

Etat actuel important:
- le firmware ne consomme pas encore les endpoints `/history` et `/meta`;
- les graphes embarques sont alimentes localement par echantillonnage des dernieres valeurs affichees.

## Structure

```text
PulseMon/
├── api/                    # backend FastAPI + tests + contrat API
├── esp/                    # firmware ESP32-S3 (PlatformIO + ESP-IDF)
├── docs/                   # specs, plans, ADR, rapports
├── pulsemon-api.service    # unite systemd
├── pulsemon-api.conf       # config runtime backend
└── PKGBUILD                # packaging Arch Linux
```

## Backend rapide

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Tests backend:

```bash
cd api
.venv/bin/pytest -q
```

## Firmware

```bash
cd esp
pio run -e LVGL-320-480
```

## Contrat et documentation de reference

- `api/docs/API_CONTRACT_V1.md`
- `api/docs/API_GPU_CONTRACT_V1.md`
- `docs/specs/spec_esp32_integration_stats_linux.md`
- `docs/specs/cahier_des_charges_stats_linux_esp_32.md`
- `docs/specs/cahier_fonctionnel_stats_linux_esp_32.md`
- `docs/plans/plan_implementation_api_stats_linux.md`

## Hors perimetre V1

- MQTT
- cloud/broker
- multi-machines
- persistance longue duree
- ecriture/pilotage de la machine Linux
