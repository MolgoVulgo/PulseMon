# PulseMon

PulseMon est un système de supervision locale composé d’un backend Linux et d’un afficheur ESP32-S3.

L’hôte Linux collecte les métriques système et les expose via une API HTTP FastAPI. L’ESP32-S3 interroge cette API, conserve les dernières valeurs valides dans un cache local et affiche les écrans actifs avec LVGL. Le déploiement cible est une station personnelle sur réseau local privé, sans cloud obligatoire ni broker externe. L’écran Printer optionnel utilise une liaison HTTP/MQTT directe et locale entre l’ESP32-S3 et l’imprimante configurée ; il n’ajoute ni broker PulseMon ni dépendance cloud.

## Périmètre actif

Le runtime courant fournit :

- utilisation, température et puissance CPU optionnelle ;
- utilisation et capacité mémoire ;
- utilisation GPU AMD, fréquences, VRAM, température, puissance et ventilateur GPU lorsque disponibles ;
- snapshots courants et historiques mémoire bornés ;
- UI locale backend de debug sous `/ui` ;
- écrans ESP32-S3 actifs Main, GPU et Météo, plus un écran Printer accessible uniquement tant que l’imprimante configurée est joignable ;
- météo OpenWeather et brèves GNews autonomes côté ESP32-S3 ;
- télémétrie imprimante en lecture seule récupérée directement par l’ESP32-S3 sur le réseau local ;
- persistance NVS de l’endpoint backend, du Wi-Fi, de l’imprimante, de la météo et des actualités côté ESP32-S3 ;
- portail de configuration et DNS captif exposés uniquement lorsque l’AP de configuration est actif, avec un appui tactile explicite de cinq secondes pour ouvrir une fenêtre manuelle bornée.

Le backend n’utilise aucune base persistante. Les snapshots et historiques sont reconstruits après redémarrage.

## Structure du dépôt

```text
PulseMon/
├── api/                         # backend Linux
├── esp/                         # firmware ESP32-S3
│   ├── src/ui/                  # UI générée par EEZ et compilée par le firmware
│   └── eez/pulsmon/             # projet EEZ Studio et état sauvegardé de l’application
├── docs/                        # documentation canonique anglaise
│   └── fr/                      # miroir français
├── tools/                       # runner reproductible de validation P8
├── make-a.sh                    # génère le snapshot PulseMon.zip volontairement filtré
├── README.md
└── README.fr.md
```

`docs/` est la racine documentaire canonique. Dans la source publiée, `api/docs` et `esp/docs` contiennent la cible relative `../docs` ; ils représentent l’intention de liaison vers la documentation canonique. Google Drive ne permet pas de prouver le type d’objet filesystem d’origine ; ce type doit donc être vérifié dans le worktree d’application avant de modifier l’un de ces chemins.

## Installation et lancement backend

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

L’API est exposée sous `/api/v1/*` et l’UI locale sous `/ui`.

Valider l’environnement backend avant démarrage :

```bash
cd api
python3 -m app.config
```

Tests backend :

```bash
cd api
.venv/bin/pytest -q
```

## Build firmware

Environnement principal :

```bash
cd esp
pio run -e pulsmon-esp32s3-display
```

Environnement de développement avec flags de debug PulseMon :

```bash
cd esp
pio run -e pulsmon-esp32s3-display-dev
```

L’environnement PlatformIO par défaut est `pulsmon-esp32s3-display`.

## Runner de validation P8

Pour que Codex exécute la validation P8 complète — tests backend, builds release/debug, flash release, monitor série et contrôles API sur `192.168.0.10:8000` :

```bash
python3 tools/p8_validate.py --codex-full
```

Le premier rapport génère une checklist matérielle. Codex doit ensuite finaliser le même rapport avec `--resume-report`, `--checklist-file` et `--require-hardware`. Voir `docs/fr/p8-validation.md`.

## Modèle de configuration firmware

L’hôte et le port backend, les identifiants Wi-Fi, l’endpoint/code d’accès Printer, les paramètres OpenWeather et les paramètres GNews sont stockés en NVS via le portail local. La connexion Printer utilise le namespace NVS `printer`, clés `host` et `access_code` ; `esp/src/printer_config.h` ne contient plus que les ports/timeouts protocolaires fixes et aucun credential.

L’endpoint backend utilise :

