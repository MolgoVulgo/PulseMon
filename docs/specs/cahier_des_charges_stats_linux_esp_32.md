# Cahier des charges — Affichage temps réel des statistiques Linux sur ESP32-S3

## 1. Objet du projet

Développer un système permettant d’afficher en temps réel les statistiques principales d’une machine Linux sur un module ESP32-S3 connecté en Wi‑Fi.

Le système repose sur trois briques :

- une machine Linux qui collecte les métriques système ;
- une API locale exposant ces données sur le réseau ;
- un ESP32-S3 qui consomme ces données et les affiche sur une interface graphique embarquée.

L’objectif est de disposer d’un affichage dédié, léger, autonome, lisible et réactif, orienté supervision locale de la machine.

---

## 2. Contexte matériel cible

### 2.1 Machine Linux source

Configuration cible identifiée :

- CPU : AMD Ryzen 9 5950X
- RAM : 32 Gio
- GPU : AMD Radeon RX 9070 XT dédiée
- Carte mère : Gigabyte B550 AORUS ELITE V2

### 2.2 Afficheur embarqué

Module cible :

- ESP32-S3 (JC3248W535EN)
- 16 Mo flash
- 8 Mo PSRAM
- connectivité Wi‑Fi
- développement natif via ESP-IDF
- interface graphique basée sur LVGL

---

## 3. Objectifs fonctionnels

Le système doit permettre :

- la collecte locale des métriques système Linux utiles ;
- l’exposition de ces métriques via une API HTTP locale ;
- la récupération périodique des données par l’ESP32-S3 ;
- l’affichage des valeurs instantanées sur écran ;
- l’affichage de graphes d’évolution pour CPU et GPU ;
- le fonctionnement sur réseau local sans dépendance cloud.

---

## 4. Architecture retenue

### 4.1 Principe général

Architecture validée :

- Linux : collecteur Python + API HTTP locale
- ESP32-S3 : client HTTP natif en ESP-IDF
- Transport : JSON compact, polling simple

### 4.2 Motivations

Cette architecture est retenue pour les raisons suivantes :

- simplicité de mise en œuvre ;
- faible coût de maintenance ;
- débogage plus simple qu’une architecture MQTT ;
- absence de broker supplémentaire ;
- découplage propre entre collecte, exposition et affichage ;
- adaptation directe à un usage personnel sur réseau local.

### 4.3 Variante écartée en V1

MQTT n’est pas retenu pour la première version.

Motifs :

- complexité supplémentaire inutile pour un unique client ESP32 ;
- nécessité d’un broker ;
- surcoût de conception des topics et de la logique publish/subscribe ;
- bénéfice limité pour un usage local et un écran unique.

MQTT pourra être réévalué ultérieurement si plusieurs clients, événements push ou intégration domotique deviennent nécessaires.

---

## 5. Périmètre fonctionnel V1

### 5.1 Métriques retenues

Les métriques retenues pour la version 1 sont les suivantes.

#### CPU

- charge CPU totale en pourcentage ;
- température CPU ;
- puissance CPU en watts si disponible.

#### Mémoire

- mémoire utilisée en octets ;
- mémoire totale en octets ;
- mémoire utilisée en pourcentage.

#### GPU

- charge GPU totale en pourcentage ;
- température GPU ;
- puissance GPU en watts.

### 5.2 Historisation pour affichage graphique

Les graphes affichés sur l’ESP32-S3 porteront sur :

- utilisation CPU ;
- température CPU ;
- utilisation GPU ;
- température GPU.

La mémoire ne fera pas l’objet d’un graphe en V1. Elle sera affichée sous forme de valeur instantanée et éventuellement de barre de progression.

---

## 6. Contraintes de faisabilité Linux

### 6.1 CPU AMD Ryzen 9 5950X

Les données suivantes doivent être considérées comme exploitables :

- pourcentage d’utilisation CPU ;
- température CPU.

