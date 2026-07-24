# Vue d’ensemble

PulseMon est une pile de supervision locale pour une station Linux et un afficheur ESP32-S3 sur réseau privé.

## Périmètre actif

Le backend Linux fournit actuellement :

- utilisation, température et puissance CPU optionnelle ;
- mémoire utilisée, totale et pourcentage ;
- utilisation GPU AMD, fréquences, VRAM, températures, puissance et ventilateur GPU lorsque le pilote les expose ;
- snapshots dashboard courants ;
- historiques principal et GPU bornés en mémoire ;
- UI locale de debug/admin ;
- configuration utilisateur stockée en SQLite.

L’ESP32-S3 fournit actuellement :

- configuration Wi-Fi station avec point d’accès de secours ;
- polling des dashboards principal et GPU ;
- cache local des dernières valeurs valides ;
- écrans actifs Main, GPU et Météo ;
- récupération autonome météo et brèves GNews ;
- configuration web locale du Wi-Fi, de la météo et des actualités.

## Code FAN conservé

La collecte ventilateurs, les mappings, routes API et données d’administration restent implémentés côté backend. Le client firmware et des éléments UI associés restent également présents, mais FAN n’est pas actif dans la navigation courante et n’est pas interrogé par le firmware.

## Modèle runtime

```text
capteurs Linux -> collecteurs -> services -> stores mémoire -> JSON FastAPI
JSON FastAPI -> client HTTP ESP -> parsing/cache -> variables runtime -> écrans LVGL
```

La météo et les actualités sont récupérées directement par l’ESP32-S3 et ne transitent pas par le backend Linux.

## Hors périmètre

Le projet courant n’est pas un service cloud, une plateforme multi-hôtes, un système de pilotage distant, une base de métriques long terme ni une API publique Internet. MQTT, broker externe et sécurité multi-utilisateur forte ne font pas partie de l’architecture active.
