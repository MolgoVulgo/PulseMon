# agent-stats-linux.md

## Contexte du dépôt

Ce fichier définit le cadre de travail d’un **agent Codex** pour le projet **Stats Linux vers ESP32-S3**.

Le dépôt doit être traité comme un projet **bi-brique** :
- **backend Linux** en **Python 3.11+ / FastAPI / Pydantic / psutil** ;
- **firmware embarqué** en **ESP-IDF / C ou C++ / LVGL** pour **ESP32-S3**.

Le système cible est structuré autour de :
- une collecte locale des métriques Linux ;
- une API HTTP locale versionnée ;
- un client HTTP embarqué sur ESP32-S3 ;
- un affichage temps réel de métriques CPU / RAM / GPU.

---

## Règle de cadrage

- Traiter ce dépôt comme un projet **API Python + firmware ESP-IDF**.
- Ne pas appliquer un cadrage Qt / QML / CMake desktop hérité d’un autre projet.
- Ne pas refondre l’architecture en dehors du périmètre validé.
- Respecter strictement l’architecture retenue :
  - **Linux** : collecte + normalisation + cache + API HTTP locale ;
  - **ESP32-S3** : polling HTTP + parsing JSON + cache local + rendu LVGL.
- Ne pas introduire de MQTT en V1.
- Ne pas introduire de cloud, de broker, de persistance longue durée ou de dashboard web riche sans demande explicite.

---

## Sources de vérité du projet

Lire d’abord, dans cet ordre :
1. `cahier_des_charges_stats_linux_esp_32.md`
2. `cahier_fonctionnel_stats_linux_esp_32.md`
3. `plan_implementation_api_stats_linux.md`
4. la zone réellement touchée dans le dépôt
5. les fichiers de configuration et d’entrée du module modifié

Les décisions déjà figées sont les suivantes :
- backend Linux en Python ;
- API HTTP locale ;
- JSON compact versionné ;
- polling simple ;
- ESP32-S3 en ESP-IDF ;
- UI LVGL ;
- métriques V1 limitées à CPU / RAM / GPU ;
- graphes V1 limités à CPU/GPU usage + température ;
- `cpu.power_w` nullable ;
- `gpu.power_w` prévue au contrat ;
- pas de MQTT en V1.

---

## Architecture à respecter

### Backend Linux

Le backend est responsable de :
- lecture des métriques Linux ;
- application des fallbacks capteurs ;
- normalisation des unités ;
- maintien d’un snapshot courant ;
- maintien d’un historique court en mémoire ;
- exposition HTTP des endpoints V1 ;
- stabilité stricte du contrat JSON.

Découpage attendu en priorité :
- `config`
- `models`
- `collectors`
- `normalizers`
- `store`
- `services`
- `api`
- `diagnostics`

### Firmware ESP32-S3

Le firmware est responsable de :
- Wi‑Fi ;
- découverte ou configuration du backend ;
- client HTTP ;
- parsing JSON ;
- cache local ;
- rendu LVGL ;
- affichage de l’état réseau et de l’obsolescence.

Découpage attendu en priorité :
- réseau
- client HTTP
- parsing JSON
- cache local
- UI LVGL
- supervision / erreurs

### Règles de séparation

- Ne pas faire de lecture système Linux dans les handlers HTTP.
- Ne pas faire dépendre l’UI ESP32 d’une requête réseau synchrone en cours.
- Ne pas déplacer de logique métier côté ESP32 si elle doit être fixée côté backend.
- Ne pas casser le contrat JSON pour contourner une difficulté de collecte.

---

## Contrat API à préserver

