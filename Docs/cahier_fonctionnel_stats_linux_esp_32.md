# Cahier fonctionnel — Supervision Linux temps réel sur ESP32-S3

## 1. Objet

Ce document définit le fonctionnement détaillé du système d’affichage temps réel des statistiques d’une machine Linux sur un ESP32-S3 via Wi‑Fi.

Il précise :

- l’architecture retenue ;
- le périmètre fonctionnel de l’API ;
- les limites de l’API ;
- le contrat JSON détaillé ;
- les responsabilités respectives du backend Linux et du firmware ESP32-S3 ;
- les règles de comportement, d’erreur et de rafraîchissement.

---

## 2. Périmètre fonctionnel retenu

Le système doit permettre :

- la collecte locale des métriques système sur une machine Linux ;
- l’exposition de ces métriques via une API HTTP locale ;
- la récupération périodique des données par un ESP32-S3 connecté au réseau local ;
- l’affichage de valeurs instantanées ;
- l’affichage de graphes courts pour CPU et GPU ;
- un fonctionnement autonome sans service cloud ni broker externe.

Le système est prévu pour un usage personnel sur réseau local.

---

## 3. Architecture fonctionnelle retenue

## 3.1 Vue d’ensemble

Architecture validée :

- **Machine Linux** : collecteur Python + API HTTP locale
- **ESP32-S3** : client HTTP natif ESP-IDF + interface LVGL
- **Transport** : JSON compact versionné + polling simple

## 3.2 Délimitation stricte de l’architecture

### Backend Linux

Le backend Linux est responsable de :

- collecter les métriques ;
- appliquer les règles de fallback capteurs ;
- maintenir un snapshot courant ;
- maintenir un historique court en mémoire ;
- exposer les données via HTTP ;
- fournir un contrat JSON stable ;
- signaler l’état de validité des données.

Le backend Linux n’est pas responsable de :

- dessiner une interface utilisateur riche ;
- gérer plusieurs profils d’affichage ;
- historiser en base de données ;
- faire de l’alerting complexe ;
- publier en MQTT en V1.

### ESP32-S3

L’ESP32-S3 est responsable de :

- se connecter au Wi‑Fi ;
- localiser le backend ;
- interroger périodiquement l’API ;
- parser les réponses JSON ;
- stocker localement les dernières valeurs utiles à l’affichage ;
- afficher les métriques et graphes via LVGL ;
- indiquer l’état réseau et l’obsolescence des données.

L’ESP32-S3 n’est pas responsable de :

- calculer les métriques système ;
- reconstruire des historiques à partir de données brutes longues ;
- interpréter des formats variables ;
- découvrir dynamiquement des structures JSON non versionnées.

## 3.3 Choix techniques imposés

### Côté Linux

- langage : **Python** ;
- exposition API : **HTTP** ;
- framework recommandé : **FastAPI** ;
- métriques système génériques : **psutil** ;
- métriques AMD spécifiques : lecture **sysfs / hwmon / DRM**.

### Côté ESP32

- environnement : **ESP-IDF natif** ;
- client réseau : **HTTP** ;
- interface graphique : **LVGL** ;
- traitement : séparation stricte entre réseau, parsing, stockage local et rendu UI.

## 3.4 Choix exclus de la V1

Sont exclus de la V1 :

- MQTT ;
- interface Qt ;
- authentification forte ;
- TLS obligatoire ;
- multi-machines ;
- historique persistant ;
- dashboard web complet ;
- push temps réel côté client.

---

## 4. Métriques fonctionnelles retenues

## 4.1 Métriques instantanées

### CPU

- utilisation totale CPU en pourcentage ;
- température CPU en degrés Celsius ;
- puissance CPU en watts si disponible.

### Mémoire

- mémoire utilisée en octets ;
- mémoire totale en octets ;
- mémoire utilisée en pourcentage.

### GPU

- utilisation totale GPU en pourcentage ;
- température GPU en degrés Celsius ;
- puissance GPU en watts.

## 4.2 Séries historiques pour graphes

Les séries historiques retenues en V1 sont :

- `cpu_pct` ;
- `cpu_temp_c` ;
- `gpu_pct` ;
- `gpu_temp_c`.

Aucun historique RAM n’est requis en V1.

---

## 5. Sources fonctionnelles des données Linux

## 5.1 CPU

### Utilisation CPU

Source fonctionnelle : calcul logiciel côté Python.

