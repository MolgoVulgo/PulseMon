# Backend Linux

Le backend est un service Python 3.11+/FastAPI.

## Structure runtime

- collecteurs : `api/app/collectors/` ;
- modèles de réponse : `api/app/models/` ;
- services d’orchestration : `api/app/services/` ;
- snapshots et historiques courants : `api/app/store/` ;
- point d’entrée HTTP : `api/app/main.py` ;
- UI locale : `api/app/ui.py` et `api/app/ui/index.html`.

## Échantillonnage et publication

Valeurs par défaut de `api/app/config.py` :

- acquisition capteurs : `0.1 s` ;
- publication snapshot/historique : `0.5 s` ;
- capacité d’historique : `600` entrées ;
- alpha EMA d’affichage : `0.25`.

Les samplers principal et GPU publient vers des stores de snapshot et d’historique séparés. Les handlers HTTP lisent ces stores ; ils n’exécutent pas le chemin normal d’acquisition capteur.

Les métriques absentes restent présentes via des enveloppes invalides ou des points d’historique nullables.

## Stockage runtime

Le backend n’utilise aucune base persistante. Les snapshots et historiques bornés sont conservés en mémoire et reconstruits après redémarrage. Les captures de diagnostic, lorsqu’elles sont explicitement activées, sont écrites vers les chemins JSONL configurés et ne font pas partie du modèle d’état API.

## UI locale

`/ui` affiche les métriques principales et GPU courantes ainsi que l’historique principal. Il s’agit d’une surface locale de debug/observation, pas d’une interface d’administration.

## Clé API optionnelle

Lorsque `STATS_API_KEY` est définie, les routes `/api/v1/*` exigent le header configuré, par défaut `X-API-Key`.

Limite d’intégration courante : le firmware ESP32 et l’UI web backend n’ajoutent pas ce header. Le déploiement local standard laisse donc `STATS_API_KEY` non définie jusqu’à adaptation des clients.

## Validation stricte de la configuration

Les 20 variables backend `STATS_*` sont parsées centralement par `api/app/config.py`. Le démarrage rejette les valeurs mal formées, les nombres non finis, les intervalles et capacités hors bornes, les booléens invalides, les noms de header HTTP invalides, les BDF PCI invalides et les listes de labels température GPU invalides.

Le lanceur packagé exécute `python -m app.config` avant Uvicorn, et l’import de `app.main` valide le même contrat avant la construction des services runtime. Les messages d’erreur indiquent la variable et le contrat attendu sans reprendre les valeurs sensibles.
