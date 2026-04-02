# Cahier des charges - Stats Linux vers ESP32-S3

## 1. Objet

Fournir un affichage embarque des metriques Linux via un backend local HTTP et un firmware ESP32-S3.

Systeme cible:
- collecte locale Linux;
- API HTTP versionnee;
- client HTTP ESP32;
- rendu LVGL temps reel.

## 2. Architecture retenue

- Backend Linux: Python/FastAPI, collecte capteurs, normalisation, cache snapshot/history.
- Transport: JSON versionne `"v": 1`.
- Firmware: ESP-IDF, polling HTTP, parsing JSON, affichage LVGL.

## 3. Perimetre V1

Metriques instantanees exposees:
- `cpu.pct`, `cpu.temp_c`, `cpu.power_w`
- `mem.used_b`, `mem.total_b`, `mem.pct`
- `gpu.pct`, `gpu.temp_c`, `gpu.power_w`

Series history exposees:
- `cpu_pct`, `cpu_temp_c`, `gpu_pct`, `gpu_temp_c`

Extension GPU dediee disponible:
- `/api/v1/gpu/dashboard`
- `/api/v1/gpu/history`
- `/api/v1/gpu/meta`

## 4. Contrat JSON

Principes obligatoires:
- version `"v": 1`;
- noms de champs stables;
- unites fixes;
- champs conserves meme si mesure absente;
- absence de mesure: `null` + `valid=false` dans l'enveloppe metrique.

Enveloppe metrique V1:
- `value_raw`
- `value_display`
- `source`
- `unit`
- `sampled_at`
- `estimated`
- `valid`

## 5. Endpoints V1

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

`/api/v1/history` supporte:
- `window` (`1..600`)
- `step` (`1..10`)
- `mode` (`display|raw`)
- `since_ts_ms` (delta)

## 6. Contraintes techniques

Backend:
- collecte AMD via `psutil` + `sysfs/hwmon/DRM`;
- acquisition et publication decouplees (defauts 10 Hz / 2 Hz);
- aucune lecture capteur lourde dans les handlers HTTP.

Firmware:
- polling 1 Hz des snapshots;
- consommation actuelle: `/dashboard` et `/gpu/dashboard`;
- graphes locaux bases sur snapshots recus;
- affichage degrade explicite en cas d'echec backend.

## 7. Hors perimetre V1

- MQTT
- cloud/broker
- multi-machines
- persistance longue duree
- pilotage/ecriture machine Linux
