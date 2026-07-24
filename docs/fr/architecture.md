# Architecture

PulseMon possède deux runtimes actifs.

## Backend Linux

Point d’entrée : `api/app/main.py`.

Responsabilités :

- collecter CPU, mémoire, GPU AMD et télémétrie ventilateurs conservée ;
- normaliser les valeurs et préserver les états invalides explicites ;
- publier les snapshots principal et GPU ;
- maintenir des historiques mémoire bornés ;
- exposer l’API HTTP et l’UI locale ;
- persister la configuration utilisateur et les mappings FAN conservés en SQLite.

Les handlers HTTP consomment les services et stores. Ils n’exécutent pas eux-mêmes la boucle normale d’échantillonnage haute fréquence.

## Firmware ESP32-S3

Point d’entrée : `esp/src/main.c`.

Responsabilités :

- gérer le Wi-Fi et le portail local ;
- interroger les endpoints dashboard backend ;
- parser le JSON et conserver les dernières valeurs valides ;
- mettre à jour les variables runtime et les écrans LVGL ;
- récupérer directement météo et contenu GNews ;
- stocker les paramètres appareil en NVS.

L’endpoint backend est compilé dans `esp/src/pulsemon_api_config.h`. Il n’existe pas de découverte backend active ni de paramètre d’adresse backend en NVS.

## Navigation firmware active

```text
Main <-> GPU <-> Météo
```

L’écran FAN généré et ses helpers restent dans l’arborescence mais sont volontairement exclus de la navigation active.

## Propriété des données

```text
Backend : télémétrie Linux, payloads API, historiques mémoire, configuration SQLite
Firmware : état Wi-Fi, cache d’affichage, navigation LVGL, météo/news, paramètres NVS
```

## Propriété EEZ

- `esp/src/ui/` : sortie générée compilée par le firmware ; ne jamais modifier directement.
- `esp/eez/pulsmon/` : projet EEZ Studio et état sauvegardé de l’application ; modifier uniquement via EEZ Studio.
- L’intégration runtime hors fichiers générés appartient à `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` et modules non générés associés.

## Transport

La télémétrie backend utilise HTTP local avec JSON. La météo utilise actuellement OpenWeather en HTTP clair. GNews utilise HTTPS et le header `X-Api-Key`.
