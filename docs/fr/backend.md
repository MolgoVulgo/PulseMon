# Backend Linux

Le backend est un service Python/FastAPI qui collecte la télémétrie Linux et l’expose via des endpoints HTTP locaux.

## Responsabilités runtime

Le backend doit :

- collecter les métriques CPU, mémoire, GPU AMD et ventilateurs ;
- normaliser les unités ;
- appliquer des règles de fallback stables ;
- rendre les valeurs absentes explicites ;
- maintenir un snapshot courant ;
- maintenir un historique en mémoire borné ;
- sérialiser un JSON compact ;
- exposer des diagnostics sans bruit excessif ;
- éviter les accès capteurs lourds dans les handlers HTTP.

## Découpage recommandé

Une implémentation propre sépare :

- chargement de configuration ;
- modèles domaine et API ;
- collecteurs capteurs ;
- normalisation ;
- stores snapshot et historique ;
- services d’assemblage des réponses ;
- handlers HTTP ;
- diagnostics et captures brutes.

## Échantillonnage et publication

Le backend utilise deux cadences runtime :

- acquisition capteurs, contrôlée par `STATS_SAMPLE_INTERVAL_S` ;
- publication snapshot/historique, contrôlée par `STATS_PUBLISH_INTERVAL_S`.

Le comportement par défaut peut échantillonner plus vite qu’il ne publie, puis exposer des valeurs prêtes à afficher depuis la mémoire.

## Historique

L’historique est conservé en mémoire avec une capacité bornée par `STATS_HISTORY_CAPACITY`.

Les réponses d’historique doivent rester compactes, alignées et bornées. Le client embarqué doit recevoir des tableaux directement exploitables.

## Lissage

Un lissage d’affichage peut être appliqué aux pourcentages avant publication. Le comportement documenté utilise une médiane courte puis un EMA contrôlé par `STATS_DISPLAY_EMA_ALPHA`.

Les valeurs brutes restent utiles aux diagnostics. Les valeurs d’affichage sont préférées pour la stabilité UI.

## Politique d’échec capteur

L’échec d’une métrique ne doit pas effondrer tout le snapshot.

Règles :

- conserver le champ ;
- marquer la métrique invalide ou retourner `null` selon le format ;
- préserver l’état service cohérent ;
- loguer des diagnostics exploitables ;
- ne pas fabriquer de valeur.

## Authentification

L’authentification est optionnelle pour usage LAN privé. Si `STATS_API_KEY` est configurée, les routes API exigent le header configuré, par défaut `X-API-Key`.
