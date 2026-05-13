# Firmware ESP32-S3

Le firmware est construit avec ESP-IDF et LVGL. Il affiche les métriques backend sur un écran local et fournit des fonctions autonomes côté appareil.

## Responsabilités principales

Le firmware doit :

- se connecter au Wi-Fi ;
- ouvrir un point d’accès et un portail de configuration si nécessaire ;
- interroger l’API backend ;
- parser le JSON de manière sûre ;
- cacher les dernières valeurs valides ;
- mettre à jour les variables et écrans LVGL ;
- préserver l’état d’affichage pendant les erreurs réseau temporaires ;
- afficher l’état de connectivité backend et la fraîcheur des données ;
- maintenir météo et actualités indépendantes de la disponibilité backend.

## Écrans

Les écrans actifs incluent :

- Main : synthèse CPU, RAM et GPU depuis `/api/v1/dashboard` ;
- GPU : détail GPU AMD depuis `/api/v1/gpu/dashboard` ;
- Weather : météo autonome et ligne d’information.

Des sources UI ventilateurs peuvent rester présentes. La navigation et le polling runtime doivent refléter uniquement le comportement effectivement câblé.

## Polling

Le poller principal est contrôlé par `PULSEMON_DASHBOARD_POLL_MS`, par défaut `1000` ms.

Comportement runtime courant :

- l’écran Main interroge `/api/v1/dashboard` ;
- l’écran GPU interroge `/api/v1/gpu/dashboard` ;
- l’écran Weather limite le polling backend et n’utilise pas le backend pour la météo ;
- les graphes peuvent être alimentés par des échantillons d’affichage locaux si l’historique backend n’est pas consommé.

## Parsing JSON

Le parseur doit utiliser cette priorité :

1. `value_display` ;
2. `value_raw` ;
3. valeur scalaire directe si compatibilité nécessaire.

Un JSON invalide ou incomplet doit être rejeté sans effacer les dernières valeurs UI valides.

## Navigation

Modèle swipe documenté :

- Main vers GPU par swipe gauche ;
- GPU vers Main par swipe droite ;
- GPU vers Weather par swipe gauche ;
- Weather vers GPU par swipe droite.

## Fichiers UI générés

Les fichiers générés sous `src/ui/` ne doivent pas être modifiés directement. Toute modification nécessitant variables, glyphes ou layout doit être faite dans le projet UI source puis régénérée.

## Comportement en échec

En cas d’échec backend, le firmware doit :

- conserver les dernières valeurs affichables ;
- marquer le backend offline ou stale ;
- ne pas bloquer le thread UI ;
- retenter selon le cycle nominal ;
- garder la météo et les news autonomes opérationnelles si Wi-Fi et clés sont valides.
