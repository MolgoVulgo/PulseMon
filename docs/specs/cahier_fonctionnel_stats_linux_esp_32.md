# Cahier fonctionnel - Supervision Linux sur ESP32-S3

## 1. Objet

Definir le fonctionnement detaille backend + firmware pour la V1.

## 2. Responsabilites

### Backend Linux

- lit les capteurs Linux;
- applique les priorites/fallback capteurs;
- construit un snapshot versionne;
- maintient un historique court en memoire;
- expose des endpoints HTTP stables;
- signale la fraicheur via `state.stale_ms`.

### Firmware ESP32-S3

- gere Wi-Fi et appels HTTP;
- parse les enveloppes metriques JSON;
- met a jour les variables UI;
- rend les pages LVGL Main/GPU;
- signale l'indisponibilite backend.

## 3. Contrat V1

Prefixe: `/api/v1`

Racine metrique dashboard:
- `cpu.pct`, `cpu.temp_c`, `cpu.power_w`
- `mem.used_b`, `mem.total_b`, `mem.pct`
- `gpu.pct`, `gpu.temp_c`, `gpu.power_w`

Chaque metrique est une enveloppe:
- `value_raw`
- `value_display`
- `source`
- `unit`
- `sampled_at`
- `estimated`
- `valid`

Historique V1:
- `cpu_pct`, `cpu_temp_c`, `gpu_pct`, `gpu_temp_c`
- timeline explicite `ts_ms`

## 4. Endpoints

- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

Validation `/history`:
- `window`: `1..600`
- `step`: `1..10`
- `mode`: `display|raw`
- `since_ts_ms`: `>=0`

Erreur de validation:
- HTTP 400
- payload `{ "v": 1, "error": "invalid_parameter", "field": "..." }`

## 5. Regles de comportement

Backend:
- pas de lecture capteur dans les handlers;
- `dashboard` lit le store en memoire;
- `history` bucketise les points et renvoie `null` pour trous temporels;
- mode delta possible via `since_ts_ms` sur `/api/v1/history`.

Firmware (etat actuel):
- consomme `/dashboard` et `/gpu/dashboard`;
- ne consomme pas encore `/history`/`/meta`;
- graphes locaux issus des snapshots recus;
- en echec backend, conserve les dernieres valeurs et marque `backend offline`.

## 6. Cadences

Backend par defaut:
- acquisition: 0.1 s (10 Hz)
- publication snapshot/history: 0.5 s (2 Hz)

Firmware par defaut:
- polling snapshots: 1 s
- echantillonnage graphes locaux: 1 s

## 7. Limites V1

- pas de MQTT
- pas de multi-machines
- pas de persistance longue duree
- pas de pilotage de la machine Linux

## 8. Extension fonctionnelle ajoutee - ventilateurs configures

### 8.1 Endpoints cibles

- `GET /api/v1/fans/dashboard` (vue d'affichage)
- `GET /api/v1/fans/meta` (vue technique/diagnostic)
- `GET /api/v1/fans/history` (optionnel)

### 8.2 Regles de separation

- vue d'affichage: ventilateurs mappes/valides/retenus uniquement;
- vue technique: canaux physiques, source, validite, statut mapping.

### 8.3 Workflow impose

1. detection API
2. validation/calibration sur UI locale
3. integration ESP32 sur vue deja resolue

### 8.4 Regles firmware

- pas de remapping cote ESP32;
- conserver dernier snapshot valide si echec reseau/JSON;
- signaler stale sans vider l'ecran.
