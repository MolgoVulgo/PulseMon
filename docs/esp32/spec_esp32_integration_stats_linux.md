# spec_esp32_integration_stats_linux.md

# Spécification d’intégration ESP32 — Source unique de vérité

## 1. Statut du document

**Statut : NORMATIF / RÉFÉRENCE ACTIVE**

Ce document est **la seule source de vérité** pour la partie firmware ESP32 du projet **Stats Linux**.

Il remplace comme référence d’intégration active :
- `cahier_des_charges_stats_linux_esp_32.md`
- `cahier_fonctionnel_stats_linux_esp_32.md`

Les autres documents actifs n’ont **aucune autorité** sur :
- le contrat JSON consommé,
- les endpoints utilisés,
- le mapping API → UI,
- les règles de fallback,
- la politique de rendu des graphes.

---

## 2. Objectif

Définir de manière unique et exploitable :

- les endpoints API consommés par l’ESP32,
- la structure JSON réellement attendue,
- les règles de sélection `value_display` vs `value_raw`,
- la politique de polling,
- la gestion du cache local,
- la stratégie de rendu des graphes,
- les règles de mode dégradé,
- les critères d’acceptation firmware.

---

## 3. Périmètre fonctionnel firmware

### 3.1 Fonctionnalités incluses

L’ESP32 doit afficher :

- **CPU**
  - utilisation (%)
  - température (°C)
  - puissance (W)
- **RAM**
  - utilisée
  - totale
  - pourcentage
- **GPU**
  - utilisation (%)
  - température (°C)
  - puissance (W)

### 3.2 Graphes obligatoires

Graphes temps réel :

- CPU usage
- CPU temperature
- GPU usage
- GPU temperature

### 3.3 Hors périmètre

Non géré par le firmware dans cette spec :

- recalcul métier des métriques
- réinterprétation des capteurs backend
- correction physique des watts
- resynchronisation des sources Linux
- lissage “scientifique” des données backend

Le firmware est **consommateur** d’un contrat déjà stabilisé.

---

## 4. Architecture firmware (résumé)

Architecture cible :

- `network_client` : requêtes HTTP GET + timeout + retry court
- `json_parser` : parsing et validation minimale
- `data_store` : snapshot courant + historique local exploitable
- `ui_cards` : cartes instantanées
- `graph_engine` : rendu graphe custom (hors `lv_chart`)
- `ui_state` : stale / degraded / error
- `scheduler` : cadence de polling et cadence de rendu découplées

**Règle structurante :**
- réseau ≠ rendu
- parsing ≠ interpolation visuelle
- historique vérité ≠ historique rendu

---

## 5. Endpoints API consommés

## 5.1 Endpoint dashboard

**Endpoint principal :**
- `GET /api/stats`

**Rôle :**
- snapshot instantané des métriques à afficher sur les cartes
- peut aussi servir de fallback si l’historique n’est pas disponible

## 5.2 Endpoint history

**Endpoint historique :**
- `GET /api/stats/history`

**Rôle :**
- séries temporelles prêtes à être utilisées pour les graphes
- horizon court, optimisé affichage temps réel

> Si l’API réelle utilise des chemins différents, **ce document doit être mis à jour**.
> Aucun autre document ne peut “corriger” ce point implicitement.

---

## 6. Contrat JSON normatif (modèle logique)

Le backend expose des métriques enrichies.  
Chaque métrique instantanée doit être traitée comme un objet structuré.

## 6.1 Type de base `MetricValue`

```json
{
  "value_raw": 42.37,
  "value_display": 41.8,
  "unit": "%",
  "source": "amdgpu",
  "sampled_at": "2026-03-26T11:00:00.250Z",
  "estimated": false,
  "valid": true
}
```

### Sémantique

- `value_raw` : valeur brute backend
- `value_display` : valeur stabilisée / prête à affichage
- `unit` : unité officielle
- `source` : source backend utile au diagnostic
- `sampled_at` : horodatage ISO8601 UTC de l’échantillon
- `estimated` : vrai si valeur estimée / dérivée / fallback backend
- `valid` : faux si la donnée ne doit pas être affichée comme fiable

---

## 7. Contrat dashboard normatif

## 7.1 Structure logique attendue

