# PulseMon

PulseMon est un système de supervision locale pour une machine Linux et un afficheur ESP32-S3.

Il collecte les métriques Linux sur l’hôte, les expose via une API HTTP locale, puis affiche l’état courant sur un écran dédié piloté par ESP32-S3 et LVGL. Le projet est conçu pour un réseau local privé, sans dépendance cloud obligatoire et sans broker externe.

## Ce que fait le projet

PulseMon fournit :

- un backend Linux en Python/FastAPI ;
- la supervision CPU, mémoire, GPU AMD et ventilateurs ;
- une API HTTP locale consommée par le firmware ESP32-S3 ;
- un historique court en mémoire pour les graphes ;
- une UI web locale de debug et d’administration côté backend ;
- un firmware ESP32-S3 avec configuration Wi-Fi, polling API, rendu LVGL, météo et brèves d’actualité autonomes ;
- un stockage local de configuration côté backend et côté firmware.

## À quoi il sert

PulseMon sert à disposer d’un petit tableau de bord matériel permanent pour une station Linux. Il donne une visibilité immédiate sur le CPU, la RAM, le GPU, les ventilateurs et l’état runtime sans ouvrir de dashboard desktop ni dépendre d’un service distant.

L’ESP32-S3 peut aussi afficher des informations utiles lorsque le PC Linux est indisponible, notamment la météo et des brèves d’actualité, si les clés API nécessaires sont configurées sur l’appareil.

## Structure du dépôt

```text
PulseMon/
├── api/                  # point d’entrée backend Linux et fichiers spécifiques API
├── esp/                  # point d’entrée firmware ESP32-S3 et fichiers spécifiques firmware
├── docs/                 # documentation canonique en anglais
│   └── fr/               # documentation française
├── README.md             # README anglais par défaut
└── README.fr.md          # README français
```

La documentation canonique est stockée dans `/docs`. Les chemins `api/docs` et `esp/docs` sont des liens symboliques vers `/docs`.

## Prérequis

Backend :

- hôte Linux ;
- Python 3.11 ou supérieur ;
- FastAPI ;
- uvicorn ;
- psutil ;
- Pydantic ;
- pytest et httpx pour les tests ;
- accès aux chemins Linux sysfs, hwmon et DRM pour la télémétrie CPU/GPU AMD.

Firmware :

- carte ESP32-S3 avec écran ;
- PlatformIO avec support ESP-IDF ;
- LVGL ;
- accès Wi-Fi ;
- carte SD optionnelle pour les icônes météo ;
- clés OpenWeather et GNews optionnelles pour les modules météo/news autonomes.

## Installer le backend

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Lancer le backend

```bash
cd api
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Le backend expose l’API sous `/api/v1/*` et l’UI locale de debug/admin sous `/ui`.

## Compiler le firmware

```bash
cd esp
pio run -e LVGL-320-480
```

Les identifiants Wi-Fi ne sont pas compilés dans le firmware. Ils sont stockés en NVS via le portail de configuration ESP32-S3.

## Configurer le projet

La configuration backend passe par variables d’environnement. Les paramètres principaux couvrent le bind, le port, la cadence d’échantillonnage, la capacité d’historique, la clé API optionnelle, les indices de détection GPU, les diagnostics et le stockage SQLite local.

La configuration ESP32-S3 passe par NVS et le portail local. Elle stocke les identifiants Wi-Fi, l’adresse API backend, les paramètres OpenWeather, les paramètres GNews et les options d’affichage. Les clés API ne doivent pas être imprimées, loguées ni retournées par les endpoints de configuration.

La configuration détaillée est documentée dans `/docs/fr/configuration.md`.

## Contribuer

Les contributions doivent respecter ces règles :

- conserver la séparation backend, contrat API, polling firmware et rendu LVGL ;
- conserver des payloads JSON stables et compacts ;
- rendre les métriques indisponibles nullables au lieu de supprimer les champs ;
- ne pas modifier directement les fichiers UI générés côté ESP32 ;
- ne jamais exposer les secrets dans les logs, URLs, écrans ou exports de configuration ;
- ajouter des tests pour toute modification du contrat API ;
- mettre à jour `/docs` et `/docs/fr` quand le comportement change.

## Documentation détaillée

- `/docs/fr/README.md` — index de documentation ;
- `/docs/fr/overview.md` — vue d’ensemble ;
- `/docs/fr/architecture.md` — architecture backend/firmware ;
- `/docs/fr/api.md` — contrat API HTTP ;
- `/docs/fr/backend.md` — comportement du backend Linux ;
- `/docs/fr/firmware.md` — comportement du firmware ESP32-S3 ;
- `/docs/fr/configuration.md` — configuration backend et appareil ;
- `/docs/fr/gpu.md` — supervision GPU AMD ;
- `/docs/fr/fans.md` — supervision et configuration ventilateurs ;
- `/docs/fr/weather-news.md` — modules météo et actualités autonomes ;
- `/docs/fr/web-configuration.md` — portail de configuration ESP32 ;
- `/docs/fr/development.md` — build, tests et workflow de contribution ;
- `/docs/fr/troubleshooting.md` — diagnostics et exploitation.