Implémentation attendue :

- utilisation totale globale du CPU ;
- pas de détail par cœur dans la V1 API publique.

### Température CPU

Source fonctionnelle : capteurs AMD CPU.

Règle de lecture :

1. utiliser `Tdie` si disponible ;
2. sinon utiliser `Tctl`.

### Puissance CPU

Source fonctionnelle : télémétrie disponible si exposée par la plateforme.

Règle fonctionnelle :

- la valeur est facultative ;
- si non disponible, le champ doit exister et valoir `null`.

## 5.2 Mémoire

Source fonctionnelle : métriques mémoire système globales.

Le backend doit exposer :

- mémoire utilisée ;
- mémoire totale ;
- pourcentage utilisé.

## 5.3 GPU

### Utilisation GPU

Source fonctionnelle : télémétrie `amdgpu`.

### Température GPU

Source fonctionnelle : télémétrie `amdgpu`.

### Puissance GPU

Source fonctionnelle : télémétrie `amdgpu`.

Règle fonctionnelle :

- sur GPU dédié, cette valeur est considérée attendue ;
- si absente localement, le backend devra soit exposer `null`, soit marquer explicitement l’indisponibilité dans l’état global.

---

## 6. Comportement global du système

## 6.1 Cycle nominal

1. le backend Linux collecte et met à jour les métriques ;
2. le backend maintient un snapshot courant en mémoire ;
3. le backend alimente un historique circulaire pour les graphes ;
4. l’ESP32 interroge périodiquement l’API ;
5. l’ESP32 parse les réponses ;
6. l’ESP32 met à jour son cache local ;
7. l’ESP32 met à jour l’écran.

## 6.2 Règle de découplage

La logique de rendu écran ne doit jamais dépendre directement d’une requête HTTP en cours.

Le rendu utilise uniquement :

- le dernier snapshot valide ;
- les derniers buffers graphiques valides ;
- l’état de fraîcheur des données.

## 6.3 Règle d’obsolescence

Toute donnée affichée doit être considérée comme :

- **fraîche** si issue d’un snapshot récent ;
- **obsolète** si la mise à jour n’a pas eu lieu dans la fenêtre prévue.

Le backend doit fournir une information d’obsolescence.
L’ESP32 doit l’afficher.

---

## 7. API fonctionnelle

## 7.1 Style d’API

L’API doit être :

- HTTP locale ;
- versionnée ;
- orientée lecture seule ;
- stable ;
- optimisée pour un client embarqué ;
- sans logique métier côté client.

## 7.2 Versionnement

La version d’API utilisée en V1 est :

- préfixe URL : `/api/v1/`
- champ JSON : `"v": 1`

Toute rupture de contrat impose :

- soit un nouveau préfixe ;
- soit une augmentation explicite de version.

## 7.3 Endpoints V1

### `GET /api/v1/health`

#### Rôle

Donner l’état minimal de disponibilité du service.

#### Réponse attendue

- version API ;
- timestamp serveur ;
- état global.

#### Usage côté ESP32

- diagnostic ;
- écran de statut ;
- test de connectivité.

### `GET /api/v1/dashboard`

#### Rôle

Retourner toutes les valeurs instantanées nécessaires à l’affichage principal.

#### Réponse attendue

- snapshot unique ;
- toutes les métriques V1 ;
- état global ;
- host ;
- timestamp.

#### Usage côté ESP32

- écran principal ;
- affichage instantané ;
- barres et widgets de synthèse.

### `GET /api/v1/history`

#### Rôle

Retourner les séries nécessaires aux graphes.

#### Paramètres attendus

- `window` : taille de la fenêtre historique en secondes ;
- `step` : pas d’échantillonnage en secondes.

#### Réponse attendue

- timestamp ;
- fenêtre ;
- pas ;
- séries alignées.

#### Usage côté ESP32

- écrans graphes ;
- mise à jour périodique moins fréquente.

### `GET /api/v1/meta`

#### Rôle

Retourner les informations descriptives et la liste des champs exposés.

#### Réponse attendue

- host ;
- unités ;
- liste des métriques ;
- capacités disponibles.

#### Usage côté ESP32

- diagnostic ;
- affichage de contexte ;
- validation du contrat.

---

## 8. Limites fonctionnelles de l’API

## 8.1 Limites de périmètre

L’API V1 ne doit pas :