- le namespace NVS `pulsemon_api`, clés `host` et `port` ;
- les fallbacks compilés `PULSEMON_API_DEFAULT_HOST` et `PULSEMON_API_DEFAULT_PORT` de `esp/src/pulsemon_api_config.h` ;
- les valeurs compilées `PULSEMON_HTTP_TIMEOUT_MS` et `PULSEMON_DASHBOARD_POLL_MS`.

Une sauvegarde via le portail recharge immédiatement l’endpoint backend et réveille un service Printer actif afin que les nouveaux réglages Printer soient pris en compte sans reboot. L’effacement de la configuration PulseMon supprime les réglages NVS backend et Printer et restaure le fallback backend compilé. La clé API backend optionnelle n’est ni stockée ni envoyée par le firmware courant. Le service imprimante est strictement en lecture seule : il exécute uniquement le bootstrap et la lecture d’état nécessaires à l’affichage et n’expose aucune commande d’impression.

## Positionnement sécurité

PulseMon cible un usage local et personnel. La sécurité reste proportionnée à ce contexte : les secrets ne doivent pas être logués ni retournés par les endpoints de configuration, les entrées restent validées et les expositions LAN inutiles doivent être évitées. Le serveur HTTP de configuration démarre uniquement avec l’AP de configuration et s’arrête avec lui ; le DNS captif est lié exclusivement à `192.168.4.1`. Un appui maintenu cinq secondes dans le coin supérieur gauche de Main, GPU ou Météo ouvre `PulseMon-Setup` pendant une fenêtre manuelle de cinq minutes, sans effacer les identifiants ni couper une liaison station fonctionnelle.

OpenWeather et GNews utilisent HTTPS avec validation des certificats via le bundle ESP-IDF. GNews utilise en complément le header `X-Api-Key`. Le code d’accès imprimante est un secret stocké en NVS : il ne doit être ni journalisé ni retourné par l’API de configuration. `printer_config.h` ne contient plus aucun placeholder de credential.

## Propriété EEZ et artefact généré conservé

- `esp/src/ui/` est une sortie générée compilée par le firmware et ne doit jamais être modifiée directement.
- `esp/eez/pulsmon/` appartient au workflow EEZ Studio et ne doit pas être modifié hors EEZ Studio.
- L’intégration runtime appartient aux modules non générés sous `esp/src/`.

L’UI générée contient encore un ancien écran FAN inaccessible et les bindings de compatibilité nécessaires à la compilation de cette sortie. Cet écran ne fait pas partie de la navigation active ni du contrat backend. Son retrait physique exige une modification dans EEZ Studio puis une régénération.

## Publication du projet et livraison des correctifs

La publication Google Drive connectée `pulsemon/` constitue la source de contexte par défaut pour les analyses et correctifs. `REPO_INDEX.json` décrit l’ensemble source actuellement publié ; les fichiers ciblés doivent être à la fois déclarés dans cet index et réellement lisibles depuis Drive. Une publication plus récente de l’index remplace la baseline de contexte précédente.

Les archives de correctif sont livrées séparément sous `pulsemon/patch/`. Ce dossier n’est pas une source projet, est exclu de `REPO_INDEX.json` et ne doit pas être interprété comme une chaîne de patchs implicitement appliquée.

`make-a.sh` continue de produire l’export local filtré `PulseMon.zip` pour les workflows qui demandent explicitement un ZIP. Il n’est plus la source de contexte par défaut lorsque la publication Drive est disponible, sauf si l’utilisateur désigne explicitement un ZIP précis comme base de travail.

## Documentation

- `docs/fr/README.md` — index documentaire ;
- `docs/fr/overview.md` — périmètre actif et non-objectifs ;
- `docs/fr/architecture.md` — responsabilités et flux ;
- `docs/fr/api.md` — contrat HTTP backend ;
- `docs/fr/backend.md` — runtime backend ;
- `docs/fr/firmware.md` — runtime firmware et écrans actifs ;
- `docs/fr/configuration.md` — environnement backend et configuration firmware ;
- `docs/fr/weather-news.md` — implémentation météo et actualités ;
- `docs/fr/web-configuration.md` — portail local ESP32 ;
- `docs/fr/development.md` — build, tests, publication Drive et workflow de correctif ;
- `docs/fr/p8-validation.md` — protocole P8 de build et validation matérielle ;
- `docs/fr/troubleshooting.md` — vérifications opérationnelles.
