# Firmware ESP32-S3

Le firmware utilise PlatformIO, ESP-IDF et LVGL 8.4 avec la définition de carte custom `jc3248w535c`.

## Environnements de build

- `pulsmon-esp32s3-display` : build release par défaut ;
- `pulsmon-esp32s3-display-dev` : build debug avec flags de diagnostics PulseMon.

## Démarrage

`esp/src/main.c` initialise :

1. écran et UI générée ;
2. support des icônes météo ;
3. gestionnaire Wi-Fi, NVS et callbacks de cycle AP ;
4. services météo et actualités ;
5. connexion Wi-Fi ;
6. serveur HTTP de configuration et DNS captif uniquement lorsque l’AP de configuration démarre réellement ;
7. poller backend après connexion station.

## Écrans actifs et navigation

```text
Main --gauche--> GPU --gauche--> Météo
Main <--droite-- GPU <--droite-- Météo
```

Seuls Main, GPU et Météo sont des cibles de navigation valides.

## Polling backend

`PULSEMON_DASHBOARD_POLL_MS` vaut par défaut `1000 ms`.

- Écran GPU : requête `/api/v1/gpu/dashboard`.
- Autres écrans actifs : requête `/api/v1/dashboard`.
- En cas d’échec backend : conserver les dernières valeurs, marquer le backend offline et basculer automatiquement vers Météo.
- Au retour du backend après bascule offline automatique : revenir sur Main.

L’hôte et le port backend sont chargés depuis le namespace NVS `pulsemon_api`. `pulsemon_api_config.h` fournit l’hôte/port de fallback compilés et les valeurs fixes de timeout/polling. Les changements du portail sont rechargés immédiatement. Le firmware n’utilise ni découverte backend ni header de clé API. Le portail n’est pas un service LAN permanent : HTTP et DNS captif démarrent avec l’AP de configuration et s’arrêtent avec lui. Un appui maintenu cinq secondes dans le coin supérieur gauche de Main, GPU ou Météo ouvre une fenêtre manuelle de dix minutes en conservant la connexion station. Le déclencheur est implémenté dans le runtime non généré et ne modifie aucune sortie EEZ.

## Météo et actualités

Météo et GNews fonctionnent indépendamment du backend dès que le Wi-Fi et les clés nécessaires sont disponibles. Les deux utilisent HTTPS avec validation des certificats via le bundle ESP-IDF et conservent leur état local selon leur implémentation.

## Fichiers EEZ

- `esp/src/ui/` est la sortie générée compilée par le firmware. Ne jamais la modifier directement.
- `esp/eez/pulsmon/` est le projet EEZ Studio et l’état sauvegardé de l’application. Modifier uniquement via EEZ Studio.
- L’intégration runtime non générée reste hors de ces fichiers générés.

La sortie générée conserve un ancien écran FAN inaccessible et les bindings de compatibilité requis. Il n’est pas chargé par la navigation active et n’est adossé à aucune API FAN générique. Le retirer uniquement via EEZ Studio puis régénération.