- exposer des métriques par cœur ;
- exposer des métriques détaillées de ventilateurs ;
- exposer des tensions ;
- exposer des données de disque ;
- exposer des données réseau détaillées ;
- exposer des données de processus ;
- exposer des réglages machine ;
- permettre des écritures ou commandes ;
- piloter la machine Linux.

## 8.2 Limites de volumétrie

L’API doit rester légère et bornée.

Contraintes fonctionnelles recommandées :

- un snapshot `dashboard` doit rester compact ;
- la réponse `history` doit rester compatible avec un parsing embarqué ;
- les séries doivent être bornées à une fenêtre courte.

Règles de bornage recommandées :

- `window` maximum : **600 secondes** ;
- `step` minimum : **1 seconde** ;
- `step` maximum utile : **10 secondes**.

Si un paramètre dépasse les bornes, l’API doit :

- soit le normaliser ;
- soit retourner une erreur fonctionnelle explicite.

## 8.3 Limites de fréquence

Le backend n’a pas vocation à servir un très grand nombre de clients.

Hypothèse V1 :

- un client principal ESP32 ;
- éventuellement un client de diagnostic ponctuel.

Cadences cibles :

- `dashboard` : 1 Hz ;
- `history` : toutes les 5 à 10 secondes.

## 8.4 Limites de disponibilité des métriques

Certaines métriques peuvent ne pas être disponibles de façon universelle.

Règles :

- `cpu.power_w` est optionnelle ;
- si une métrique est indisponible, elle ne doit pas casser le contrat ;
- la réponse doit rester valide ;
- les champs doivent être présents avec `null` si nécessaire.

## 8.5 Limites de sécurité

L’API est prévue pour un LAN privé.

En V1 :

- authentification absente par défaut ;
- clé API statique possible en option ;
- pas d’exposition publique Internet ;
- pas de sécurité multi-utilisateurs.

---

## 9. Contrat JSON détaillé

## 9.1 Principes obligatoires

Le JSON doit respecter les règles suivantes :

- structure stable ;
- noms de champs fixes ;
- unités fixes ;
- valeurs numériques directement exploitables ;
- compatibilité stricte avec parsing embarqué ;
- absence de champs verbeux inutiles.

## 9.2 Types autorisés

- entier
- réel
- booléen
- chaîne courte
- `null`
- tableau plat
- objet plat ou faiblement imbriqué

## 9.3 Règles de nommage

- noms courts ;
- minuscules ;
- séparateur `_` ;
- suffixe explicite pour les unités.

Exemples :

- `_pct`
- `_c`
- `_w`
- `_b`
- `_ms`
- `_s`

## 9.4 Contrat de `GET /api/v1/health`

### Réponse attendue

```json
{
  "v": 1,
  "ts": 1774256402,
  "ok": true,
  "service": "stats-linux-api"
}
```

### Champs

- `v` : entier, version du contrat
- `ts` : entier, timestamp epoch Unix
- `ok` : booléen, état global du service
- `service` : chaîne courte, identifiant logique du service

## 9.5 Contrat de `GET /api/v1/dashboard`

### Réponse attendue

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

### Détail complet des champs

#### Racine

- `v`
  - type : entier
  - obligatoire : oui
  - sens : version du contrat JSON
  - valeur V1 : `1`

- `ts`
  - type : entier
  - obligatoire : oui
  - unité : secondes Unix epoch
  - sens : timestamp de génération du snapshot

- `host`
  - type : chaîne courte
  - obligatoire : oui
  - sens : nom logique ou hostname de la machine Linux

#### Objet `cpu`

- `cpu.pct`
  - type : réel
  - obligatoire : oui
  - unité : pourcentage
  - plage attendue : `0.0` à `100.0`
  - sens : utilisation CPU globale

- `cpu.temp_c`
  - type : réel ou `null`
  - obligatoire : oui
  - unité : degrés Celsius
  - sens : température CPU retenue selon fallback capteurs

- `cpu.power_w`
  - type : réel ou `null`
  - obligatoire : oui
  - unité : watts
  - sens : puissance CPU si disponible

#### Objet `mem`

- `mem.used_b`
  - type : entier
  - obligatoire : oui
  - unité : octets
  - sens : mémoire utilisée

- `mem.total_b`
  - type : entier
  - obligatoire : oui
  - unité : octets
  - sens : mémoire totale

- `mem.pct`
  - type : réel
  - obligatoire : oui
  - unité : pourcentage
  - plage attendue : `0.0` à `100.0`
  - sens : pourcentage de mémoire utilisée

