# Architecture

PulseMon est séparé en deux composants runtime : le backend Linux et le firmware ESP32-S3.

## Backend Linux

Le backend est responsable de :

- collecter les métriques Linux ;
- lire la télémétrie CPU, mémoire, GPU AMD et ventilateurs ;
- appliquer les fallbacks capteurs ordonnés ;
- normaliser les unités et la précision numérique ;
- conserver le snapshot courant en mémoire ;
- conserver un historique court en mémoire ;
- exposer l’API HTTP ;
- exposer une UI locale de debug/admin ;
- persister les configurations utilisateur et ventilateurs si nécessaire.

Le backend ne doit pas faire de lectures capteurs lourdes dans les handlers HTTP. Les handlers lisent le store mémoire déjà normalisé.

## Firmware ESP32-S3

Le firmware est responsable de :

- se connecter au Wi-Fi ;
- fournir un point d’accès et un portail captif si nécessaire ;
- localiser ou utiliser l’adresse backend configurée ;
- interroger les endpoints backend ;
- parser les payloads JSON ;
- maintenir les caches d’affichage locaux ;
- mettre à jour les écrans LVGL ;
- afficher l’état réseau et la fraîcheur des données ;
- récupérer météo et actualités autonomes si configurées.

Le firmware ne doit pas calculer les métriques Linux, inférer les champs manquants ou dépendre d’une requête HTTP active pendant le rendu.

## Propriété des données

Le backend possède la télémétrie Linux et la génération des payloads API.

Le firmware possède l’état d’affichage, la navigation UI, l’état Wi-Fi, la météo, les actualités autonomes et la configuration locale de l’appareil.

## Transport

Le backend et le firmware communiquent en HTTP local avec JSON compact. Le polling est retenu car le déploiement cible contient un client embarqué principal sur réseau privé.

MQTT et l’architecture à broker ne font pas partie de l’architecture active.

## Séparation runtime

Pipeline firmware :

```text
réseau -> client HTTP -> parseur JSON -> cache local -> variables LVGL -> écrans rendus
```

Pipeline backend :

```text
lectures capteurs -> normalisation -> store snapshot/historique -> sérialisation API
```
