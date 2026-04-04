# Spec integration ESP32 - PulseMon V1

## 1. Statut

Statut: normatif pour l'integration firmware ESP32-S3.

Reference d'interface:
- `api/docs/API_CONTRACT_V1.md`
- `api/docs/API_GPU_CONTRACT_V1.md`

En cas de divergence, le code API et les tests de contrat backend font foi.

## 2. Endpoints

### 2.1 Endpoints actuellement consommes par le firmware

- `GET /api/v1/dashboard`
- `GET /api/v1/gpu/dashboard`

### 2.2 Endpoints disponibles mais non consommes dans le firmware actuel

- `GET /api/v1/history`
- `GET /api/v1/meta`
- `GET /api/v1/health`
- `GET /api/v1/gpu/history`
- `GET /api/v1/gpu/meta`
- `GET /api/v1/fans/dashboard` (extension planifiee)
- `GET /api/v1/fans/meta` (vue technique)
- `GET /api/v1/fans/history` (optionnel)

## 3. Regles de parsing firmware

Le client JSON ESP suit cette priorite par metrique:
1. `value_display` si present et numerique
2. sinon `value_raw` si present et numerique
3. sinon scalaire numerique direct (compatibilite)
4. sinon valeur invalide (`valid=false` cote firmware)

Le firmware ne reconstruit pas de logique metier backend.

## 4. Mapping API -> UI

### 4.1 Page Main

- CPU usage <- `cpu.pct.value_display`
- CPU temp <- `cpu.temp_c.value_display`
- RAM used/total <- `mem.used_b.value_display` / `mem.total_b.value_display`
- RAM usage <- `mem.pct.value_display`
- GPU usage <- `gpu.pct.value_display`
- GPU temp <- `gpu.temp_c.value_display`
- GPU power <- `gpu.power_w.value_display`
- Meta statut <- `host`, `state.ok`, `state.stale_ms`

### 4.2 Page GPU

- GPU usage <- `gpu.pct.value_display`
- Core clock <- `gpu.core_clock_mhz.value_display`
- Memory clock <- `gpu.mem_clock_mhz.value_display`
- VRAM used/total <- `gpu.vram_used_b.value_display` / `gpu.vram_total_b.value_display`
- VRAM pct <- `gpu.vram_pct.value_display`
- Temp <- `gpu.temp_c.value_display`
- Power <- `gpu.power_w.value_display`
- Fan <- `gpu.fan_rpm.value_display` (+ `gpu.fan_pct.value_display` si present)

## 5. Graphes LVGL (etat implemente)

Les graphes ne lisent pas l'endpoint `history`.

Pipeline actuel:
- poller HTTP -> mise a jour des variables texte (`set_var_*`)
- extraction numerique locale (`vars_get_graph_sample`)
- push dans `lv_chart` chaque seconde

Implications:
- historique graphique base sur les snapshots recus localement;
- pas de resynchronisation temporelle backend via `ts_ms`;
- en cas de perte reseau, les dernieres valeurs restent affichees.

## 6. Polling et robustesse

Cadence actuelle:
- `PULSEMON_DASHBOARD_POLL_MS=1000` ms

Selection endpoint selon ecran actif:
- ecran `Main` -> `/dashboard`
- ecran `GPU` -> `/gpu/dashboard`

En cas d'echec HTTP/JSON:
- mise a jour `host_meta` avec `backend offline`
- conservation implicite des dernieres mesures affichees

## 7. Contraintes de separation

A conserver:
- reseau, parsing, vars UI et rendu LVGL separes;
- aucune attente HTTP dans le thread de rendu LVGL;
- aucune modification manuelle des fichiers `esp/src/ui/` generes.

## 8. Criteres d'acceptation firmware (etat courant)

Conforme si:
1. parse correctement les enveloppes metriques V1;
2. met a jour les pages Main/GPU sans blocage UI;
3. supporte l'absence de metrique (affichage `--`/N/A);
4. affiche un statut degrade en cas d'indisponibilite backend;
5. conserve le decouplage reseau/parsing/rendu.

## 9. Extension ajoutee - ecran GPU AMD specialise

### 9.1 Panneaux cibles

La page GPU specialisee vise 7 panneaux:
1. GPU Usage %
2. GPU Core Clock
3. VRAM
4. Temperature
5. Power
6. VRAM Clock
7. Fan

### 9.2 Mapping fonctionnel

- Usage <- `gpu.pct.value_display`
- Core clock <- `gpu.core_clock_mhz.value_display`
- VRAM <- `gpu.vram_used_b.value_display`, `gpu.vram_total_b.value_display`, `gpu.vram_pct.value_display`
- Temperature <- `gpu.temp_c.value_display`
- Power <- `gpu.power_w.value_display`
- VRAM clock <- `gpu.mem_clock_mhz.value_display`
- Fan <- `gpu.fan_rpm.value_display` (+ `gpu.fan_pct.value_display` si present)

### 9.3 Contraintes UI

- 1 panneau = titre + graphe + valeur instantanee.
- si serie indisponible: graphe vide mais panneau conserve.
- si metrique indisponible: afficher `N/A`.
- aucune derivation metier cote ESP32.

## 10. Extension planifiee - ventilateurs

Regles firmware:
- consommer prioritairement `/api/v1/fans/dashboard`;
- ne pas utiliser `/api/v1/fans/meta` pour l'affichage final;
- ne pas effectuer de remapping `fanX` -> role cote ESP32.

Affichage embarque minimal cible:
- label ventilateur;
- rpm;
- pct_fans (0..100) si disponible;
- etat stale/offline global.

Regles d'affichage panneau Fan (implementation runtime):
- `fan_1` est toujours visible;
- `fan_2` et `fan_3` passent visibles uniquement si la source JSON contient des donnees exploitables;
- `fan_4` a `fan_6` sont hors scope et doivent rester hidden;
- aucune logique de remapping cote ESP32: ordre direct `fans[0..2]`.
