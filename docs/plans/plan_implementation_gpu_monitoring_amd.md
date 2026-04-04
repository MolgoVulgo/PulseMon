# Plan d'implementation - Monitoring GPU AMD specialise

## 1. Objet

Planifier l'ajout de l'ecran GPU AMD specialise (7 panneaux) en conservant:
- l'architecture actuelle;
- le contrat JSON V1 stable;
- le decouplage backend/API/ESP32/LVGL.

## 2. Perimetre

Inclus:
- metriques GPU etendues (clock, VRAM, fan);
- historique GPU dedie;
- adaptation affichage firmware GPU;
- validation contrat et non-regression.

Exclus:
- OC/undervolt;
- controle ventilateurs;
- multi-GPU;
- MQTT/cloud.

## 3. Lots d'implementation

### Lot A - Backend collecte et normalisation

Objectif:
- fiabiliser la lecture de `gpu_busy_percent`, clocks, VRAM, temp, power, fan.

Actions:
1. verifier priorites de sources sysfs/hwmon/drm.
2. normaliser unites:
   - MHz pour clocks
   - bytes pour VRAM
   - W pour power
   - RPM/% pour fan
3. garantir `null` explicite pour metriques indisponibles.
4. journaliser la source/fallback retenu.

Sortie attendue:
- snapshot GPU complet, deterministe, stable.

### Lot B - Contrat API GPU

Objectif:
- figer `GET /api/v1/gpu/dashboard|history|meta` pour la page specialisee.

Actions:
1. garantir presence des champs GPU requis.
2. verifier alignement des series history GPU.
3. stabiliser `caps` dans `meta` (ex: `fan_pct`, `hotspot_temp`, `mem_temp`).

Sortie attendue:
- contrat compatible clients ESP32 sans logique additionnelle.

### Lot C - Historique GPU

Objectif:
- fournir un historique exploitable pour graphes courts.

Actions:
1. viser cadence exploitable 1 Hz pour la restitution des series.
2. valider profondeur (>=300 points, cible 600).
3. verifier ordre chronologique et tableaux plats.

Sortie attendue:
- series fiables pour les 7 panneaux.

### Lot D - Firmware LVGL

Objectif:
- rendre les 7 panneaux GPU specialises sans regression UI.

Actions:
1. mapper les metriques GPU vers variables d'affichage.
2. conserver pipeline poll/parse/cache/rendu actuel.
3. afficher `N/A` pour metriques absentes.
4. maintenir fluidite (pas de blocage long, buffers bornes).

Sortie attendue:
- page GPU lisible et stable sur ESP32-S3.

### Lot E - Validation

Objectif:
- verifier fonctionnalite et non-regression.

Actions:
1. tests backend cibles contrat GPU.
2. verification des payloads GPU (`dashboard`, `history`, `meta`).
3. verification de non-regression des pages existantes Main/GPU.

Sortie attendue:
- ajout valide sans rupture du contrat V1.

## 4. Ordonnancement recommande

1. Lot A
2. Lot B
3. Lot C
4. Lot D
5. Lot E

## 5. Risques

1. variabilite capteurs AMD selon kernel/driver.
2. disponibilite partielle de `fan_pct`.
3. surcharge visuelle ou performance LVGL sur page dense.

Mitigations:
- fallback strict `null`;
- caps explicites dans `meta`;
- simplification graphes et cadence maitrisee.
