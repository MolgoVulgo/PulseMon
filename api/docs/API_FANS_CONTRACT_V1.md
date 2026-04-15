# API Fans Contract V1

Prefixe contractuel: `/api/v1/fans`

Extension ventilateurs V1:
- vue d'affichage pour clients finaux (`dashboard`)
- vue technique pour diagnostic/calibration (`meta`)
- gestion DB locale (UI admin, CRUD `fan_mappings`)

## Endpoints

- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /api/v1/fans/config`
- `PUT /api/v1/fans/config`
- `GET /api/v1/fans/reference`
- `GET /api/v1/db/fans?include_deleted=1`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`

## `GET /api/v1/fans/dashboard`

Retourne uniquement les ventilateurs:
- mappes;
- valides;
- actives (`enabled=true`).

Fallback sans mapping:
- si la base SQLite est vide, l'API tente d'importer le mapping legacy JSON puis, sinon, cree un mapping initial avec les canaux detectes;
- un canal avec `rpm = 0` est considere OFF et n'apparait pas dans le `dashboard`.
- lors du bootstrap initial, un canal OFF est enregistre avec `"enabled": false`.

Payload:
- `v`, `ts`, `host`
- `fans[]`:
  - `label`
  - `role`
  - `rpm` (nullable)
  - `pwm_pct` (nullable)
  - `pct_fans` (nullable): pourcentage de vitesse mecanique reelle (0..100)

Regle `pct_fans`:
- calcule si `rpm` et `rpm_max` sont disponibles;
- si `rpm_min` est absent (`null`), le backend utilise `rpm_min = 0`;
- formule: `clamp(round((rpm - rpm_min_effective) * 100 / (rpm_max - rpm_min_effective)), 0, 100)`;
- `rpm_min_effective = rpm_min` si present, sinon `0`;
- sinon `pct_fans = null`.

## `GET /api/v1/fans/meta`

Retourne la vue technique complete:
- `v`, `ts`, `host`
- `channels[]`:
  - `channel`
  - `hwmon_name`
  - `hwmon_path`
  - `source`
  - `group`
  - `label`
  - `rpm`
  - `pwm_pct`
  - `connected`
  - `valid`
  - `error`
  - `mapping`:
    - `configured`
    - `label`
    - `role`
    - `order`
    - `enabled`
- `display_labels[]`

## `GET /api/v1/fans/config`

Retourne la configuration de mapping active:
- `v`
- `mapping_path` (source SQLite, format `sqlite:///...`)
- `allowed_roles` (liste fermee: `cpu|case|pump|gpu|radiator|unknown`)
- `mappings[]`

Comportement premier lancement:
- si la base est vide, l'API bootstrap automatiquement `mappings[]` depuis les canaux detectes (avec import legacy JSON si disponible).

## `PUT /api/v1/fans/config`

Sauvegarde complete de la configuration:
- corps JSON strict: `{ "mappings": [...] }`
- la reponse retourne la config ecrite (`v`, `mapping_path`, `mappings`).

Champs de mapping utiles UI:
- `reference_id` (nullable): identifiant selectionne depuis le catalogue reference
- `rpm_min` (nullable)
- `rpm_max` (nullable)

## `GET /api/v1/fans/reference`

Retourne un catalogue aplati depuis `tmp/fan_reference_seed.json`:
- `v`
- `generated_at`
- `count`
- `items[]`:
  - `id` (`brand|series|model`)
  - `brand`, `series`, `model`
  - `rpm_min`, `rpm_max`
  - `pwm`, `connector`, `size_mm`

## Mapping configurable

Source de mapping:
- SQLite par defaut: `~/.config/pulsemon/config.db`
- override: `STATS_CONFIG_DB_PATH`
- import legacy optionnel: `STATS_FANS_MAPPING_FILE` (uniquement migration initiale)

Format minimal:

```json
{
  "mappings": [
    {
      "label": "CPU",
      "role": "cpu",
      "order": 10,
      "enabled": true,
      "match": {
        "hwmon_name": "it8628",
        "channel": "fan1",
        "group": "motherboard"
      }
    }
  ]
}
```

## Endpoints DB (admin)

Ces routes manipulent directement `fan_mappings` en SQLite pour l'UI locale de gestion:

- `GET /api/v1/db/fans?include_deleted=1`
  - retourne `items[]` avec `id`, `deleted`, `mapping`.
- `POST /api/v1/db/fans`
  - cree une entree (`mapping`).
- `PUT /api/v1/db/fans/{fan_id}`
  - met a jour une entree existante.
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
  - suppression logique: `deleted=true`, `enabled=false`.
- `POST /api/v1/db/fans/{fan_id}/restore`
  - restauration: `deleted=false`, `enabled=true`.
- `DELETE /api/v1/db/fans/{fan_id}`
  - suppression definitive de la ligne SQLite.
