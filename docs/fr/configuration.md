# Configuration

La configuration PulseMon est séparée entre le backend Linux et le firmware ESP32-S3.

## Variables d’environnement backend

Paramètres backend principaux :

| Variable | Rôle |
|---|---|
| `STATS_BIND_HOST` | adresse de bind HTTP, défaut `0.0.0.0` |
| `STATS_BIND_PORT` | port HTTP, défaut `8000` |
| `STATS_SAMPLE_INTERVAL_S` | intervalle d’acquisition capteurs |
| `STATS_PUBLISH_INTERVAL_S` | intervalle de publication snapshot/historique |
| `STATS_HISTORY_CAPACITY` | capacité d’historique mémoire |
| `STATS_DISPLAY_EMA_ALPHA` | facteur EMA pour les pourcentages affichés |
| `STATS_API_KEY` | clé API optionnelle |
| `STATS_API_KEY_HEADER` | header de clé API, défaut `X-API-Key` |
| `STATS_LOG_LEVEL` | niveau de logs |
| `STATS_GPU_PCI_SLOT` | forçage optionnel du GPU AMD |
| `STATS_GPU_TEMP_LABEL_PRIORITY` | priorité des labels température GPU |
| `STATS_DIAGNOSTICS` | activation diagnostics |
| `STATS_FANS_MAPPING_FILE` | fichier legacy optionnel de mapping ventilateurs |
| `STATS_CONFIG_DB_PATH` | chemin de base SQLite de configuration |

Paramètres de capture diagnostics :

| Variable | Rôle |
|---|---|
| `STATS_DIAG_RAW_CAPTURE` | active la capture brute |
| `STATS_DIAG_RAW_HZ` | fréquence de capture brute |
| `STATS_DIAG_RAW_DURATION_S` | durée de capture brute |
| `STATS_DIAG_RAW_LOG_PATH` | chemin de sortie brute |
| `STATS_DIAG_COMPARE_CAPTURE` | active la comparaison brut/affichage |
| `STATS_DIAG_COMPARE_HZ` | fréquence de comparaison |
| `STATS_DIAG_COMPARE_DURATION_S` | durée de comparaison |
| `STATS_DIAG_COMPARE_LOG_PATH` | chemin de sortie comparaison |

## Stockage backend local

Le backend utilise SQLite pour la configuration locale persistante, notamment le mapping ventilateurs et la configuration UI utilisateur.

Si la base est vide, le backend peut importer un mapping legacy JSON ou générer un mapping initial depuis les canaux détectés.

## Configuration API firmware

L’accès backend est défini dans les headers de configuration firmware :

- `PULSEMON_API_HOST` ;
- `PULSEMON_API_PORT` ;
- `PULSEMON_API_BASE_URL` ;
- `PULSEMON_HTTP_TIMEOUT_MS` ;
- `PULSEMON_DASHBOARD_POLL_MS`.

Une adresse LAN statique peut être utilisée. Un hostname ou une découverte mDNS reste compatible avec l’architecture.

## Configuration Wi-Fi firmware

Les identifiants Wi-Fi sont stockés en NVS, pas compilés dans le firmware.

Au boot :

1. le firmware vérifie les credentials station stockés ;
2. il tente la connexion Wi-Fi ;
3. si les credentials sont absents ou invalides, il ouvre l’AP `PulseMon-Setup` ;
4. le portail local est disponible sur `http://192.168.4.1/` ;
5. le DNS captif redirige les noms courants vers le portail.

## Gestion des secrets

Les clés API ne doivent jamais être :

- affichées à l’écran ;
- écrites dans les logs ;
- retournées par les APIs de configuration ;
- intégrées en query string quand un header est disponible ;
- documentées avec des valeurs locales réelles.

Les APIs de configuration doivent exposer uniquement des indicateurs de présence comme `openweather_key_set` ou `gnews_key_set`.
