# Cahier des charges — Monitoring GPU AMD dédié (écran spécialisé)

## 1. Objet

Définir un écran spécialisé dédié au monitoring **GPU AMD** pour le projet Stats Linux.

Cet écran complète la page généraliste existante.

Objectif :
- afficher des métriques GPU plus riches et plus lisibles ;
- se rapprocher de la logique d’un panneau type Adrenalin ;
- rester compatible avec l’architecture existante : **API Linux → JSON → ESP32-S3 → LVGL**.

---

## 2. Périmètre V1 retenu

La page spécialisée GPU AMD doit afficher les métriques suivantes.

### 2.1 Métriques instantanées principales

#### Charge / fréquences
- utilisation GPU (%)
- fréquence GPU cœur (MHz/GHz)
- fréquence VRAM (MHz/GHz)

#### Mémoire vidéo
- VRAM utilisée (octets, affichage GiB)
- VRAM totale (octets, affichage GiB)
- VRAM utilisée (%)

#### Thermique / puissance
- température GPU edge/core (°C)
- puissance GPU board / package (W)

#### Refroidissement
- vitesse ventilateur (RPM)
- vitesse ventilateur (%) si disponible

---

## 3. Analyse de la maquette fournie

L’image fournie montre un écran orienté “AMD Radeon Adrenalin clone” avec les blocs suivants :

- **GPU Usage %**
- **GPU Usage GHz** (fréquence cœur)
- **VRAM**
- **Temperature**
- **Power**
- **VRAM GHz**
- **Fan**

### 3.1 Ce que la maquette valide

La maquette valide la structure fonctionnelle suivante :

- page **100 % dédiée GPU** ;
- mélange de **graphes courts + valeur instantanée** ;
- priorité donnée à la lecture rapide ;
- chaque graphe expose une **métrique unique** ;
- affichage en style dashboard dense ;
- hiérarchie claire : usage / fréquence / mémoire / thermique / puissance / ventilation.

### 3.2 Ajustements à retenir pour la V1

La maquette est pertinente, mais la V1 doit rester stricte :

- conserver **1 graphe = 1 métrique** ;
- éviter les graphes “décoratifs” sans utilité de décision ;
- garder des métriques réellement lisibles sur petit écran ;
- ne pas dépendre de capteurs AMD non universels.

---

## 4. Architecture imposée

Aucune rupture d’architecture.

### Backend Linux
- collecte GPU AMD via `amdgpu` / `sysfs` / `hwmon` / `drm`
- normalisation des unités
- snapshot courant
- historique court en mémoire
- exposition HTTP JSON stable

### ESP32-S3
- polling HTTP
- parsing JSON
- cache local
- rendu LVGL
- graphes spécialisés GPU

### Règle
- **aucune logique de calcul métier côté ESP32**
- **aucune dérivation de capteur côté UI**

---

## 5. Sources Linux AMD à prévoir

## 5.1 Sources prioritaires

Les métriques GPU AMD doivent être recherchées prioritairement dans :

- `/sys/class/drm/card*/device/gpu_busy_percent`
- `/sys/class/drm/card*/device/mem_info_vram_used`
- `/sys/class/drm/card*/device/mem_info_vram_total`
- `/sys/class/drm/card*/device/pp_dpm_sclk`
- `/sys/class/drm/card*/device/pp_dpm_mclk`
- `/sys/class/drm/card*/device/hwmon/hwmon*/temp*_input`
- `/sys/class/drm/card*/device/hwmon/hwmon*/power*_average`
- `/sys/class/drm/card*/device/hwmon/hwmon*/fan*_input`
- `/sys/class/drm/card*/device/hwmon/hwmon*/pwm*_enable`
- `/sys/class/drm/card*/device/hwmon/hwmon*/pwm*`

## 5.2 Règle de compatibilité

Toutes les métriques ne sont **pas garanties** selon :

- génération GPU AMD ;
- pilote kernel ;
- version Mesa / amdgpu ;
- exposition hwmon effective ;
- droits d’accès.

### Règle imposée
- chaque champ existe dans le JSON ;
- si absent : `null` ;
- aucun champ ne disparaît dynamiquement.

---

## 6. Métriques V1 obligatoires

Les métriques suivantes sont **obligatoires dans le contrat**, même si certaines peuvent être `null`.

### 6.1 Charge GPU
- `gpu.pct`
- source attendue : `gpu_busy_percent`
- unité : `%`
- plage : `0.0 → 100.0`

### 6.2 Fréquence GPU cœur
- `gpu.core_clock_mhz`
- source prioritaire : état actif extrait de `pp_dpm_sclk`
- unité : `MHz`

### 6.3 Fréquence VRAM
- `gpu.mem_clock_mhz`
- source prioritaire : état actif extrait de `pp_dpm_mclk`
- unité : `MHz`

### 6.4 VRAM
- `gpu.vram_used_b`
- `gpu.vram_total_b`
- `gpu.vram_pct`
- source : `mem_info_vram_used` / `mem_info_vram_total`

