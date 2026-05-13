# Dépannage

## Le backend n’expose pas les métriques

Vérifier :

- le processus service est lancé ;
- le bind host et le port sont corrects ;
- l’utilisateur Linux peut lire sysfs, hwmon et DRM ;
- la sélection de chemin GPU AMD est correcte ;
- le mode diagnostics indique les chemins capteurs retenus ;
- le header API key est présent si l’authentification est activée.

## Les valeurs GPU manquent

Vérifier :

- le GPU est visible sous `/sys/class/drm/card*/device` ;
- le pilote amdgpu expose `gpu_busy_percent` ou une télémétrie équivalente ;
- les entrées hwmon exposent des labels température ;
- `STATS_GPU_PCI_SLOT` est défini si la sélection automatique prend la mauvaise carte ;
- `STATS_GPU_TEMP_LABEL_PRIORITY` correspond aux labels exposés.

## La courbe GPU est trop nerveuse

Utiliser les valeurs d’affichage pour l’UI et les valeurs brutes pour diagnostics. Vérifier `STATS_DISPLAY_EMA_ALPHA` et comparer les captures raw/display.

## Le pourcentage ventilateur vaut null

Vérifier :

- le canal ventilateur est détecté ;
- le mapping existe dans la configuration SQLite ;
- `rpm_min` et `rpm_max` sont valides ;
- la valeur runtime est dans une plage plausible ;
- l’import legacy n’a pas échoué silencieusement.

## L’ESP32 ne se connecte pas au Wi-Fi

Vérifier :

- les credentials stockés en NVS ;
- la disponibilité du portail `PulseMon-Setup` ;
- l’adresse captive `http://192.168.4.1/` ;
- les constantes de retry ;
- la puissance du signal et la visibilité du SSID.

## L’ESP32 indique backend offline

Vérifier :

- le backend est joignable depuis le même LAN ;
- l’hôte, port et base URL configurés ;
- les règles firewall ;
- le header API key optionnel ;
- le timeout HTTP ;
- la route backend `/api/v1/health`.

## Météo ou news absentes

Vérifier :

- Wi-Fi connecté ;
- heure SNTP valide ;
- indicateur de présence de clé OpenWeather ou GNews à true ;
- clés stockées en NVS ;
- absence d’erreur quota ou autorisation active ;
- priorité d’alerte météo ne masquant pas les news ;
- backoff news écoulé ;
- fichiers d’icônes présents sur SD si l’affichage icônes est impacté.

## Diagnostics utiles

Comparaison GPU brut/affichage :

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

Capture brute multi-métriques :

```bash
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```