La température devra être lue avec priorité sur la sonde logique correspondant à Tdie lorsqu’elle est disponible, avec repli possible sur Tctl.

La puissance CPU ne doit pas être considérée comme garantie. Elle doit être traitée comme une métrique optionnelle, dépendante de la télémétrie effectivement exposée par la plateforme.

### 6.2 GPU AMD Radeon dédiée

Les données suivantes doivent être considérées comme attendues :

- pourcentage d’utilisation GPU ;
- température GPU ;
- puissance GPU.

Comme le GPU est dédié, la puissance GPU est considérée comme une métrique pertinente et exploitable indépendamment du CPU.

---

## 7. Spécification de l’API

### 7.1 Technologie retenue

L’API sera développée en Python.

Pile technique recommandée :

- FastAPI pour l’exposition HTTP ;
- psutil pour les métriques système généralistes ;
- lecture sysfs/hwmon/DRM pour les métriques AMD spécifiques ;
- uvicorn pour l’exécution du service.

### 7.2 Rôle de l’API

L’API doit :

- collecter les métriques système ;
- normaliser les données ;
- mettre en cache les dernières valeurs ;
- maintenir un historique court en mémoire pour les graphes ;
- exposer ces données sous forme JSON compacte.

### 7.3 Endpoints V1

Les endpoints minimaux de la V1 sont :

#### `GET /api/v1/health`
Retourne :

- état du service ;
- version de l’API ;
- timestamp serveur.

#### `GET /api/v1/dashboard`
Retourne :

- snapshot courant complet pour l’écran principal.

#### `GET /api/v1/history`
Retourne :

- historique court des séries destinées aux graphes.

Paramètres envisagés :

- `window` : profondeur d’historique en secondes ;
- `step` : pas d’échantillonnage.

#### `GET /api/v1/meta`
Retourne :

- nom d’hôte ;
- caractéristiques globales ;
- liste des métriques disponibles ;
- unités associées.

---

## 8. Contrat de données JSON

### 8.1 Principes généraux

Le format JSON devra respecter les règles suivantes :

- versionné ;
- compact ;
- stable ;
- orienté affichage embarqué ;
- sans redondance inutile ;
- avec valeurs numériques directement exploitables.

### 8.2 Types et conventions

- les pourcentages seront transmis en nombre réel ;
- les températures seront transmises en degrés Celsius ;
- les puissances seront transmises en watts ;
- les tailles mémoire seront transmises en octets ;
- les timestamps seront transmis en epoch Unix ;
- les champs non disponibles pourront être transmis à `null`.

### 8.3 Exemple de payload `dashboard`

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

### 8.4 Exemple de payload `history`

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

### 8.5 Contraintes de format

- `cpu_power_w` doit être nullable ;
- `gpu_power_w` doit être prévu comme présent lorsque la télémétrie le permet ;
- les séries doivent être transmises sous forme de tableaux plats ;
- les objets inutiles ou structures verbeuses sont à éviter.

---

## 9. Spécification côté ESP32-S3

### 9.1 Environnement logiciel

Le firmware sera développé en natif via ESP-IDF.

La partie graphique utilisera LVGL.

### 9.2 Rôle du firmware

Le firmware doit :

- se connecter au réseau Wi‑Fi ;
- résoudre ou connaître l’adresse du serveur API ;
- interroger périodiquement les endpoints HTTP ;
- parser les réponses JSON ;
- stocker localement les valeurs utiles ;
- mettre à jour l’interface graphique ;
- afficher un état de connexion et de fraîcheur des données.

### 9.3 Découpage logiciel recommandé

Le firmware devra être structuré en blocs séparés :

- gestion Wi‑Fi et connectivité réseau ;
- client HTTP ;
- parsing JSON ;
- stockage local des données ;
- logique UI LVGL ;
- supervision système et gestion des erreurs.

Le rendu graphique ne devra pas dépendre directement du thread ou de la tâche réseau.

### 9.4 Rafraîchissement

Cadences recommandées :

