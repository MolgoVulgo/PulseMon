# Plan d’implémentation API — Stats Linux vers ESP32-S3

## 1. Objet

Ce document définit le plan d’implémentation du backend API, considéré comme le cœur du système.

L’objectif de cette brique est de :

- collecter les bonnes métriques Linux ;
- appliquer des règles de fallback robustes ;
- produire un JSON stable, compact et strictement conforme au contrat ;
- maintenir un historique exploitable par l’ESP32 ;
- garantir une base suffisamment propre pour que l’intégration ESP32 soit essentiellement une phase de consommation et d’affichage.

Le principe directeur est simple : si les métriques sont correctes, si leur fraîcheur est maîtrisée, et si le JSON est stable, la plus grande partie du risque projet est levée.

---

## 2. Objectifs d’implémentation

L’API doit atteindre les objectifs suivants :

1. fournir un snapshot instantané fiable ;
2. fournir un historique court cohérent pour les graphes ;
3. ne jamais casser le contrat JSON en cas de métrique absente ;
4. rester légère en charge CPU ;
5. séparer clairement collecte, normalisation, stockage et exposition HTTP ;
6. rendre le comportement déterministe pour le client ESP32.

---

## 3. Architecture logicielle de l’API

## 3.1 Découpage imposé

Le backend doit être structuré en modules distincts.

### `config`
Responsabilité :
- lecture de configuration ;
- valeurs par défaut ;
- validation des paramètres.

### `models`
Responsabilité :
- modèles de données internes ;
- schémas de réponse ;
- types communs ;
- état des métriques.

### `collectors`
Responsabilité :
- lecture des métriques Linux ;
- accès `psutil` ;
- lecture `sysfs` ;
- lecture `hwmon` ;
- lecture `drm` / `amdgpu`.

### `normalizers`
Responsabilité :
- conversion d’unités ;
- arrondis éventuels ;
- traitement des valeurs absentes ;
- application des règles de fallback.

### `store`
Responsabilité :
- stockage du snapshot courant ;
- stockage de l’historique ;
- gestion du ring buffer ;
- calcul de l’obsolescence.

### `services`
Responsabilité :
- assemblage des données ;
- génération des réponses ;
- validation métier ;
- contrôle de cohérence.

### `api`
Responsabilité :
- endpoints HTTP ;
- validation des paramètres ;
- sérialisation JSON ;
- gestion des codes de retour.

### `diagnostics`
Responsabilité :
- journalisation ;
- métriques internes du service ;
- diagnostics capteurs ;
- inspection du mapping capteurs.

---

## 4. Stack technique recommandée

## 4.1 Langage et runtime

- Python 3.11+

## 4.2 Framework HTTP

- FastAPI
- uvicorn

## 4.3 Collecte système

- psutil pour les métriques CPU/mémoire généralistes
- accès fichier pour `/sys/class/hwmon`
- accès fichier pour `/sys/class/drm/card*/device`

## 4.4 Validation / modèles

- Pydantic

## 4.5 Tests

- pytest
- httpx pour les tests d’API

---

## 5. Structure projet recommandée

```text
api/
├── app/
│   ├── main.py
│   ├── config.py
│   ├── models/
│   │   ├── domain.py
│   │   ├── api.py
│   │   └── errors.py
│   ├── collectors/
│   │   ├── cpu.py
│   │   ├── memory.py
│   │   ├── gpu.py
│   │   ├── sensors.py
│   │   └── files.py
│   ├── normalizers/
│   │   ├── cpu.py
│   │   ├── gpu.py
│   │   └── common.py
│   ├── store/
│   │   ├── snapshot_store.py
│   │   ├── history_store.py
│   │   └── state.py
│   ├── services/
│   │   ├── sampler.py
│   │   ├── dashboard_service.py
│   │   ├── history_service.py
│   │   ├── meta_service.py
│   │   └── health_service.py
│   ├── api/
│   │   ├── routes_health.py
│   │   ├── routes_dashboard.py
│   │   ├── routes_history.py
│   │   └── routes_meta.py
│   └── diagnostics/
│       ├── logging.py
│       └── probe.py
├── tests/
├── requirements.txt
└── README.md
```

