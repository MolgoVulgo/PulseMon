# Architecture

PulseMon comporte deux runtimes actifs.

## Backend Linux

Point d’entrée : `api/app/main.py`.

Responsabilités :

- collecter la télémétrie CPU, mémoire et GPU AMD ;
- normaliser les valeurs et préserver les états invalides explicites ;
- publier les snapshots principal et GPU ;
- maintenir des historiques bornés en mémoire ;
- exposer l’API HTTP et l’UI locale de debug.

Les handlers HTTP consomment les services et stores. Ils ne possèdent pas la boucle d’échantillonnage haute fréquence. Le backend n’utilise aucune base persistante.

## Firmware ESP32-S3

Point d’entrée : `esp/src/main.c`.

Responsabilités :

- gérer le Wi-Fi et le portail local de configuration limité à l’AP de configuration ;
- interroger les endpoints dashboard backend ;
- parser le JSON et préserver les dernières valeurs valides ;
- mettre à jour les variables runtime et écrans LVGL ;
- récupérer directement météo et contenu GNews ;
- stocker l’hôte/port backend, les réglages Wi-Fi, météo et actualités en NVS.

L’endpoint backend est chargé depuis le namespace NVS `pulsemon_api`. `esp/src/pulsemon_api_config.h` fournit l’hôte/port de fallback compilés ainsi que les valeurs fixes de polling et timeout. Il n’existe pas de découverte automatique du backend. Le portail HTTP suit le cycle réel de l’AP : démarrage sur `WIFI_EVENT_AP_START`, arrêt sur `WIFI_EVENT_AP_STOP`, avec refus des requêtes lorsque l’AP est inactif. Le DNS captif est lié uniquement à l’adresse `192.168.4.1` de l’AP de configuration. Un hotspot invisible ajouté au runtime sur Main, GPU et Météo ouvre l’AP après un appui de cinq secondes dans le coin supérieur gauche. La fenêtre manuelle dure dix minutes tout en conservant une connexion station fonctionnelle.

## Navigation firmware active

```text
Main <-> GPU <-> Météo
```

## Propriété des données

```text
Backend : télémétrie Linux, payloads API, historiques bornés en mémoire
Firmware : état Wi-Fi, cache affichage, navigation LVGL, météo/news, réglages NVS
```

## Propriété EEZ

- `esp/src/ui/` : sortie générée compilée par le firmware ; ne jamais modifier directement.
- `esp/eez/pulsmon/` : projet EEZ Studio et état sauvegardé ; modifier uniquement via EEZ Studio.
- L’intégration runtime hors fichiers générés appartient à `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` et aux modules non générés associés.

L’UI générée conserve un ancien écran FAN inaccessible et des bindings de compatibilité. Ces artefacts ne définissent aucune route, API ou cible de navigation active. Leur retrait exige EEZ Studio et une régénération.

## Transport

La télémétrie backend utilise HTTP local et JSON. OpenWeather et GNews utilisent HTTPS avec validation des certificats via le bundle ESP-IDF. GNews utilise aussi un header `X-Api-Key`.
