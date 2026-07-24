# Configuration

## Environnement backend

Variables lues par `api/app/config.py` :

| Variable | Défaut | Rôle |
|---|---:|---|
| `STATS_BIND_HOST` | `0.0.0.0` | adresse de bind |
| `STATS_BIND_PORT` | `8000` | port HTTP |
| `STATS_SAMPLE_INTERVAL_S` | `0.1` | intervalle d’acquisition capteurs |
| `STATS_PUBLISH_INTERVAL_S` | `0.5` | intervalle de publication snapshot/historique |
| `STATS_HISTORY_CAPACITY` | `600` | capacité d’historique mémoire |
| `STATS_API_KEY` | non définie | clé optionnelle pour `/api/v1/*` |
| `STATS_API_KEY_HEADER` | `X-API-Key` | nom du header d’authentification |
| `STATS_LOG_LEVEL` | `INFO` | niveau de logs application |
| `STATS_DIAGNOSTICS` | `0` | activation diagnostics |
| `STATS_DIAG_RAW_CAPTURE` | `0` | activation capture brute |
| `STATS_DIAG_RAW_HZ` | `8.0` | fréquence capture brute |
| `STATS_DIAG_RAW_DURATION_S` | `60` | durée capture brute |
| `STATS_DIAG_RAW_LOG_PATH` | `api/diagnostics/raw_metrics.jsonl` | chemin capture brute |
| `STATS_DIAG_COMPARE_CAPTURE` | `0` | activation comparaison brut/affichage |
| `STATS_DIAG_COMPARE_HZ` | `10.0` | fréquence comparaison |
| `STATS_DIAG_COMPARE_DURATION_S` | `60` | durée comparaison |
| `STATS_DIAG_COMPARE_LOG_PATH` | `api/diagnostics/raw_vs_display_gpu_pct.jsonl` | chemin comparaison |
| `STATS_DISPLAY_EMA_ALPHA` | `0.25` | alpha EMA d’affichage |

Variables lues directement par les collecteurs ou services :

| Variable | Défaut/comportement | Rôle |
|---|---|---|
| `STATS_GPU_PCI_SLOT` | sélection automatique | forcer un slot PCI AMD |
| `STATS_GPU_TEMP_LABEL_PRIORITY` | `edge,junction,mem,unknown` | priorité des labels température GPU |
| `STATS_CONFIG_DB_PATH` | résolution ci-dessous | chemin base SQLite |
| `STATS_FANS_MAPPING_FILE` | chaîne de fallback service | import legacy mapping FAN |
| `STATS_FANS_REFERENCE_SEED_FILE` | fichier dépôt `tmp/fan_reference_seed.json` sauf override | catalogue de références FAN conservé |

## Chemin SQLite

Ordre de résolution :

1. `STATS_CONFIG_DB_PATH` ;
2. `~/.config/pulsemon/config.db` si inscriptible ;
3. `/tmp/pulsemon/config.db`.

Le fichier `pulsemon-api.conf` fourni ne définit actuellement pas `STATS_CONFIG_DB_PATH` ; un déploiement exigeant un chemin persistant fixe doit le définir explicitement.

## Endpoint backend firmware

L’endpoint backend est compilé dans `esp/src/pulsemon_api_config.h` :

```text
PULSEMON_API_HOST
PULSEMON_API_PORT
PULSEMON_API_BASE_URL
PULSEMON_HTTP_TIMEOUT_MS
PULSEMON_DASHBOARD_POLL_MS
```

Les valeurs courantes contiennent une adresse LAN statique et un port HTTP. Elles ne sont pas stockées en NVS et ne sont pas modifiables depuis le portail local.

Le firmware n’envoie actuellement pas `STATS_API_KEY_HEADER`.

## Namespaces NVS firmware

Identifiants Wi-Fi :

```text
namespace : pulsemon_wifi
clés : ssid, password
```

Paramètres météo :

```text
namespace : pulsemon_cfg
clés : ow_key, gmt_min, ow_city, lang
```

Paramètres news :

```text
namespace : news
clés : provider, gnews_key, enabled, refresh_min, category, lang,
       country, max_items, slide_speed, max_age_days, last_ok_ts, last_error
```

Les valeurs météo par défaut sont un décalage GMT de `+60 minutes`, la langue `fr`, sans ville ni clé. Les valeurs news par défaut sont GNews activé, rafraîchissement `30 minutes`, catégorie `general`, langue/pays `fr`, maximum `5` items, vitesse `35` et âge maximal `15 jours`.

## Comportement Wi-Fi

- Avec des identifiants stockés, le firmware démarre en mode station et se connecte.
- Sans identifiants valides, il démarre en mode AP+station et expose l’AP ouvert `PulseMon-Setup`.
- Après plusieurs échecs station, l’AP de configuration est activé.
- Après connexion station réussie, l’AP est désactivé.
- Le serveur HTTP de configuration et le DNS captif sont démarrés pendant le démarrage firmware normal.
