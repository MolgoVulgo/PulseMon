# Cahier des charges GPU AMD V1

## 1. Portee

Extension GPU dediee sans rupture du contrat V1 principal.

Backend:
- FastAPI
- collecte amdgpu/sysfs/hwmon

Firmware:
- consommation HTTP
- affichage LVGL

## 2. Compatibilite V1

Endpoints principaux conserves:
- `GET /api/v1/dashboard`
- `GET /api/v1/history`
- `GET /api/v1/meta`

Endpoints GPU dedies:
- `GET /api/v1/gpu/dashboard`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`

Regles:
- JSON versionne `"v": 1`;
- metrique absente => enveloppe avec `value_raw/value_display = null` et `valid=false`;
- pas de champ supprime selon la machine.

## 3. Metriques GPU exposees

- `gpu.pct`
- `gpu.core_clock_mhz`
- `gpu.mem_clock_mhz`
- `gpu.vram_used_b`
- `gpu.vram_total_b`
- `gpu.vram_pct`
- `gpu.temp_c`
- `gpu.power_w`
- `gpu.fan_rpm`
- `gpu.fan_pct`

## 4. Series history GPU exposees

- `gpu_pct`
- `gpu_core_clock_mhz`
- `gpu_vram_used_b`
- `gpu_temp_c`
- `gpu_power_w`
- `gpu_mem_clock_mhz`
- `gpu_fan_rpm`

## 5. Etat firmware actuel

Le firmware consomme actuellement:
- `/api/v1/gpu/dashboard`

Le firmware ne consomme pas encore:
- `/api/v1/gpu/history`
- `/api/v1/gpu/meta`

Les graphes GPU sont alimentees localement depuis les snapshots recus.

## 6. Hors perimetre V1

- MQTT
- multi-GPU
- controle OC/ventilateurs
- cloud
- persistance longue duree

## 7. Ajout fonctionnel V1 - Ecran GPU AMD specialise

Cet ajout introduit une page GPU dediee, de type dashboard dense, sans rupture d'architecture:
- backend Linux -> API JSON -> ESP32-S3 -> LVGL

Objectifs:
- metriques GPU plus lisibles et plus riches;
- separation claire de la page GPU vs page generaliste;
- aucune logique metier derivee cote firmware.

### 7.1 Layout cible (7 panneaux)

Ligne 1:
1. GPU Usage %
2. GPU Core Clock

Ligne 2:
3. VRAM
4. Temperature
5. Power

Ligne 3:
6. VRAM Clock
7. Fan

### 7.2 Regles UI

- 1 panneau = titre + graphe court + valeur instantanee.
- 1 graphe = 1 metrique.
- metrique absente: afficher `N/A` sans casser le layout.
- graphes simples, buffers bornes, pas d'effets couteux.

## 8. Sources Linux AMD prioritaires

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

Regles:
- chaque champ contractuel existe;
- valeur indisponible: `null`;
- aucun champ ne disparait dynamiquement.

## 9. Historique GPU dedie (ajout V1)

Series requises:
- `gpu_pct`
- `gpu_core_clock_mhz`
- `gpu_vram_used_b`
- `gpu_temp_c`
- `gpu_power_w`
- `gpu_mem_clock_mhz`
- `gpu_fan_rpm`

Cadence cible:
- 1 Hz exploitable pour graphes

Profondeur recommandee:
- minimum 300 points
- cible 600 points

## 10. Regles de precision recommandees

- `%`: 1 decimale max
- `temp_c`: 1 decimale max
- `power_w`: 1 decimale max
- `clock_mhz`: entier
- `bytes`: entier
- `fan_rpm`: entier

## 11. Criteres d'acceptation de l'ajout

Conforme si:
1. les 7 panneaux sont presents;
2. chaque panneau a une valeur instantanee exploitable (ou `N/A`);
3. les graphes sont alimentes par historique GPU dedie;
4. les unites restent stables;
5. l'API GPU dediee reste distincte du dashboard generaliste.