---

## 6. Contrat interne des métriques

## 6.1 Modèle interne de snapshot

Le backend doit manipuler un modèle interne unique avant sérialisation HTTP.

### Structure logique

- timestamp snapshot
- host
- cpu
- mem
- gpu
- état global

### Principes

- tous les champs attendus par l’API existent toujours en interne ;
- les métriques absentes valent `None` ;
- aucun endpoint ne reconstruit sa propre logique métier ;
- les endpoints ne lisent que le store déjà normalisé.

## 6.2 Modèle interne d’échantillon historique

Chaque tick d’échantillonnage doit produire un enregistrement minimal :

- `ts`
- `cpu_pct`
- `cpu_temp_c`
- `gpu_pct`
- `gpu_temp_c`

Optionnellement, pour diagnostic interne :

- `cpu_power_w`
- `gpu_power_w`

Mais seuls les champs utiles aux graphes V1 doivent être servis publiquement.

---

## 7. Mapping des données Linux

## 7.1 CPU usage

### Source
- `psutil.cpu_percent(interval=None)`

### Règle
- une phase d’initialisation doit amorcer la lecture ;
- ensuite la valeur est lue au tick d’échantillonnage ;
- la valeur servie est l’utilisation globale CPU.

## 7.2 CPU temperature

### Objectif
Trouver la meilleure température CPU exploitable.

### Stratégie
1. scanner les capteurs `hwmon` ;
2. identifier la source `k10temp` ;
3. chercher un label `Tdie` ;
4. si absent, fallback sur `Tctl` ;
5. si aucune sonde valide, retourner `None`.

### Sortie
- `cpu.temp_c` = float ou `None`

## 7.3 CPU power

### Objectif
Exposer la puissance CPU uniquement si la plateforme la fournit réellement.

### Stratégie
- implémenter une sonde dédiée ;
- si aucune source fiable n’est détectée, retourner `None` ;
- ne jamais synthétiser une pseudo-valeur.

### Sortie
- `cpu.power_w` = float ou `None`

## 7.4 Memory

### Source
- `psutil.virtual_memory()`

### Sorties
- `mem.used_b`
- `mem.total_b`
- `mem.pct`

## 7.5 GPU usage

### Objectif
Lire l’utilisation GPU AMD.

### Stratégie
1. repérer la bonne carte dans `/sys/class/drm/card*/device` ;
2. identifier le device AMD cible ;
3. lire `gpu_busy_percent` ;
4. convertir en float ;
5. si indisponible, retourner `None`.

### Sortie
- `gpu.pct` = float ou `None`

## 7.6 GPU temperature

### Objectif
Lire la température GPU AMD.

### Stratégie
- repérer le `hwmon` associé au GPU AMD ;
- lire la température principale exploitable ;
- convertir en degrés Celsius ;
- fallback `None` si absent.

### Sortie
- `gpu.temp_c` = float ou `None`

## 7.7 GPU power

### Objectif
Lire la puissance GPU AMD.

### Stratégie
- prioriser la télémétrie AMD disponible ;
- chercher la puissance moyenne si exposée ;
- convertir en watts ;
- fallback `None` si la métrique n’existe pas.

### Sortie
- `gpu.power_w` = float ou `None`

---

## 8. Politique de fallback capteurs

## 8.1 Règles générales

Chaque métrique doit suivre une politique stricte.

1. tenter les sources primaires ;
2. tenter les fallbacks ordonnés ;
3. si échec total, retourner `None` ;
4. enregistrer la cause en diagnostic ;
5. ne jamais supprimer le champ du JSON.

## 8.2 Règles interdites

Il est interdit de :

- fabriquer des valeurs estimées ;
- renommer dynamiquement les champs selon la machine ;
- exposer des unités variables ;
- faire dépendre l’API du nom exact d’un fichier non validé.

