# Backend Linux

Le backend est un service Python 3.11+/FastAPI.

## Structure runtime

- collecteurs : `api/app/collectors/` ;
- modèles de réponse : `api/app/models/` ;
- services d’orchestration : `api/app/services/` ;
- snapshots et historiques : `api/app/store/` ;
- point d’entrée HTTP : `api/app/main.py` ;
- UI locale : `api/app/ui.py` et `api/app/ui/index.html`.

## Échantillonnage et publication

Valeurs par défaut de `api/app/config.py` :

- acquisition capteurs : `0,1 s` ;
- publication snapshot/historique : `0,5 s` ;
- capacité d’historique : `600` entrées ;
- alpha EMA d’affichage : `0,25`.

Les snapshots principal et GPU sont servis depuis la mémoire. Les métriques absentes restent présentes via des enveloppes invalides ou des points d’historique nullables.

## Configuration SQLite

`api/app/store/config_db.py` stocke :

- les `fan_mappings` conservés ;
- les `user_settings` libres ;
- les métadonnées de migration de schéma.

Résolution du chemin :

1. `STATS_CONFIG_DB_PATH` si défini ;
2. `~/.config/pulsemon/config.db` si inscriptible ;
3. fallback `/tmp/pulsemon/config.db`.

Le mode WAL SQLite est activé.

## Clé API optionnelle

Lorsque `STATS_API_KEY` est définie, les routes `/api/v1/*` exigent le header configuré, par défaut `X-API-Key`.

Limite d’intégration courante : le firmware ESP32 et l’UI web backend n’ajoutent pas ce header. Le déploiement local standard laisse donc `STATS_API_KEY` non définie jusqu’à mise à jour de ces clients.

## Statut FAN

L’implémentation FAN backend reste fonctionnelle et testée, mais appartient au périmètre legacy conservé. Elle ne prouve pas que FAN est actif dans le flux produit firmware courant.