### 6.5 Température GPU
- `gpu.temp_c`
- source prioritaire : capteur principal GPU AMD via hwmon
- unité : `°C`

### 6.6 Puissance GPU
- `gpu.power_w`
- source prioritaire : `power*_average`
- unité : `W`

### 6.7 Ventilation
- `gpu.fan_rpm`
- `gpu.fan_pct` (optionnelle mais prévue)
- source prioritaire : `fan*_input`
- unité : `RPM` / `%`

---

## 7. Métriques V1.1 / optionnelles (prévoir mais non bloquantes)

Ces métriques peuvent être prévues dans `meta`, mais ne doivent pas bloquer la V1.

- température hotspot / junction
- température mémoire (si exposée)
- tension GPU (mV)
- limite de puissance (power cap)
- power state / perf level
- PCIe link speed / width
- encode/decode usage
- multi-fan détaillé

Décision :
- **hors écran V1 principal**
- possible écran diagnostic ultérieur

---

## 8. Structure fonctionnelle de l’écran GPU spécialisé

## 8.1 Layout retenu V1

La page spécialisée doit être organisée en **7 panneaux**.

### Ligne 1
1. **GPU Usage %**
2. **GPU Core Clock**

### Ligne 2
3. **VRAM**
4. **Temperature**
5. **Power**

### Ligne 3
6. **VRAM Clock**
7. **Fan**

Cette structure reprend la logique de la maquette et reste cohérente pour LVGL.

---

## 9. Contenu fonctionnel de chaque panneau

## 9.1 Panneau — GPU Usage %

### Affichage
- graphe historique court
- valeur instantanée
- libellé : `GPU Utilisation`

### Données
- source instantanée : `gpu.pct`
- historique : `gpu_pct`

### Unité
- `%`

### Contraintes
- axe Y fixe : `0 → 100`

---

## 9.2 Panneau — GPU Core Clock

### Affichage
- graphe historique court
- valeur instantanée
- libellé : `GPU Fréquence`

### Données
- source instantanée : `gpu.core_clock_mhz`
- historique : `gpu_core_clock_mhz`

### Unité
- affichage adaptatif : `MHz` ou `GHz`
- transport API : **MHz uniquement**

### Contraintes
- axe Y dynamique borné selon GPU

---

## 9.3 Panneau — VRAM

### Affichage
- graphe historique de consommation VRAM
- valeur utilisée
- valeur totale
- ratio ou pourcentage

### Données
- `gpu.vram_used_b`
- `gpu.vram_total_b`
- `gpu.vram_pct`
- historique : `gpu_vram_used_b`

### Unité
- transport : octets
- affichage : `GiB`

### Contraintes
- axe Y basé sur `vram_total`

---

## 9.4 Panneau — Temperature

### Affichage
- graphe historique court
- valeur instantanée
- libellé : `GPU Température`

### Données
- `gpu.temp_c`
- historique : `gpu_temp_c`

### Unité
- `°C`

### Contraintes
- axe Y recommandé : `0 → 120`

---

## 9.5 Panneau — Power

### Affichage
- graphe historique court
- valeur instantanée
- libellé : `GPU PPT` ou `GPU Power`

### Données
- `gpu.power_w`
- historique : `gpu_power_w`

### Unité
- `W`

### Contraintes
- axe Y dynamique avec borne supérieure issue de `meta` si connue

---

## 9.6 Panneau — VRAM Clock

### Affichage
- graphe historique court
- valeur instantanée
- libellé : `GPU Fréquence mémoire`

### Données
- `gpu.mem_clock_mhz`
- historique : `gpu_mem_clock_mhz`

### Unité
- transport : `MHz`
- affichage : `MHz/GHz`

---

## 9.7 Panneau — Fan

### Affichage
- graphe historique court
- valeur instantanée RPM
- pourcentage si disponible
- libellé : `GPU Ventilateur`

### Données
- `gpu.fan_rpm`
- `gpu.fan_pct`
- historique : `gpu_fan_rpm`

### Unité
- `RPM`
- `%` si disponible

### Contraintes
- axe Y recommandé : `0 → max_fan_rpm` si connu, sinon dynamique borné

---

## 10. Historique à maintenir côté backend

Le backend doit maintenir un historique court dédié GPU.

### Séries V1 requises
- `gpu_pct`
- `gpu_core_clock_mhz`
- `gpu_vram_used_b`
- `gpu_temp_c`
- `gpu_power_w`
- `gpu_mem_clock_mhz`
- `gpu_fan_rpm`

### Cadence
- échantillonnage interne : **1 Hz**

### Profondeur recommandée
- **300 points minimum**
- **600 points conseillé**

---

## 11. API dédiée recommandée

La page GPU spécialisée ne doit pas surcharger le `dashboard` généraliste.

### Endpoint recommandé V1
- `GET /api/v1/gpu/dashboard`

### Endpoint historique recommandé V1
- `GET /api/v1/gpu/history?window=300&step=1`

### Endpoint méta recommandé
- `GET /api/v1/gpu/meta`

Décision :
- **séparer le contrat GPU spécialisé du dashboard global**
- éviter de gonfler inutilement la page généraliste

