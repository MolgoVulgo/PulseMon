# Dépannage

## Backend indisponible

Vérifier :

- état du processus ou service systemd ;
- adresse de bind et port ;
- route `GET /api/v1/health` ;
- permissions sur sysfs, hwmon et DRM ;
- configuration optionnelle `STATS_API_KEY`.

Ne pas activer `STATS_API_KEY` pour le flux ESP/UI courant standard : ces clients n’envoient pas encore le header.

## Configuration SQLite non persistante

Vérifier le chemin résolu via `/api/v1/db/data` ou les logs. Définir explicitement `STATS_CONFIG_DB_PATH` pour un chemin persistant fixe. Sans cette variable, le backend utilise `~/.config/pulsemon/config.db` si possible, sinon `/tmp/pulsemon/config.db`.

## L’ESP32 affiche backend offline

L’endpoint backend est compilé dans `esp/src/pulsemon_api_config.h`. Vérifier :

- `PULSEMON_API_HOST` ;
- cohérence de `PULSEMON_API_PORT` et `PULSEMON_API_BASE_URL` ;
- routage LAN et firewall ;
- port backend 8000 sauf modification aux deux extrémités ;
- timeout HTTP.

Il n’existe actuellement aucun réglage portail ou NVS pour l’adresse backend.

## Configuration Wi-Fi ESP32

Vérifier :

- namespace NVS `pulsemon_wifi` ;
- AP `PulseMon-Setup` si identifiants absents ou retries épuisés ;
- `GET /api/wifi/status` ;
- réseaux visibles via `GET /api/wifi/scan`.

## Météo indisponible

Vérifier :

- présence clé OpenWeather et ID ville ;
- langue et décalage GMT valides ;
- Wi-Fi et DNS ;
- accès HTTP clair à `api.openweathermap.org` dans l’implémentation courante ;
- fichiers d’icônes SD si seules les icônes manquent.

OpenWeather en HTTP est un défaut connu, pas l’état de sécurité final visé.

## Actualités indisponibles

Vérifier :

- heure SNTP valide ;
- présence de la clé GNews ;
- accès HTTPS/DNS à GNews ;
- intervalle de rafraîchissement et backoff ;
- règles d’âge et validation des articles.

## FAN

FAN est conservé mais inactif dans le firmware. L’absence de navigation ou polling FAN est le comportement courant attendu, pas une panne opérationnelle.

## Diagnostics backend

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```