#### Objet `gpu`

- `gpu.pct`
  - type : réel or `null`
  - obligatoire : oui
  - unité : pourcentage
  - plage attendue : `0.0` à `100.0`
  - sens : utilisation GPU globale

- `gpu.temp_c`
  - type : réel ou `null`
  - obligatoire : oui
  - unité : degrés Celsius
  - sens : température GPU

- `gpu.power_w`
  - type : réel ou `null`
  - obligatoire : oui
  - unité : watts
  - sens : puissance GPU

#### Objet `state`

- `state.ok`
  - type : booléen
  - obligatoire : oui
  - sens : état logique du snapshot

- `state.stale_ms`
  - type : entier
  - obligatoire : oui
  - unité : millisecondes
  - sens : âge ou obsolescence de la donnée par rapport au rythme prévu

## 9.6 Contrat de `GET /api/v1/history`

### Requête

Exemple :

`/api/v1/history?window=300&step=1`

### Réponse attendue

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

### Détail complet des champs

- `v`
  - type : entier
  - obligatoire : oui
  - sens : version du contrat

- `ts`
  - type : entier
  - obligatoire : oui
  - unité : secondes Unix epoch
  - sens : timestamp de génération de la réponse

- `window_s`
  - type : entier
  - obligatoire : oui
  - unité : secondes
  - sens : profondeur historique réellement servie

- `step_s`
  - type : entier
  - obligatoire : oui
  - unité : secondes
  - sens : pas réel entre les points

- `series`
  - type : objet
  - obligatoire : oui
  - sens : conteneur des séries graphiques

#### Séries obligatoires V1

- `series.cpu_pct`
  - type : tableau de réels
  - unité : pourcentage
  - sens : historique utilisation CPU

- `series.cpu_temp_c`
  - type : tableau de réels
  - unité : degrés Celsius
  - sens : historique température CPU

- `series.gpu_pct`
  - type : tableau de réels
  - unité : pourcentage
  - sens : historique utilisation GPU

- `series.gpu_temp_c`
  - type : tableau de réels
  - unité : degrés Celsius
  - sens : historique température GPU

### Contraintes de cohérence

- toutes les séries doivent avoir la même longueur ;
- l’ordre des points doit être chronologique ;
- les séries doivent correspondre à `window_s` et `step_s` ;
- l’ESP32 ne doit pas avoir à réaligner les tableaux.

## 9.7 Contrat de `GET /api/v1/meta`

### Réponse attendue

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

### Détail des champs

- `v` : entier, version du contrat
- `host` : chaîne, machine source
- `metrics` : tableau de chaînes, métriques instantanées garanties par le contrat
- `history_series` : tableau de chaînes, séries historiques disponibles

---

## 10. Codes de comportement attendus

## 10.1 Réussite

- `200 OK` sur lecture réussie.

## 10.2 Erreurs fonctionnelles minimales

### Paramètres invalides

Exemples :

- `window` hors bornes ;
- `step` invalide.

Comportement attendu :

- `400 Bad Request`
- message JSON explicite et court.

### Service indisponible

Comportement attendu :

- `503 Service Unavailable`
- état explicite si le backend ne peut produire un snapshot valide.

### Ressource absente

Comportement attendu :

- `404 Not Found` si endpoint inconnu.

---

## 11. Rythmes fonctionnels de rafraîchissement

## 11.1 Backend Linux

Le backend doit idéalement échantillonner à un rythme au moins équivalent au besoin d’affichage.

Recommandation V1 :

- échantillonnage interne à 1 seconde.

## 11.2 ESP32

Rythmes fonctionnels retenus :

- `dashboard` : 1 appel par seconde ;
- `history` : 1 appel toutes les 5 à 10 secondes ;
- `health` : à l’initialisation, au diagnostic, ou lors d’erreur ;
- `meta` : à l’initialisation ou au diagnostic.

---

## 12. Comportement de l’ESP32-S3

## 12.1 Organisation logique minimale

Le firmware doit être organisé en quatre blocs :

1. réseau ;
2. client HTTP ;
3. parsing + cache local ;
4. interface LVGL.

## 12.2 Cache local obligatoire

L’ESP32 doit maintenir :

- un dernier snapshot valide ;
- un dernier historique valide ;
- un état réseau ;
- un état de fraîcheur des données.

## 12.3 Comportement en cas d’échec réseau