---

## 9. Boucle d’échantillonnage

## 9.1 Principe

Le backend ne doit pas recalculer lourdement à chaque requête HTTP.

Il doit exister une boucle d’échantillonnage interne qui :

- collecte les métriques ;
- normalise les valeurs ;
- met à jour le snapshot ;
- pousse un point dans l’historique.

## 9.2 Cadence

Cadence cible V1 :

- 1 tick par seconde

## 9.3 Séquence d’un tick

1. lire CPU % ;
2. lire CPU temp ;
3. lire CPU power ;
4. lire mémoire ;
5. lire GPU % ;
6. lire GPU temp ;
7. lire GPU power ;
8. construire le snapshot brut ;
9. normaliser ;
10. calculer l’état ;
11. enregistrer snapshot courant ;
12. pousser séries historiques ;
13. enregistrer diagnostics éventuels.

## 9.4 Gestion d’erreur pendant un tick

- une métrique isolée ne doit pas faire échouer tout le tick ;
- le tick produit un snapshot partiel valide ;
- seul un échec structurel complet peut invalider `state.ok`.

---

## 10. Store et historique

## 10.1 Snapshot store

Le store snapshot doit contenir :

- dernier snapshot valide ;
- horodatage de dernière mise à jour ;
- statut global ;
- état diagnostic de la dernière collecte.

## 10.2 History store

Le store historique doit être un ring buffer mémoire.

### Champs stockés
- `cpu_pct`
- `cpu_temp_c`
- `gpu_pct`
- `gpu_temp_c`
- `ts`

## 10.3 Taille recommandée

Pour une résolution à 1 seconde :

- historique minimal : 300 points
- historique conseillé : 600 points

## 10.4 Règles de service history

Le service `history` doit :

- lire le ring buffer ;
- filtrer selon `window` ;
- sous-échantillonner selon `step` ;
- garantir des séries de même longueur ;
- renvoyer uniquement des tableaux plats.

---

## 11. État global et obsolescence

## 11.1 Définition

L’état global sert à savoir si la donnée est exploitable.

## 11.2 Champs minimaux

- `state.ok`
- `state.stale_ms`

## 11.3 Règles

### `state.ok = true`
- si le snapshot courant a été produit correctement ;
- même si certaines métriques optionnelles sont nulles.

### `state.ok = false`
- si aucun snapshot cohérent n’a jamais pu être produit ;
- ou si le service n’a plus de données exploitables.

### `state.stale_ms`
- temps écoulé depuis le dernier tick valide par rapport à l’instant de réponse ;
- toujours fourni.

---

## 12. Contrat API à implémenter

## 12.1 Endpoint `GET /api/v1/health`

### But
- diagnostic minimal du service.

### Réponse

```json
{
  "v": 1,
  "ts": 1774256402,
  "ok": true,
  "service": "stats-linux-api"
}
```

### Implémentation
- répondre sans dépendance forte à toutes les métriques ;
- refléter l’état global du service.

## 12.2 Endpoint `GET /api/v1/dashboard`

### But
- fournir le snapshot instantané complet.

### Réponse cible

```json
{
  "v": 1,
  "ts": 1774256402,
  "host": "linux-main",
  "cpu": {
    "pct": 12.4,
    "temp_c": 43.8,
    "power_w": null
  },
  "mem": {
    "used_b": 9123454976,
    "total_b": 34359738368,
    "pct": 26.6
  },
  "gpu": {
    "pct": 7.0,
    "temp_c": 39.0,
    "power_w": 36.4
  },
  "state": {
    "ok": true,
    "stale_ms": 0
  }
}
```

### Implémentation
- lire uniquement le snapshot store ;
- ne faire aucune lecture système synchrone dans le handler ;
- sérialiser un JSON stable.

## 12.3 Endpoint `GET /api/v1/history`

### But
- fournir les séries nécessaires aux graphes.

### Paramètres
- `window`
- `step`

### Réponse cible

