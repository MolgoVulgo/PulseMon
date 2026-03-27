# Spec integration ESP32 - PulseMon V1

## 1. Statut

Statut: NORMATIF pour l'integration firmware ESP32-S3.

Cette spec definit:
- l'usage de l'API backend existante;
- le mapping API -> UI LVGL;
- les regles de polling, cache local et mode degrade.

Cette spec est coherente avec:
- `docs/specs/cahier_des_charges_stats_linux_esp_32.md`
- `docs/specs/cahier_fonctionnel_stats_linux_esp_32.md`
- `docs/plans/plan_implementation_api_stats_linux.md`
- `api/docs/API_CONTRACT_V1.md`

Si divergence, le contrat HTTP de reference est `api/docs/API_CONTRACT_V1.md`.

---

## 2. Endpoints consommes

- `GET /api/v1/dashboard`
- `GET /api/v1/history?window=<1..600>&step=<1..10>&mode=display`
- `GET /api/v1/health` (etat service)
- `GET /api/v1/meta` (capacites exposees)

Le firmware consomme exclusivement les endpoints versionnes `/api/v1/*`.

---

## 3. Contrat dashboard (resume)

Le payload est versionne `"v": 1` et expose des metriques enveloppees:

```json
{
  "value_raw": 83.0,
  "value_display": 89.4,
  "source": "/sys/class/drm/card1/device/gpu_busy_percent",
  "unit": "percent",
  "sampled_at": 1774256402000,
  "estimated": false,
  "valid": true
}
```

Structure V1 attendue:
- `cpu.pct`, `cpu.temp_c`, `cpu.power_w`
- `mem.used_b`, `mem.total_b`, `mem.pct`
- `gpu.pct`, `gpu.temp_c`, `gpu.power_w`
- `state.ok`, `state.stale_ms`

Regles:
- champs toujours presents;
- donnee absente = `null` + `valid=false`;
- aucune valeur synthetique inventee cote firmware.

---

## 4. Contrat history (resume)

Structure V1 attendue:
- `v`, `ts`, `ts_ms`, `window_s`, `step_s`
- `series.cpu_pct`
- `series.cpu_temp_c`
- `series.gpu_pct`
- `series.gpu_temp_c`

Regles:
- toutes les series alignees sur `ts_ms`;
- trous temporels possibles -> `null`;
- pas d'interpolation metier firmware sur des `null`.

---

## 5. Politique value_display vs value_raw

- Cartes instantanees: afficher `value_display`.
- Graphes: utiliser prioritairement `history` (mode `display`).
- Debug optionnel: `value_raw` possible en overlay technique.

Interdictions:
- recalculer un lissage metier concurrent du backend;
- melanger raw/display sans regle explicite;
- remplacer une mesure invalide par 0.

---

## 6. Polling et rendu

Cadences cibles firmware:
- dashboard: 1 Hz (500 ms possible si budget CPU/reseau stable)
- history: 5 a 10 s

Cadence rendu UI:
- decouplee du reseau;
- aucune attente HTTP dans le thread de rendu.

---

## 7. Cache local et resilience

Buffers minimaux:
- data buffer: dernier dashboard valide + dernier history valide;
- display buffer: etat visuel temporaire (animations, points ecran).

Comportement degrade:
- en echec reseau, conserver le dernier snapshot valide;
- afficher explicitement etat stale/degrade;
- ne pas presenter une valeur obsolete comme fraiche.

---

## 8. Mapping officiel API -> UI

Cartes:
- CPU usage <- `cpu.pct.value_display`
- CPU temp <- `cpu.temp_c.value_display`
- CPU power <- `cpu.power_w.value_display`
- RAM used/total <- `mem.used_b.value_display` / `mem.total_b.value_display`
- RAM usage <- `mem.pct.value_display`
- GPU usage <- `gpu.pct.value_display`
- GPU temp <- `gpu.temp_c.value_display`
- GPU power <- `gpu.power_w.value_display`

Graphes:
- CPU usage <- `series.cpu_pct`
- CPU temp <- `series.cpu_temp_c`
- GPU usage <- `series.gpu_pct`
- GPU temp <- `series.gpu_temp_c`

---

## 9. Acceptation firmware

Conforme V1 si:
1. endpoints `/api/v1/*` utilises;
2. cartes basees sur `value_display`;
3. graphes bases sur `history.series.*`;
4. `null/valid=false` geres sans artefact visuel;
5. stale/degrade visible en UI;
6. reseau, parsing, cache, rendu restent decouples.

---

## 10. Documents associes

- ADR UI: `docs/adr/adr_esp32_ui_architecture_lvgl_hybride.md`
- Plan API: `docs/plans/plan_implementation_api_stats_linux.md`
- Rapports: `docs/reports/`