```json
{
  "timestamp": "2026-03-26T11:00:00.500Z",
  "cpu": {
    "usage_pct": { "value_raw": 37.2, "value_display": 36.9, "unit": "%", "source": "os", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true },
    "temp_c":   { "value_raw": 61.4, "value_display": 61.0, "unit": "°C", "source": "k10temp", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true },
    "power_w":  { "value_raw": 48.7, "value_display": 49.0, "unit": "W", "source": "rapl_or_estimator", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": true, "valid": true }
  },
  "memory": {
    "used_mb":  { "value_raw": 8234, "value_display": 8234, "unit": "MB", "source": "meminfo", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true },
    "total_mb": { "value_raw": 31998, "value_display": 31998, "unit": "MB", "source": "meminfo", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true },
    "usage_pct":{ "value_raw": 25.7, "value_display": 25.7, "unit": "%", "source": "meminfo", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true }
  },
  "gpu": {
    "usage_pct": { "value_raw": 78.1, "value_display": 74.8, "unit": "%", "source": "amdgpu", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true },
    "temp_c":   { "value_raw": 67.9, "value_display": 67.2, "unit": "°C", "source": "amdgpu", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true },
    "power_w":  { "value_raw": 112.3, "value_display": 111.0, "unit": "W", "source": "amdgpu", "sampled_at": "2026-03-26T11:00:00.250Z", "estimated": false, "valid": true }
  },
  "state": {
    "ok": true,
    "stale_ms": 250,
    "degraded": false
  }
}
```

## 7.2 Règles normatives

- `timestamp` = horodatage de publication du snapshot
- `state.ok = false` => dashboard non fiable globalement
- `state.stale_ms` = ancienneté perçue du snapshot côté backend
- `state.degraded = true` => backend partiellement fonctionnel, UI doit le refléter

---

## 8. Contrat history normatif

## 8.1 Structure logique attendue

```json
{
  "timestamp": "2026-03-26T11:00:00.500Z",
  "window_seconds": 60,
  "step_ms": 500,
  "series": {
    "cpu_usage_pct": [12.0, 13.1, 15.0, 14.4],
    "cpu_temp_c": [55.2, 55.3, 55.4, 55.6],
    "gpu_usage_pct": [34.0, 48.0, 61.0, 57.0],
    "gpu_temp_c": [60.1, 60.3, 60.8, 61.0]
  },
  "quality": {
    "cpu_usage_pct": { "valid": true, "estimated": false },
    "cpu_temp_c": { "valid": true, "estimated": false },
    "gpu_usage_pct": { "valid": true, "estimated": false },
    "gpu_temp_c": { "valid": true, "estimated": false }
  }
}
```

## 8.2 Règles normatives

- `window_seconds` = horizon réel du graphe
- `step_ms` = résolution nominale des points
- toutes les séries doivent avoir la **même longueur**
- les valeurs invalides doivent être :
  - soit absentes et compensées backend,
  - soit représentées par `null` si explicitement autorisé

### Règle firmware

**Le firmware doit accepter :**
- tableaux numériques
- tableaux contenant `null`

Si `null` :
- ne pas tracer de point faux
- casser visuellement le segment si nécessaire
- ne jamais interpoler brutalement une valeur invalide comme si elle était vraie

---

## 9. Politique officielle `value_display` vs `value_raw`

C’est le point critique.

## 9.1 Règle officielle

### Cartes instantanées
**Afficher `value_display`**

### Graphes temps réel
**Utiliser en priorité les séries `history` fournies par l’API**

### Fallback graphe si `history` indisponible
**Construire un historique local à partir de `value_display`**

### Mode diagnostic (optionnel)
**Afficher `value_raw` uniquement dans un écran debug / dev**

## 9.2 Interdiction

Le firmware **ne doit pas** :

- reconstruire une “meilleure” valeur métier que `value_display`
- recalculer un lissage métier concurrent du backend
- mélanger `raw` et `display` sans règle explicite

---

## 10. Politique de polling

## 10.1 Hypothèse backend stabilisé

Le backend peut :
- acquérir à fréquence plus élevée
- publier à cadence stabilisée (ex. ~2 Hz)

## 10.2 Politique firmware officielle

### Dashboard polling
- **500 ms** recommandé
- **1000 ms** toléré si charge réseau ou contraintes UI

### History polling
- **1000 ms** recommandé
- possibilité de **2000 ms** si l’historique couvre déjà une fenêtre glissante suffisante

## 10.3 Règle de découplage

Même si les requêtes sont à 500 ms / 1000 ms :

- la **boucle de rendu** doit tourner indépendamment
- le rendu visuel peut être à **20–30 FPS**
- l’UI ne doit jamais “sauter” au rythme du polling

---

## 11. Cache local firmware

## 11.1 Buffers obligatoires

### Buffer vérité (data)
Stocke :
- dernier dashboard valide
- dernière history valide
- timestamps de réception
- état de validité par bloc

### Buffer rendu (display)
Stocke :
- valeurs interpolées / animées pour cartes
- positions X/Y pré-calculées pour graphes
- état de transition visuelle

## 11.2 Règle

- `data buffer` = vérité logique
- `display buffer` = vérité visuelle temporaire

Le `display buffer` ne doit **jamais** réécrire la donnée source.

---

## 12. Règles de rendu des graphes

## 12.1 Moteur

- **Pas de `lv_chart`**
- moteur custom recommandé / retenu

## 12.2 Règles visuelles

