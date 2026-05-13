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
- rend les pages LVGL Main/GPU/Fan;
- signale l'indisponibilite backend.

Etat courant important:
- les pages Main et GPU sont alimentees par polling HTTP;
- le client `/api/v1/fans/dashboard` existe, mais le poller firmware ne l'appelle pas encore;
- la page Fan est donc presente cote UI, mais n'est pas encore synchronisee avec l'API fans.

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
- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /api/v1/fans/config`
- `PUT /api/v1/fans/config`
- `GET /api/v1/fans/reference`
- `GET /api/v1/user/config`
- `PUT /api/v1/user/config`
- `GET /api/v1/db/data`
- `GET /api/v1/db/fans`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`

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
- ne consomme pas encore `/fans/dashboard` malgre le client C disponible;
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
- `GET /api/v1/fans/config` (configuration mapping)
- `GET /api/v1/fans/reference` (catalogue de reference)

### 8.2 Regles de separation

- vue d'affichage: ventilateurs mappes/valides/retenus uniquement;
- vue technique: canaux physiques, source, validite, statut mapping.

### 8.3 Workflow impose

1. detection API
2. validation/calibration sur UI locale
3. integration ESP32 sur vue deja resolue

### 8.4 Regles firmware

- pas de remapping cote ESP32;
- integration poller fans restante: utiliser `/api/v1/fans/dashboard` sans lecture de `/fans/meta` cote firmware;
- conserver dernier snapshot valide si echec reseau/JSON;
- signaler stale sans vider l'ecran.
