# Firmware ESP32-S3

Le firmware utilise PlatformIO, ESP-IDF et LVGL 8.4 avec la définition de carte custom `jc3248w535c`.

## Environnements de build

- `pulsmon-esp32s3-display` : build release par défaut ;
- `pulsmon-esp32s3-display-dev` : build debug avec flags de diagnostics PulseMon.

## Démarrage

`esp/src/main.c` initialise :

1. écran et UI générée ;
2. support des icônes météo ;
3. gestionnaire Wi-Fi et NVS ;
4. services météo et news ;
5. serveur HTTP local de configuration ;
6. DNS captif ;
7. connexion Wi-Fi et poller backend après connexion station.

## Écrans actifs et navigation

```text
Main --gauche--> GPU --gauche--> Météo
Main <--droite-- GPU <--droite-- Météo
```

L’écran FAN existe dans les sources générées mais n’est pas une cible de navigation valide.

## Polling backend

`PULSEMON_DASHBOARD_POLL_MS` vaut par défaut `1000 ms`.

- Écran GPU : requête `/api/v1/gpu/dashboard`.
- Autres écrans actifs : requête `/api/v1/dashboard`.
- En cas d’échec backend : conserver les dernières valeurs, marquer le backend offline et basculer automatiquement vers Météo.
- Au retour du backend après bascule offline automatique : revenir sur Main.

L’URL backend est compilée dans `pulsemon_api_config.h`. Le firmware n’utilise ni découverte backend, ni configuration backend en NVS, ni header de clé API.

## Météo et actualités

Météo et GNews fonctionnent indépendamment du backend dès que le Wi-Fi et les clés nécessaires sont disponibles. Les services conservent leur état local selon leur implémentation.

## Fichiers EEZ

- `esp/src/ui/` est la sortie générée compilée par le firmware. Ne jamais la modifier directement.
- `esp/eez/pulsmon/` est le projet EEZ Studio et l’état sauvegardé de l’application. Modifier uniquement via EEZ Studio.
- L’intégration runtime non générée reste hors de ces fichiers générés.

## Code FAN conservé

Les structures et helpers FAN restent présents, mais le poller firmware ne récupère pas les données FAN et la navigation active ne peut pas charger l’écran FAN.
