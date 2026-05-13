# Supervision GPU AMD

PulseMon cible la télémétrie GPU AMD via DRM Linux, hwmon et les chemins sysfs exposés par amdgpu.

## Métriques

Le périmètre GPU inclut :

- pourcentage d’utilisation GPU ;
- température GPU ;
- puissance GPU ;
- valeurs lissées prêtes à afficher ;
- valeurs brutes pour diagnostics ;
- dashboard GPU dédié ;
- historique GPU borné.

## Sélection des sources

Le backend doit identifier le GPU AMD cible via `/sys/class/drm/card*/device` et les entrées hwmon associées. `STATS_GPU_PCI_SLOT` peut forcer un périphérique si la détection automatique est ambiguë.

La priorité des labels de température est contrôlée par `STATS_GPU_TEMP_LABEL_PRIORITY`, avec des labels typiques comme edge, junction et memory.

## Lissage

L’utilisation GPU peut être visuellement nerveuse. Le backend peut exposer à la fois des valeurs brutes et des valeurs d’affichage. La valeur d’affichage utilise un lissage pour stabiliser l’UI sans masquer les diagnostics bruts.

Le firmware doit utiliser les valeurs d’affichage pour le rendu et réserver les valeurs brutes au diagnostic.

## API GPU

Endpoints GPU :

- `GET /api/v1/gpu/dashboard` ;
- `GET /api/v1/gpu/history` ;
- `GET /api/v1/gpu/meta`.

Le dashboard principal peut aussi contenir une synthèse GPU.

## Politique d’échec

Si une métrique GPU est indisponible :

- conserver le champ ;
- le marquer invalide ou nullable ;
- garder le reste du payload exploitable ;
- loguer le chemin retenu et la cause en mode diagnostic ;
- ne pas synthétiser de fausse télémétrie.

## Payload GPU détaillé

Le dashboard GPU peut exposer les enveloppes métriques suivantes :

- `gpu.pct` ;
- `gpu.core_clock_mhz` ;
- `gpu.mem_clock_mhz` ;
- `gpu.vram_used_b` ;
- `gpu.vram_total_b` ;
- `gpu.vram_pct` ;
- `gpu.temp_c` ;
- `gpu.power_w` ;
- `gpu.fan_rpm` ;
- `gpu.fan_pct`.

L’historique GPU peut exposer :

- `gpu_pct` ;
- `gpu_core_clock_mhz` ;
- `gpu_vram_used_b` ;
- `gpu_temp_c` ;
- `gpu_power_w` ;
- `gpu_mem_clock_mhz` ;
- `gpu_fan_rpm`.

Si le store historique GPU est vide, le backend peut faire une lecture live de warmup avant de répondre. Les séries indisponibles doivent retourner des points `null` plutôt que disparaître silencieusement.
