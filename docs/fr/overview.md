# Vue d’ensemble

PulseMon est une pile de supervision locale composée d’un backend Linux et d’un firmware d’affichage ESP32-S3.

Le backend Linux collecte la télémétrie de l’hôte et l’expose via une API HTTP compacte. Le firmware ESP32-S3 interroge l’API, parse les données, conserve les dernières valeurs valides et affiche l’information via LVGL.

Le projet cible une station personnelle sur réseau local privé. Il privilégie le comportement déterministe, les payloads compacts, le déploiement simple et l’autonomie locale plutôt que les fonctions de supervision lourdes.

## Périmètre

PulseMon couvre :

- utilisation, température et puissance optionnelle CPU ;
- utilisation et capacité mémoire ;
- utilisation, température, puissance et écran détaillé GPU AMD ;
- supervision ventilateurs et mapping configurable ;
- historique court pour graphes ;
- UI web locale de debug/admin ;
- écrans ESP32-S3 pour métriques principales, GPU, météo et ligne d’information ;
- récupération autonome météo et actualités côté ESP32-S3.

## Modèle runtime

Le backend échantillonne l’hôte à cadence fixe, stocke le snapshot courant en mémoire, stocke un historique court dans un ring buffer, puis sert les réponses API depuis cet état mémoire.

Le firmware ne calcule jamais les métriques Linux. Il consomme le contrat backend, accepte les valeurs nulles pour les métriques indisponibles et conserve le dernier état d’affichage valide en cas d’erreur réseau ou de parsing.

## Hors périmètre

PulseMon n’est pas un service cloud de monitoring, une plateforme d’observabilité multi-hôtes, un profiler de processus, un outil de pilotage distant ni une API publique exposée sur Internet.

Il ne nécessite pas MQTT, broker, stockage long terme en base, authentification multi-utilisateur forte ni dashboard web riche pour l’usage nominal.
