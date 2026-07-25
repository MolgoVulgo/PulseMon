# Vue d’ensemble

PulseMon est une pile de supervision locale pour une station Linux et un afficheur ESP32-S3 sur réseau privé.

## Périmètre actif

Le backend Linux fournit :

- utilisation, température et puissance CPU optionnelle ;
- mémoire utilisée, totale et pourcentage ;
- utilisation GPU AMD, fréquences, VRAM, température, puissance et ventilateur GPU lorsque le pilote les expose ;
- snapshots courants des dashboards principal et GPU ;
- historiques principal et GPU bornés en mémoire ;
- UI locale de debug sous `/ui`.

L’ESP32-S3 fournit :

- configuration Wi-Fi station avec AP de secours ;
- polling des dashboards backend principal et GPU ;
- cache local des dernières valeurs valides ;
- écrans actifs Main, GPU et Météo ;
- récupération autonome de la météo et des brèves GNews ;
- configuration web locale de l’endpoint backend et des réglages Wi-Fi, météo et actualités lorsque l’AP de configuration est actif, avec un déclencheur tactile local de cinq secondes pour une fenêtre manuelle bornée.

## Modèle runtime

```text
Capteurs Linux -> collecteurs -> services -> stores mémoire -> JSON FastAPI
JSON FastAPI -> client HTTP ESP -> parsing/cache -> variables runtime -> écrans LVGL
```

Météo et actualités sont récupérées directement par l’ESP32-S3 et ne transitent pas par le backend Linux.

## Modèle de persistance

- snapshots et historiques backend : mémoire volatile uniquement ;
- endpoint backend, réglages Wi-Fi, météo et actualités ESP32 : NVS ;
- fallback endpoint, timeout HTTP et intervalle de polling : configuration compilée dans le firmware ;
- stockage long terme des métriques : non implémenté.

## Non-objectifs

Le projet courant n’est pas un service cloud, une plateforme multi-hôtes, un système de contrôle distant, une base de métriques long terme ni une API Internet publique. MQTT, broker externe, sous-système générique de gestion FAN et sécurité multi-utilisateur forte sont hors du design actif.