Les endpoints V1 de référence sont :
- `GET /api/v1/health`
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`

Contraintes impératives :
- JSON versionné avec `"v": 1` ;
- structure stable ;
- noms de champs fixes ;
- unités fixes ;
- champs toujours présents ;
- valeurs absentes à `null`, jamais supprimées ;
- tableaux d’historique plats et alignés ;
- aucune reconstruction métier côté client ESP32.

Métriques instantanées V1 :
- `cpu.pct`
- `cpu.temp_c`
- `cpu.power_w`
- `mem.used_b`
- `mem.total_b`
- `mem.pct`
- `gpu.pct`
- `gpu.temp_c`
- `gpu.power_w`

Séries historiques V1 :
- `cpu_pct`
- `cpu_temp_c`
- `gpu_pct`
- `gpu_temp_c`

---

## Priorités techniques du projet

Toujours prioriser dans cet ordre :
1. exactitude du mapping capteurs Linux ;
2. stabilité du modèle interne ;
3. stabilité du JSON `dashboard` ;
4. cohérence du JSON `history` ;
5. robustesse des fallbacks ;
6. gestion de l’obsolescence ;
7. résilience réseau côté ESP32 ;
8. rendu UI.

Le principe directeur du projet est simple :
- si les métriques sont correctes,
- si le snapshot est frais,
- si l’historique est cohérent,
- et si le JSON reste stable,
- alors l’intégration ESP32 devient essentiellement un travail de consommation et d’affichage.

---

## Politique de modification

- Modifier uniquement les fichiers nécessaires à l’intention du changement.
- Conserver les changements localisés, lisibles et testables.
- Ne pas toucher aux artefacts générés, builds, caches ou dépendances lockées sans nécessité explicite.
- Ne pas ajouter de fonctionnalités hors périmètre V1 sans demande explicite.
- En cas de doute, préférer une correction minimale dans le module déjà responsable.
- Si le changement impacte le contrat JSON, mettre à jour la documentation associée et les tests de non-régression.
- Si le changement impacte les fallbacks capteurs, documenter précisément la source retenue et le fallback appliqué.
- Si le changement impacte l’ESP32, préserver le découplage réseau / parsing / cache / UI.

### Collaboration obligatoire

- Ne jamais générer du code en solo sans exposer d’abord le constat technique.
- Toujours détailler ce qui va être modifié avant toute modification de code.
- Toujours expliciter les hypothèses réelles quand une structure de dépôt ou un capteur n’est pas encore confirmé.

---

## Contraintes techniques du dépôt

### Backend

- Python 3.11+.
- Framework HTTP : FastAPI.
- Modèles : Pydantic.
- Collecte généraliste : psutil.
- Collecte spécifique AMD : sysfs / hwmon / DRM / amdgpu.
- Pas de lecture lourde à chaque requête HTTP.
- Boucle d’échantillonnage interne cible : **1 Hz**.
- Historique en ring buffer mémoire.

### Firmware

- ESP32-S3.
- ESP-IDF natif.
- UI via LVGL.
- Polling `dashboard` cible : **1 Hz**.
- Polling `history` cible : **5 à 10 s**.
- Conserver le dernier snapshot valide en cas d’échec.
- Signaler explicitement les données obsolètes.

### Interdits V1

- MQTT.
- multi-machines.
- écriture ou pilotage de la machine Linux.
- persistance longue durée.
- sécurité complexe.
- structure JSON variable selon la machine.
- valeurs synthétiques inventées pour masquer une métrique absente.

---

## Commandes standard

Les commandes exactes dépendent de la structure réelle du dépôt.
Ne pas les inventer si le dépôt ne les expose pas encore.

### Backend Python

Si un module `api/` ou équivalent existe :

```bash
python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
pytest -q
```

### Firmware ESP-IDF

Si un projet ESP-IDF est présent :

```bash
idf.py build
idf.py flash
idf.py monitor
```

### Principe

- valider uniquement ce qui est réellement exécutable dans l’environnement courant ;
- ne pas prétendre avoir compilé ou flashé si ce n’est pas le cas ;
- signaler explicitement toute limite d’environnement.

---

## Validation attendue

Avant de conclure :

### Pour le backend
- tests unitaires ou d’intégration ciblés ;
- vérification des endpoints impactés ;
- vérification de la stabilité du JSON ;
- vérification du comportement avec métriques absentes ;
- vérification de `state.ok` et `state.stale_ms`.

### Pour le firmware
- build de la cible concernée si l’environnement le permet ;
- validation du parsing JSON ;
- validation de la résilience sur perte réseau ;
- vérification rapide de non-régression UI si la zone LVGL est touchée.

### Si une validation ne peut pas être exécutée
Le signaler explicitement avec la raison réelle :
- dépendance manquante ;
- capteur absent ;
- machine Linux cible indisponible ;
- environnement ESP-IDF non disponible ;
- matériel non connecté ;
- dépôt incomplet ;
- commande non encore définie dans le projet.

---

## Règles spécifiques capteurs / métriques

- CPU usage : source attendue `psutil.cpu_percent(interval=None)`.
- CPU temp : priorité `Tdie`, fallback `Tctl`.
- CPU power : optionnelle ; `None` si non fiable ou absente.
- RAM : via `psutil.virtual_memory()`.
- GPU usage : priorité `gpu_busy_percent` côté amdgpu.
- GPU temp : lecture `hwmon`/amdgpu.
- GPU power : télémétrie amdgpu si disponible.

Règles impératives :
- ne jamais fabriquer une pseudo-mesure ;
- ne jamais supprimer un champ parce qu’une sonde est absente ;
- journaliser la cause d’absence d’une métrique ;
- conserver un comportement déterministe pour le client.

---

## Logging / diagnostics

Journaliser utilement :
- démarrage du service ;
- configuration effective ;
- capteurs détectés ;
- source retenue pour chaque métrique ;
- fallback retenu ;
- erreurs de lecture ;
- erreurs HTTP ;
- invalidité des paramètres.

Éviter :
- le bruit à chaque tick en production ;
- les logs verbeux sans valeur ;
- les suppositions présentées comme des faits.

---

## Structure de réponse attendue

- Constat technique
- Actions appliquées
- Validation exécutée
- Limites / hypothèses, si nécessaires

Style attendu :
- direct
- court
- orienté exécution
- pas de code sauf si l’utilisateur le demande
- des faits, pas des explications longues
- pas de tirades intermminables

---

## Commits Git

- Faire des commits par unité logique cohérente et testable.
- Ne jamais faire de commit automatiquement sans demande explicite de l’utilisateur.
- Ne jamais pousser soi-même.
- Messages en anglais.
- Conventional commit recommandé :

```text
<type>(<scope>): <summary>
```

Scopes recommandés dans ce projet :
- `api`
- `collectors`
- `normalizers`
- `store`
- `services`
- `firmware`
- `lvgl`
- `docs`
- `tests`

Le message doit décrire exactement le diff réel :
- `git status`
- `git diff --stat HEAD`
- `git diff HEAD`

Mentionner dans le corps :
- changements fonctionnels ;
- robustesse / gestion d’erreurs ;
- impact contrat JSON ;
- tests exécutés ;
- docs mises à jour.

### Template

```text
<type>(<scope>): <summary>

- ...
- ...

Tests: <commande> (<résultat>)
Docs: <liste de fichiers ou "none">
```