```json
{
  "v": 1,
  "ts": 1774256402,
  "window_s": 300,
  "step_s": 1,
  "series": {
    "cpu_pct": [8.0, 9.4, 11.2, 10.1],
    "cpu_temp_c": [41.0, 41.1, 41.3, 41.4],
    "gpu_pct": [2.0, 4.0, 7.0, 3.0],
    "gpu_temp_c": [38.0, 38.0, 39.0, 39.0]
  }
}
```

### Implémentation
- lire le history store ;
- contrôler les bornes des paramètres ;
- garantir l’alignement des séries.

## 12.4 Endpoint `GET /api/v1/meta`

### But
- décrire le contrat exposé.

### Réponse cible

```json
{
  "v": 1,
  "host": "linux-main",
  "metrics": [
    "cpu.pct",
    "cpu.temp_c",
    "cpu.power_w",
    "mem.used_b",
    "mem.total_b",
    "mem.pct",
    "gpu.pct",
    "gpu.temp_c",
    "gpu.power_w"
  ],
  "history_series": [
    "cpu_pct",
    "cpu_temp_c",
    "gpu_pct",
    "gpu_temp_c"
  ]
}
```

### Implémentation
- réponse statique ou semi-statique ;
- pas de logique coûteuse.

---

## 13. Validation des paramètres

## 13.1 Paramètre `window`

### Règles
- type entier
- min recommandé : 10
- max recommandé : 600
- défaut recommandé : 300

## 13.2 Paramètre `step`

### Règles
- type entier
- min : 1
- max recommandé : 10
- défaut recommandé : 1

## 13.3 Politique d’erreur

En cas de paramètre invalide :

- code HTTP `400`
- payload d’erreur court et déterministe.

Exemple :

```json
{
  "v": 1,
  "error": "invalid_parameter",
  "field": "window"
}
```

---

## 14. Politique de sérialisation JSON

## 14.1 Règles imposées

- clés ordonnées logiquement ;
- champs toujours présents ;
- nombres sous forme numérique, jamais en chaîne ;
- `null` pour les valeurs absentes ;
- pas de métadonnées inutiles ;
- pas de noms de clés variables selon la plateforme.

## 14.2 Règles de précision

Recommandation :

- `%` : 1 décimale max ;
- `temp_c` : 1 décimale max ;
- `power_w` : 1 décimale max ;
- bytes : entiers.

Objectif :
- compacité ;
- stabilité ;
- affichage direct côté ESP32.

---

## 15. Authentification optionnelle

## 15.1 V1 par défaut

- aucune authentification obligatoire.

## 15.2 Option prévue

- header statique type `X-API-Key`

## 15.3 Implémentation

- middleware ou dépendance FastAPI légère ;
- activable par configuration ;
- aucun impact sur le contrat JSON métier.

---

## 16. Logging et diagnostics

## 16.1 Ce qu’il faut journaliser

- démarrage service ;
- configuration effective ;
- détection capteurs ;
- source retenue pour chaque métrique ;
- erreurs de lecture capteur ;
- erreurs HTTP ;
- invalidité de paramètres.

## 16.2 Ce qu’il ne faut pas faire

- loguer chaque tick de manière verbeuse en production ;
- noyer les erreurs utiles dans du bruit.

## 16.3 Mode diagnostic recommandé

Prévoir un mode debug permettant d’afficher :

- chemins capteurs détectés ;
- fallback choisi ;
- valeurs lues avant normalisation ;
- raisons d’absence des métriques nulles.

---

## 17. Tests à implémenter

## 17.1 Tests unitaires

### Collecteurs
- lecture CPU usage
- détection `k10temp`
- fallback `Tdie` → `Tctl`
- lecture mémoire
- lecture `gpu_busy_percent`
- lecture temp GPU
- lecture puissance GPU
- comportement si fichier absent

### Normalisation
- conversion m°C → °C
- conversion µW → W ou mW → W selon source
- clamp éventuel des pourcentages
- gestion des `None`

### Store
- ajout snapshot
- push ring buffer
- rotation buffer
- calcul `stale_ms`

