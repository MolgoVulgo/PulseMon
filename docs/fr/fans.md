# Sous-système FAN conservé

Le développement FAN a été abandonné comme fonctionnalité active de PulseMon, mais l’implémentation n’a pas été entièrement supprimée.

## Statut courant

Le backend fournit encore :

- découverte des ventilateurs via hwmon ;
- mappings FAN en SQLite ;
- import/bootstrap des mappings ;
- endpoints dashboard, métadonnées, configuration et références ;
- routes d’administration locale et support UI backend.

Le firmware contient encore :

- structures de payload et fonctions de parsing FAN ;
- objets générés de l’écran FAN ;
- variables runtime et helpers dormants.

Le comportement firmware actif exclut FAN :

- `actions.c` associe `SCREEN_ID_FAN` à aucune cible ;
- la navigation active contient uniquement Main, GPU et Météo ;
- `pulsemon_poller.c` n’appelle pas `pulsemon_fetch_fans_dashboard()`.

## Règle de maintenance

Ne pas supprimer ni réparer opportunément ce sous-système conservé. Ne pas le documenter comme fonctionnalité utilisateur active. Toute réactivation, suppression finale ou migration de données exige une tâche explicite couvrant backend, firmware, EEZ et documentation.

## Routes API conservées

- `GET /api/v1/fans/dashboard`
- `GET /api/v1/fans/meta`
- `GET /api/v1/fans/config`
- `PUT /api/v1/fans/config`
- `GET /api/v1/fans/reference`
- `GET /api/v1/db/fans`
- `POST /api/v1/db/fans`
- `PUT /api/v1/db/fans/{fan_id}`
- `POST /api/v1/db/fans/{fan_id}/soft-delete`
- `POST /api/v1/db/fans/{fan_id}/restore`
- `DELETE /api/v1/db/fans/{fan_id}`