---

## 12. Contrat JSON — GPU dashboard

### Réponse cible

```json
{
  "v": 1,
  "ts": 1774256402,
  "host": "linux-main",
  "gpu": {
    "pct": 10.0,
    "core_clock_mhz": 739,
    "mem_clock_mhz": 1300,
    "vram_used_b": 2040109465,
    "vram_total_b": 17080198144,
    "vram_pct": 12.0,
    "temp_c": 56.0,
    "power_w": 49.0,
    "fan_rpm": 0,
    "fan_pct": null
  },
  "state": {
    "ok": true,
    "stale_ms": 0
  }
}
```

### Règles
- champs fixes
- unités fixes
- valeurs numériques directes
- `null` si indisponible

---

## 13. Contrat JSON — GPU history

### Réponse cible

```json
{
  "v": 1,
  "ts": 1774256402,
  "window_s": 300,
  "step_s": 1,
  "series": {
    "gpu_pct": [7.0, 8.0, 10.0],
    "gpu_core_clock_mhz": [650, 680, 739],
    "gpu_vram_used_b": [1986422374, 2013265920, 2040109465],
    "gpu_temp_c": [55.0, 55.0, 56.0],
    "gpu_power_w": [47.0, 48.0, 49.0],
    "gpu_mem_clock_mhz": [1300, 1300, 1300],
    "gpu_fan_rpm": [0, 0, 0]
  }
}
```

### Contraintes
- séries alignées
- tableaux plats uniquement
- ordre chronologique
- aucune interpolation côté ESP32

---

## 14. Contrat JSON — GPU meta

### Réponse cible

```json
{
  "v": 1,
  "host": "linux-main",
  "gpu_name": "AMD Radeon RX",
  "metrics": [
    "gpu.pct",
    "gpu.core_clock_mhz",
    "gpu.mem_clock_mhz",
    "gpu.vram_used_b",
    "gpu.vram_total_b",
    "gpu.vram_pct",
    "gpu.temp_c",
    "gpu.power_w",
    "gpu.fan_rpm",
    "gpu.fan_pct"
  ],
  "history_series": [
    "gpu_pct",
    "gpu_core_clock_mhz",
    "gpu_vram_used_b",
    "gpu_temp_c",
    "gpu_power_w",
    "gpu_mem_clock_mhz",
    "gpu_fan_rpm"
  ],
  "caps": {
    "fan_pct": false,
    "hotspot_temp": false,
    "mem_temp": false
  }
}
```

---

## 15. Règles de précision

Pour rester stable côté ESP32 :

- `%` : 1 décimale max
- `temp_c` : 1 décimale max
- `power_w` : 1 décimale max
- `clock_mhz` : entier
- `bytes` : entier
- `fan_rpm` : entier

---

## 16. Contraintes UI LVGL

## 16.1 Règles imposées

- graphes **simples et fluides**
- pas d’effets coûteux
- buffers bornés
- pas de recalcul complexe côté UI
- réutilisation du système de graphes existant si possible

## 16.2 Règle d’affichage

- chaque panneau = **titre + graphe + ligne valeur instantanée**
- valeur instantanée toujours visible même si graphe non disponible
- si série absente : graphe vide mais panneau conservé

## 16.3 Gestion des indisponibilités

Si une métrique est absente :
- afficher `N/A`
- conserver la structure
- ne pas casser le layout

---

## 17. Limites V1

La page GPU spécialisée V1 ne doit pas intégrer :

- OC / undervolt
- changement de power profile
- réglage ventilateurs
- monitoring multi-GPU
- encode/decode détaillé
- graphes multiples superposés
- histogrammes complexes
- diagnostics bas niveau PCIe

---

## 18. Critères d’acceptation

La page GPU AMD spécialisée est conforme si :

1. elle affiche les **7 panneaux** validés ;
2. chaque panneau possède une valeur instantanée exploitable ;
3. les graphes sont alimentés par un historique 1 Hz ;
4. les unités sont stables ;
5. les métriques absentes restent `null` sans casser l’UI ;
6. l’API dédiée GPU est distincte du dashboard généraliste ;
7. l’écran reste lisible sur ESP32-S3 avec LVGL ;
8. les métriques principales AMD sont cohérentes avec la télémétrie système disponible.

---

## 19. Décisions figées pour la suite

### Retenu
- écran GPU spécialisé séparé
- cible AMD uniquement en V1
- 7 panneaux type Adrenalin-like
- API GPU dédiée
- historique GPU dédié
- JSON stable et borné
- aucun calcul métier côté ESP32

### Priorité absolue
1. fiabiliser le mapping AMD Linux réel ;
2. valider les capteurs réellement disponibles sur la machine cible ;
3. figer le contrat JSON GPU ;
4. seulement ensuite, figer la maquette LVGL.

---

## 20. Références projet

Ce cahier s’aligne sur les documents existants du projet Stats Linux :

- cahier des charges global ESP32/API
- cahier fonctionnel
- plan d’implémentation API
- logique de séparation snapshot / history déjà retenue