- endpoint `dashboard` : 1 requête par seconde ;
- endpoint `history` : toutes les 5 à 10 secondes, ou à l’affichage de l’écran graphes.

L’interface graphique pourra être rafraîchie indépendamment du rythme réseau.

### 9.5 Résilience

Le firmware devra :

- conserver le dernier snapshot valide ;
- signaler explicitement les données obsolètes ;
- gérer les timeouts réseau ;
- gérer les pertes Wi‑Fi ;
- permettre une reprise propre des mises à jour.

---

## 10. Sécurité et exposition réseau

### 10.1 Hypothèse d’usage

Le projet est destiné à un usage personnel sur réseau local.

Le niveau de sécurité peut donc rester modéré, sous réserve que l’accès au réseau local soit maîtrisé.

### 10.2 Position retenue en V1

En version 1 :

- l’API pourra être exposée sur le réseau local ;
- l’authentification pourra être absente par défaut ;
- une clé API statique en header pourra être ajoutée en option si nécessaire.

### 10.3 Contraintes minimales

Le service devra au minimum permettre :

- un bind contrôlé ;
- une limitation explicite au LAN ;
- la possibilité d’ajouter une authentification légère sans refonte.

---

## 11. Découverte réseau

Deux approches sont envisageables :

- configuration statique de l’adresse IP ou du nom d’hôte du serveur ;
- découverte via mDNS.

La découverte via mDNS est recommandée pour éviter le codage en dur d’une adresse IP, sous une forme du type `linuxstats.local`.

---

## 12. Performance et contraintes non fonctionnelles

### 12.1 Côté Linux

Le service Python doit :

- avoir une faible charge CPU ;
- limiter les lectures coûteuses ;
- maintenir un cache en mémoire ;
- conserver un historique circulaire pour les graphes.

### 12.2 Côté ESP32-S3

Le firmware doit :

- limiter les allocations dynamiques répétées ;
- découpler les tâches réseau et UI ;
- rester stable sur des cycles de polling longs ;
- exploiter la PSRAM si nécessaire pour les buffers graphiques.

---

## 13. Hors périmètre V1

Ne font pas partie du périmètre de la première version :

- MQTT ;
- interface desktop Qt ;
- configuration avancée multi-machines ;
- authentification forte ;
- chiffrement complexe ;
- historisation longue durée persistante ;
- alerting avancé ;
- pilotage à distance ;
- dashboard web riche.

---

## 14. Livrables attendus

### 14.1 Backend Linux

- service Python de collecte ;
- API HTTP locale ;
- structure de cache et historique ;
- documentation des endpoints ;
- mapping métrique Linux vers champ JSON.

### 14.2 Firmware ESP32-S3

- projet ESP-IDF ;
- client HTTP ;
- parser JSON ;
- interface LVGL ;
- écrans de valeurs instantanées et graphes ;
- gestion des erreurs réseau.

### 14.3 Documentation

- contrat d’API ;
- description du format JSON ;
- stratégie de découverte réseau ;
- procédure de configuration de base.

---

## 15. Décisions validées à ce stade

Les décisions actuellement retenues sont les suivantes :

- backend Linux en Python ;
- exposition via API HTTP locale ;
- client embarqué ESP32-S3 en ESP-IDF ;
- interface graphique embarquée via LVGL ;
- JSON compact versionné ;
- polling HTTP simple ;
- focus fonctionnel sur CPU, RAM et GPU ;
- graphes sur température et utilisation CPU/GPU ;
- puissance CPU traitée comme optionnelle ;
- puissance GPU intégrée à la V1.

---

## 16. Étapes suivantes

Les prochaines étapes de conception sont :

1. figer le contrat d’API champ par champ ;
2. définir le mapping Linux exact des capteurs et fallbacks ;
3. définir les écrans et widgets LVGL ;
4. définir la structure logicielle du firmware ;
5. lancer un prototype du backend Python ;
6. lancer un prototype de consommation HTTP côté ESP32-S3.