## 17.2 Tests d’intégration

- `GET /health`
- `GET /dashboard`
- `GET /history`
- `GET /meta`
- paramètres invalides
- métriques nulles mais JSON valide
- cohérence longueur des séries

## 17.3 Tests de non-régression contrat

Créer des tests figés sur le JSON exact.

Objectif :
- empêcher toute dérive du contrat qui casserait l’ESP32.

---

## 18. Ordre de mise en œuvre recommandé

## Phase 1 — Squelette projet

1. créer la structure FastAPI ;
2. créer les modèles Pydantic ;
3. créer les endpoints vides ;
4. créer la configuration ;
5. définir les réponses JSON cibles.

## Phase 2 — Collecte minimale utile

1. implémenter CPU % ;
2. implémenter mémoire ;
3. implémenter CPU temp ;
4. implémenter GPU % ;
5. implémenter GPU temp ;
6. implémenter GPU power.

À ce stade, `cpu.power_w` peut rester `None`.

## Phase 3 — Snapshot et historique

1. implémenter boucle sampler 1 Hz ;
2. implémenter snapshot store ;
3. implémenter history store ;
4. exposer `dashboard` ;
5. exposer `history`.

## Phase 4 — Validation robuste

1. validations paramètres ;
2. gestion erreurs ;
3. `state.ok` ;
4. `state.stale_ms` ;
5. diagnostics.

## Phase 5 — Sécurisation légère

1. bind configurable ;
2. clé API optionnelle ;
3. logs propres ;
4. durcissement de la sérialisation.

## Phase 6 — Finalisation contrat

1. tests de non-régression ;
2. documentation finale ;
3. figer la V1 ;
4. transmettre au développement ESP32.

---

## 19. Critères de fin pour la brique API

L’API peut être considérée prête pour intégration ESP32 si :

1. `dashboard` retourne en continu un JSON strictement stable ;
2. `history` retourne des séries cohérentes et bornées ;
3. les métriques absentes sont correctement nullables ;
4. les métriques CPU, mémoire et GPU principales sont lues correctement ;
5. les handlers HTTP ne lisent pas directement le système ;
6. le backend tient le rythme 1 Hz sans dérive ;
7. les erreurs de lecture n’effondrent pas le service ;
8. des tests figent le contrat JSON.

---

## 20. Décision d’implémentation prioritaire

La priorité absolue est la suivante :

1. fiabiliser le mapping capteurs Linux ;
2. fiabiliser le modèle interne ;
3. fiabiliser le JSON `dashboard` ;
4. fiabiliser le JSON `history`.

Le reste est secondaire tant que ces quatre points ne sont pas verrouillés.

C’est ce noyau qui conditionne directement la simplicité du firmware ESP32 et la réussite globale du système.

---

## 21. Plan d’exécution final (opérationnel)

Ce plan transforme les sections précédentes en séquence de livraison directement exécutable.

### 21.1 Hypothèse de départ (constat dépôt)

- le dépôt courant contient la documentation de cadrage ;
- le squelette backend n’est pas encore initialisé ;
- la première implémentation doit donc créer la base `api/` puis livrer les endpoints V1.

### 21.2 Lots d’implémentation (ordre imposé)

#### Lot 0 — Bootstrap technique

Objectif :
- créer le projet Python/FastAPI testable localement.

Actions :
- créer l’arborescence `api/app/...` conforme à la section 5 ;
- initialiser dépendances minimales (`fastapi`, `uvicorn`, `pydantic`, `psutil`, `pytest`, `httpx`) ;
- créer un point d’entrée `app.main:app` ;
- ajouter un test de smoke API.

Critère de sortie :
- service démarre ;
- un test HTTP simple passe.

#### Lot 1 — Modèles et contrat JSON figé

Objectif :
- figer les schémas Pydantic V1 avant la logique capteur.

Actions :
- créer modèles `health`, `dashboard`, `history`, `meta`, `error` ;
- imposer champs obligatoires et nullabilité attendue ;
- ajouter snapshots JSON de référence pour non-régression.

