# API User Config Contract V1

Prefixe contractuel: `/api/v1/user`

## Endpoints

- `GET /api/v1/user/config`
- `PUT /api/v1/user/config`

## `GET /api/v1/user/config`

Retourne la configuration utilisateur persistée:

```json
{
  "v": 1,
  "settings": {}
}
```

`settings` est un objet clé/valeur JSON libre.

## `PUT /api/v1/user/config`

Remplace entièrement la configuration utilisateur:

```json
{
  "settings": {
    "refresh_hz": 1,
    "fans_view_mode": "meta",
    "show_gpu_graph": true
  }
}
```

Réponse: même forme que `GET`.

## Stockage

- SQLite locale (`STATS_CONFIG_DB_PATH`, défaut `~/.config/pulsemon/config.db`).
- Les valeurs sont stockées par clé (`user_settings.key`) avec payload JSON (`user_settings.value_json`).