En cas d’échec :

- conserver les dernières données valides ;
- indiquer l’obsolescence ;
- ne pas vider l’écran ;
- retenter selon le cycle nominal.

## 12.4 Comportement si JSON invalide

En cas de JSON invalide ou incomplet :

- rejeter la trame ;
- conserver la dernière trame valide ;
- incrémenter un état d’erreur interne ;
- ne pas tenter d’interprétation partielle arbitraire.

---

## 13. Écrans fonctionnels ESP32

## 13.1 Écran principal

Doit afficher :

- CPU % ;
- CPU °C ;
- CPU W si disponible ;
- RAM utilisée / totale / % ;
- GPU % ;
- GPU °C ;
- GPU W ;
- état réseau ;
- indication de fraîcheur des données.

## 13.2 Écran graphes

Doit afficher les graphes :

- CPU % ;
- CPU °C ;
- GPU % ;
- GPU °C.

## 13.3 Écran statut

Doit afficher au minimum :

- état Wi‑Fi ;
- état API ;
- host ;
- version API ;
- timestamp dernière mise à jour ;
- état stale.

---

## 14. Sécurité fonctionnelle

## 14.1 Hypothèse V1

Le système fonctionne sur réseau local privé.

## 14.2 Position retenue

- pas d’authentification obligatoire en V1 ;
- possibilité d’ajouter un header de clé API ;
- aucune exposition Internet directe prévue.

## 14.3 Conséquences côté ESP32

L’ESP32 doit être capable :

- d’appeler l’API sans auth ;
- ou d’ajouter un header statique si l’API est protégée.

---

## 15. Paramètres configurables à prévoir

## 15.1 Côté backend

Doivent être configurables :

- interface ou IP de bind ;
- port HTTP ;
- activation ou non d’une clé API ;
- valeur de la clé API ;
- fréquence d’échantillonnage ;
- taille du ring buffer historique ;
- nom logique du host servi.

## 15.2 Côté ESP32

Doivent être configurables :

- SSID ;
- mot de passe Wi‑Fi ;
- adresse IP, hostname ou mDNS du backend ;
- port ;
- fréquence de polling ;
- présence éventuelle d’une clé API ;
- timeout réseau.

---

## 16. Critères d’acceptation fonctionnels

Le système sera considéré conforme si :

1. l’ESP32 affiche les métriques CPU, RAM et GPU prévues ;
2. les valeurs sont mises à jour à un rythme conforme ;
3. les graphes CPU/GPU sont fonctionnels ;
4. l’absence éventuelle de `cpu.power_w` ne casse pas l’affichage ;
5. l’API retourne un JSON conforme et stable ;
6. l’ESP32 reste fonctionnel en cas de perte temporaire du backend ;
7. l’écran signale correctement les données obsolètes ;
8. les séries historiques sont cohérentes et exploitables sans retraitement complexe.

---

## 17. Décisions fonctionnelles figées

Les décisions retenues à ce stade sont :

- architecture HTTP locale simple ;
- backend Python ;
- firmware natif ESP-IDF ;
- interface LVGL ;
- JSON compact versionné ;
- CPU/RAM/GPU comme périmètre V1 ;
- graphes CPU/GPU uniquement ;
- `cpu.power_w` nullable ;
- `gpu.power_w` prévue dans le contrat ;
- aucun MQTT en V1 ;
- aucune UI desktop en V1.

---

## 18. Points techniques nécessaires déjà déterminés

Les points suivants sont considérés comme acquis pour la suite du projet :

- le backend doit servir un **snapshot** et un **historique** séparés ;
- le JSON doit rester minimaliste et borné ;
- le client ESP32 ne doit jamais dériver la logique métier ;
- les séries historiques doivent être alignées et homogènes ;
- les valeurs non disponibles doivent être **nullables**, pas supprimées ;
- l’obsolescence de la donnée doit être exposée et affichée ;
- le rendu UI doit être découplé du réseau ;
- le projet doit rester exploitable même si certaines métriques matérielles varient selon la plateforme.

---

## 19. Suite logique du projet

À partir de ce cahier fonctionnel, la suite consiste à produire :

1. la spécification technique détaillée de l’API ;
2. le mapping Linux exact des chemins de lecture ;
3. le schéma logiciel ESP-IDF ;
4. la maquette des écrans LVGL ;
5. un prototype backend Python ;
6. un prototype de client HTTP + parsing sur ESP32-S3.

