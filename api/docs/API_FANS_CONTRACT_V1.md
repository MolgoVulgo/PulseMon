# API Fans Contract V1

Prefixe contractuel: `/api/v1/fans`

Extension ventilateurs V1:
- vue d'affichage pour clients finaux (`dashboard`)
- vue technique pour diagnostic/calibration (`meta`)

## Endpoints

- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`

## `GET /api/v1/fans/dashboard`

Retourne uniquement les ventilateurs:
- mappes;
- valides;
- actives (`enabled=true`).

Fallback sans mapping:
- si le fichier `STATS_FANS_MAPPING_FILE` est absent, l'API cree un mapping initial avec les canaux detectes puis repond en mode mappe;
- si le fichier `STATS_FANS_MAPPING_FILE` est invalide, l'endpoint scanne les canaux detectes actifs (`rpm > 0`) uniquement;
- un canal avec `rpm = 0` est considere OFF et n'apparait pas dans le `dashboard`.
- lors du bootstrap initial, un canal OFF est ecrit dans le mapping avec `"enabled": false`.

Payload:
- `v`, `ts`, `host`
- `fans[]`:
  - `label`
  - `role`
  - `rpm` (nullable)
  - `pwm_pct` (nullable)

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

## Mapping configurable

Fichier de mapping:
- `STATS_FANS_MAPPING_FILE` (defaut `api/config/fans_mapping.json`)

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