Critère de sortie :
- sérialisation stable ;
- tests de schéma passent.

#### Lot 2 — Collecte CPU/RAM minimale

Objectif :
- livrer rapidement un `dashboard` partiel mais contractuel.

Actions :
- implémenter `cpu.pct` via `psutil.cpu_percent(interval=None)` (avec amorçage) ;
- implémenter `mem.used_b`, `mem.total_b`, `mem.pct` via `psutil.virtual_memory()` ;
- implémenter CPU temp avec fallback `Tdie -> Tctl` ;
- laisser `cpu.power_w`, `gpu.*` à `None` sans casser le contrat.

Critère de sortie :
- `GET /api/v1/dashboard` retourne tous les champs V1 ;
- métriques absentes présentes en `null`.

#### Lot 3 — Collecte GPU AMD

Objectif :
- compléter le contrat GPU V1.

Actions :
- détecter la carte AMD cible (`/sys/class/drm/card*/device`) ;
- lire `gpu_busy_percent` ;
- lire température GPU (`hwmon`) ;
- lire puissance GPU si exposée ;
- journaliser clairement source et fallback.

Critère de sortie :
- `gpu.pct`, `gpu.temp_c`, `gpu.power_w` alimentés quand disponibles ;
- fallback `None` propre sinon.

#### Lot 4 — Store, sampler et obsolescence

Objectif :
- découpler collecte et HTTP, stabiliser le rythme 1 Hz.

Actions :
- implémenter boucle sampler interne 1 Hz ;
- implémenter snapshot store ;
- implémenter ring buffer historique (300-600 points) ;
- calculer `state.ok` et `state.stale_ms`.

Critère de sortie :
- handlers HTTP sans lecture système directe ;
- snapshot et historique mis à jour automatiquement.

#### Lot 5 — Endpoints V1 complets

Objectif :
- exposer les quatre endpoints contractuels.

Actions :
- implémenter `/api/v1/health` ;
- implémenter `/api/v1/dashboard` ;
- implémenter `/api/v1/history` avec `window`/`step` bornés ;
- implémenter `/api/v1/meta` ;
- implémenter erreurs `400` déterministes sur paramètres invalides.

Critère de sortie :
- conformité fonctionnelle complète V1.

#### Lot 6 — Durcissement et diagnostics

Objectif :
- rendre le service exploitable en continu.

Actions :
- logs de démarrage/config/capteurs/fallbacks/erreurs ;
- mode diagnostic activable ;
- option `X-API-Key` sans impact contrat métier ;
- vérification compacité/précision JSON.

Critère de sortie :
- logs utiles sans bruit excessif ;
- auth optionnelle opérationnelle.

#### Lot 7 — Validation finale et handoff ESP32

Objectif :
- figer la V1 pour consommation firmware.

Actions :
- exécuter tests unitaires ciblés (collectors/normalizers/store) ;
- exécuter tests d’intégration endpoints ;
- exécuter tests de non-régression JSON ;
- finaliser docs contrat + configuration.

Critère de sortie :
- contrat stable ;
- dossier prêt pour intégration ESP32-S3.

### 21.3 Définition de done globale

La brique API est terminée quand les points suivants sont vrais simultanément :

1. `GET /api/v1/dashboard` est stable et complet à chaque tick.
2. `GET /api/v1/history` renvoie des séries alignées, bornées, cohérentes.
3. les champs absents restent présents avec `null`.
4. `state.ok` et `state.stale_ms` reflètent l’état réel.
5. aucune lecture capteur n’est faite dans les handlers HTTP.
6. les tests de contrat empêchent toute dérive JSON.

### 21.4 Stratégie de commits recommandée

- 1 commit par lot ;
- scope Conventional Commit aligné module (`api`, `collectors`, `store`, `services`, `tests`, `docs`) ;
- chaque commit doit rester testable isolément ;
- mise à jour doc dans le même commit si impact contrat.
