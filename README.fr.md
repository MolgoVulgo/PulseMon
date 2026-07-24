# PulseMon

PulseMon est un système de supervision locale composé d’un backend Linux et d’un afficheur ESP32-S3.

L’hôte Linux collecte les métriques système et les expose via une API HTTP FastAPI. L’ESP32-S3 interroge cette API, conserve les dernières valeurs valides dans un cache local et affiche les écrans actifs avec LVGL. Le déploiement cible est une station personnelle sur réseau local privé, sans MQTT, cloud obligatoire ni broker externe.

## Périmètre actif

Le runtime courant fournit :

- utilisation, température et puissance CPU optionnelle ;
- utilisation et capacité mémoire ;
- utilisation GPU AMD, fréquences, VRAM, température, puissance et ventilateur GPU lorsque disponibles ;
- snapshots courants et historiques mémoire bornés ;
- UI locale backend de debug/admin sous `/ui` ;
- écrans ESP32-S3 actifs Main, GPU et Météo ;
- météo OpenWeather et brèves GNews autonomes côté ESP32-S3 ;
- stockage de configuration local en SQLite côté backend et en NVS côté ESP32-S3.

## Sous-système FAN conservé

Le backend contient encore les collecteurs ventilateurs, le stockage des mappings, les endpoints API et les fonctions d’administration. Des types firmware, éléments UI générés et helpers runtime dormants restent également présents.

Ce sous-système n’est plus une fonctionnalité produit active :

- l’écran FAN n’est pas accessible depuis la navigation firmware active ;
- le poller firmware n’interroge pas le dashboard ventilateurs ;
- le code est conservé pour l’instant et ne doit pas être traité comme une cible active sans réactivation explicite.

## Structure du dépôt

```text
PulseMon/
├── api/                         # backend Linux
├── esp/                         # firmware ESP32-S3
│   ├── src/ui/                  # UI générée par EEZ et compilée par le firmware
│   └── eez/pulsmon/             # projet EEZ Studio et état sauvegardé de l’application
├── docs/                        # documentation canonique anglaise
│   └── fr/                      # documentation française
├── make-a.sh                    # génère le snapshot PulseMon.zip volontairement filtré
├── README.md
└── README.fr.md
```

`docs/` est la racine documentaire canonique. Dans le snapshot fourni, `api/docs` et `esp/docs` contiennent la cible relative `../docs` ; ils représentent l’intention de liaison vers la documentation canonique.

## Installation backend

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Lancement backend

```bash
cd api
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

L’API est exposée sous `/api/v1/*` et l’UI locale sous `/ui`.

## Tests backend

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

## Modèle courant de configuration firmware

Les identifiants Wi-Fi, paramètres OpenWeather et paramètres GNews sont stockés en NVS via le portail local.

L’adresse backend n’est pas stockée en NVS dans l’implémentation actuelle. Elle est compilée depuis `esp/src/pulsemon_api_config.h` via :

- `PULSEMON_API_HOST` ;
- `PULSEMON_API_PORT` ;
- `PULSEMON_API_BASE_URL` ;
- `PULSEMON_HTTP_TIMEOUT_MS` ;
- `PULSEMON_DASHBOARD_POLL_MS`.

Le firmware n’envoie actuellement pas le header optionnel de clé API backend.

## Positionnement sécurité

PulseMon cible un usage local et personnel. La sécurité reste proportionnée à ce contexte : les secrets ne doivent pas être logués ni retournés par les endpoints de configuration, les entrées restent validées et les expositions LAN inutiles doivent être évitées.

Défaut connu : le client OpenWeather utilise actuellement HTTP en clair. GNews utilise déjà HTTPS et le header `X-Api-Key`.

## Génération du snapshot

`make-a.sh` crée le snapshot `PulseMon.zip` utilisé pour les analyses et correctifs. Il exclut volontairement les builds locaux, environnements virtuels, caches, `tmp/`, configurations SDK locales et autres contenus propres à la machine. L’archive est donc un snapshot contrôlé du projet, pas une copie exhaustive du répertoire de travail.

## Documentation

- `docs/fr/README.md` — index documentaire ;
- `docs/fr/overview.md` — périmètre courant et modèle runtime ;
- `docs/fr/architecture.md` — responsabilités et flux ;
- `docs/fr/api.md` — contrat HTTP backend ;
- `docs/fr/backend.md` — comportement backend ;
- `docs/fr/firmware.md` — comportement firmware et écrans actifs ;
- `docs/fr/configuration.md` — environnement, SQLite et NVS ;
- `docs/fr/fans.md` — statut du sous-système FAN conservé ;
- `docs/fr/weather-news.md` — implémentation météo et actualités ;
- `docs/fr/web-configuration.md` — portail local ESP32 ;
- `docs/fr/development.md` — build, tests et génération du snapshot ;
- `docs/fr/troubleshooting.md` — vérifications opérationnelles.
