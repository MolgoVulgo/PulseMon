# API HTTP

Le backend PulseMon expose une API HTTP locale en lecture sous `/api/v1`.

L’API est optimisée pour un client embarqué : clés stables, payloads compacts, unités fixes, valeurs numériques, métriques indisponibles nullables et historique borné.

## Règles générales

- Tous les payloads contiennent `v`.
- Les timestamps sont Unix ou en millisecondes quand le nom du champ l’indique.
- Les pourcentages sont numériques.
- Les températures sont en degrés Celsius.
- Les puissances sont en watts.
- Les tailles mémoire sont en octets.
- Les métriques absentes restent présentes avec `null` ou une enveloppe invalide.
- La clé API est transmise par header quand l’authentification est activée.

## Santé service

`GET /api/v1/health`

Rôle : vérifier la disponibilité minimale du service.

Réponse typique :

```json
{
  "v": 1,
  "ts": 1774256402,
  "ok": true,
  "service": "stats-linux-api"
}
```

## Dashboard principal

`GET /api/v1/dashboard`

Rôle : fournir le snapshot courant pour l’écran principal.

L’implémentation active utilise des enveloppes métriques pour la télémétrie affichée. Chaque enveloppe peut contenir :

- `value_raw` — valeur brute collectée ;
- `value_display` — valeur lissée ou prête à afficher ;
- `source` — source de télémétrie retenue ;
- `unit` — unité fixe ;
- `sampled_at` — timestamp d’échantillonnage ;
- `estimated` — indique si la valeur est estimée ;
- `valid` — indique si la valeur est exploitable.

Le firmware priorise `value_display`, bascule sur `value_raw`, puis peut tolérer une valeur scalaire directe pour compatibilité.

## Historique principal

`GET /api/v1/history?window=300&step=1&mode=display`

Rôle : fournir un historique court aligné pour graphes ou diagnostics.

Paramètres :

- `window` : entier, minimum `1`, maximum `600`, défaut `300` ;
- `step` : entier, minimum `1`, maximum `10`, défaut `1` ;
- `mode` : `display` ou `raw`, défaut `display` ;
- `since_ts_ms` : timestamp optionnel en millisecondes pour récupération delta.

La réponse contient des séries alignées et une timeline explicite `ts_ms`. Le client embarqué ne doit pas réaligner les tableaux lui-même.

Erreur de paramètre invalide :

```json
{
  "v": 1,
  "error": "invalid_parameter",
  "field": "window"
}
```

## Métadonnées

`GET /api/v1/meta`

Rôle : exposer l’hôte, les métriques disponibles, les séries historiques et les capacités pour diagnostic et validation.

## Endpoints GPU

`GET /api/v1/gpu/dashboard`

Rôle : état détaillé du GPU AMD pour l’écran GPU.

`GET /api/v1/gpu/history?window=300&step=1&mode=display`

Rôle : historique GPU borné pour affichage ou diagnostic.

`GET /api/v1/gpu/meta`

Rôle : métadonnées de source et de capacité GPU.

## Endpoints ventilateurs

`GET /api/v1/fans/dashboard`

Rôle : télémétrie ventilateurs courante et pourcentages calculés.

`GET /api/v1/fans/meta`

Rôle : métadonnées ventilateurs.

`GET /api/v1/fans/config`

Rôle : lire le mapping ventilateurs configuré.

`PUT /api/v1/fans/config`

Rôle : mettre à jour le mapping et les valeurs de calibration.

`GET /api/v1/fans/reference`

Rôle : fournir les références ventilateurs utilisées par l’UI admin.

## Configuration utilisateur

`GET /api/v1/user/config`

Rôle : lire la configuration UI utilisateur côté backend.

`PUT /api/v1/user/config`

Rôle : modifier la configuration UI utilisateur côté backend.

## Administration base locale

Le backend peut exposer des routes d’administration locale pour les mappings ventilateurs :

- `GET /api/v1/db/data`
- `GET /api/v1/db/fans?include_deleted=1`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`

Ces routes sont des outils d’administration locale, pas des dépendances d’affichage firmware.

## UI locale

`GET /ui`

Rôle : interface locale backend de debug/admin.

## Payload configuration utilisateur

`GET /api/v1/user/config` retourne :

```json
{
  "v": 1,
  "settings": {}
}
```

`PUT /api/v1/user/config` remplace tout l’objet de configuration utilisateur. L’objet `settings` est un JSON libre stocké en SQLite par clé.

Exemple :

```json
{
  "settings": {
    "refresh_hz": 1,
    "fans_view_mode": "meta",
    "show_gpu_graph": true
  }
}
```