- scrolling horizontal fluide ou redraw glissant
- pas de “saut” de frame lié à l’arrivée réseau
- interpolation visuelle autorisée **uniquement** entre deux points valides
- pas de re-normalisation brutale à chaque tick

## 12.3 Échelles

### Usage %
- min = 0
- max = 100

### Température
- plage configurable
- recommandation initiale :
  - CPU : 20 → 100 °C
  - GPU : 20 → 100 °C

## 12.4 Anti-artefacts

Interdictions :

- lisser sur un `null`
- dessiner une valeur “inventée” après perte de donnée
- faire une transition instantanée si `valid=false`

---

## 13. Gestion des états dégradés

## 13.1 Cas A — métrique invalide

Si `metric.valid = false` :

- carte : afficher `--`
- unité peut rester visible
- style visuel atténué / warning discret
- ne pas afficher une dernière valeur comme si elle était fraîche

## 13.2 Cas B — métrique estimée

Si `metric.estimated = true` :

- afficher la valeur normalement
- marquage discret possible (icône / point / teinte)
- ne pas bloquer le rendu

## 13.3 Cas C — dashboard stale

Si `state.stale_ms > 2000` :

- état visuel “stale”
- conserver la dernière valeur affichée
- désactiver les animations de montée/descente agressives

## 13.4 Cas D — history indisponible

- conserver le dernier graphe valide
- si indisponibilité prolongée :
  - passer en “frozen”
  - option fallback : alimenter le graphe avec historique local à partir du dashboard

## 13.5 Cas E — backend global KO

Si `state.ok = false` ou HTTP failure répété :

- afficher bannière / indicateur discret “API indisponible”
- conserver dernier snapshot valide pendant une fenêtre de grâce
- au-delà de **5 s**, basculer en mode dégradé explicite

---

## 14. Timeouts / retry / robustesse réseau

## 14.1 Timeouts recommandés

- connexion : 1000 ms
- lecture : 1000 ms
- timeout total requête : 1500 ms max

## 14.2 Retry

- pas de retry bloquant en boucle
- au maximum 1 retry court si la frame réseau courante échoue
- jamais de blocage de l’UI sur le réseau

---

## 15. Mapping officiel API → UI

## 15.1 Cartes

- CPU usage card ← `cpu.usage_pct.value_display`
- CPU temp card ← `cpu.temp_c.value_display`
- CPU power card ← `cpu.power_w.value_display`

- RAM used/total card ← `memory.used_mb.value_display` / `memory.total_mb.value_display`
- RAM usage card ← `memory.usage_pct.value_display`

- GPU usage card ← `gpu.usage_pct.value_display`
- GPU temp card ← `gpu.temp_c.value_display`
- GPU power card ← `gpu.power_w.value_display`

## 15.2 Graphes

- CPU usage graph ← `history.series.cpu_usage_pct`
- CPU temp graph ← `history.series.cpu_temp_c`
- GPU usage graph ← `history.series.gpu_usage_pct`
- GPU temp graph ← `history.series.gpu_temp_c`

## 15.3 Diagnostic overlay (optionnel)

- source ← `metric.source`
- stale global ← `state.stale_ms`
- valid / estimated ← flags métriques

---

## 16. Critères d’acceptation firmware

Le firmware est conforme si :

1. il n’utilise **qu’un seul document** comme référence d’intégration : celui-ci
2. les cartes affichent `value_display`
3. les graphes utilisent `history` en priorité
4. le rendu est fluide indépendamment du polling
5. aucune métrique invalide n’est affichée comme “fraîche”
6. `estimated=true` n’empêche pas l’affichage
7. l’absence de `history` ne casse pas l’UI
8. une perte réseau temporaire < 5 s ne détruit pas l’expérience
9. aucune logique firmware ne “corrige” le backend sans règle explicite
10. le code n’embarque aucune dépendance à l’ancien JSON simplifié

---

## 17. Documents non normatifs associés

### Architecture (non normatif)
- `adr_esp32_ui_architecture_lvgl_hybride.md`

### Rapports techniques (non normatifs)
- `rapport_stabilisation_courbe_gpu_2026-03-24.md`
- `rapport_technique_gpu_cpu_2026-03-24.md`

### Backend (non normatif pour firmware)
- `plan_implementation_api_stats_linux.md`

---

## 18. Documents obsolètes

À archiver :

- `cahier_des_charges_stats_linux_esp_32.md`
- `cahier_fonctionnel_stats_linux_esp_32.md`

Ces documents ne doivent plus être utilisés pour :
- le développement firmware,
- la revue de code,
- la validation d’intégration,
- les prompts Codex.

---

## 19. Décision finale

À partir de maintenant :

- **1 seule spec active**
- **0 duplication de contrat JSON**
- **0 champ API défini ailleurs**
- **1 doc d’architecture séparé**
- **anciens cahiers = archive**

Fin.
