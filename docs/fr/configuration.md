# Configuration

## Environnement backend

Le backend reconnaît exactement 20 variables `STATS_*`. `api/app/config.py` les parse et les valide toutes avant la création des stores, samplers ou diagnostics. Le lanceur packagé exécute le même précontrôle avant le démarrage d’Uvicorn.

| Variable | Défaut | Contrat accepté | Rôle |
|---|---:|---|---|
| `STATS_BIND_HOST` | `0.0.0.0` | IPv4, IPv6 ou hostname valide, sans espace | adresse de bind |
| `STATS_BIND_PORT` | `8000` | entier `1..65535` | port HTTP |
| `STATS_SAMPLE_INTERVAL_S` | `0.1` | nombre fini `0.01..60.0` | intervalle d’acquisition capteurs |
| `STATS_PUBLISH_INTERVAL_S` | `0.5` | nombre fini `0.01..3600.0` | intervalle de publication snapshot/historique |
| `STATS_HISTORY_CAPACITY` | `600` | entier `1..1000000` | capacité d’historique mémoire |
| `STATS_API_KEY` | non définie | vide pour désactiver ; sinon chaîne sans espace périphérique ni caractère de contrôle | clé optionnelle pour `/api/v1/*` |
| `STATS_API_KEY_HEADER` | `X-API-Key` | token HTTP valide, 128 caractères maximum | nom du header d’authentification |
| `STATS_LOG_LEVEL` | `INFO` | `CRITICAL`, `ERROR`, `WARNING`, `INFO` ou `DEBUG` ; `WARN` et `FATAL` sont normalisés | niveau de logs application et Uvicorn |
| `STATS_DIAGNOSTICS` | `0` | `1/0`, `true/false`, `yes/no` ou `on/off` | activation diagnostics |
| `STATS_DIAG_RAW_CAPTURE` | `0` | booléen strict | activation capture brute |
| `STATS_DIAG_RAW_HZ` | `8.0` | nombre fini `0.1..1000.0` | fréquence capture brute |
| `STATS_DIAG_RAW_DURATION_S` | `60` | entier `1..86400` | durée capture brute |
| `STATS_DIAG_RAW_LOG_PATH` | `api/diagnostics/raw_metrics.jsonl` | chemin non vide sans caractère de contrôle | chemin capture brute |
| `STATS_DIAG_COMPARE_CAPTURE` | `0` | booléen strict | activation comparaison brut/affichage |
| `STATS_DIAG_COMPARE_HZ` | `10.0` | nombre fini `0.1..1000.0` | fréquence comparaison |
| `STATS_DIAG_COMPARE_DURATION_S` | `60` | entier `1..86400` | durée comparaison |
| `STATS_DIAG_COMPARE_LOG_PATH` | `api/diagnostics/raw_vs_display_gpu_pct.jsonl` | chemin non vide sans caractère de contrôle | chemin comparaison |
| `STATS_DISPLAY_EMA_ALPHA` | `0.25` | nombre fini `>0.0` et `<=1.0` | alpha EMA d’affichage |
| `STATS_GPU_PCI_SLOT` | non définie | BDF PCI `dddd:bb:ss.f` ou `bb:ss.f` | forcer un slot PCI AMD |
| `STATS_GPU_TEMP_LABEL_PRIORITY` | `edge,junction,mem,unknown` | 1 à 32 labels uniques séparés par des virgules et conformes à `[a-z0-9_-]` ; `unknown` est ajouté s’il manque | priorité des labels température GPU |

Une configuration invalide lève `ConfigError` et arrête le démarrage avec le nom de la variable et le contrat attendu. Les valeurs ne sont pas reprises dans le message afin de ne pas exposer une clé API. Les nombres non finis tels que `nan` et `inf` sont rejetés.

Validation de l’environnement courant sans démarrer le service :

```bash
cd api
python3 -m app.config
```

Le collecteur GPU ne lit plus directement ses deux variables. `load_config()` les valide et les normalise, puis `api/app/main.py` applique la sélection GPU une seule fois au démarrage.

Le backend n’utilise aucune base de configuration persistante. Les chemins de diagnostic relatifs sont résolus depuis le répertoire de travail du processus.

## Endpoint backend firmware

L’endpoint backend est réparti entre des réglages NVS et des fallbacks compilés.

Réglages runtime NVS :

```text
namespace : pulsemon_api
clés : host, port
```

Les hôtes acceptés sont les adresses IPv4 ou hostnames DNS/mDNS composés de lettres, chiffres, points et tirets internes. Les schémas, chemins, espaces, underscores et littéraux IPv6 bruts sont rejetés. Le port doit être dans `1..65535`.

Réglages compilés dans `esp/src/pulsemon_api_config.h` :

```text
PULSEMON_API_DEFAULT_HOST
PULSEMON_API_DEFAULT_PORT
PULSEMON_HTTP_TIMEOUT_MS
PULSEMON_DASHBOARD_POLL_MS
```

L’hôte/port NVS remplacent les valeurs compilées. Une sauvegarde du portail local recharge immédiatement l’endpoint actif sans redémarrage. L’effacement de la configuration PulseMon supprime le namespace `pulsemon_api` et restaure le fallback compilé. Le client conserve l’endpoint actif en RAM et ne relit pas la NVS à chaque polling. Un appui de cinq secondes dans le coin supérieur gauche d’un écran actif ouvre une fenêtre manuelle de l’AP pendant dix minutes. Le déclencheur ne modifie pas la NVS et ne coupe pas une connexion station existante.

Le firmware ne stocke ni n’envoie actuellement d’identifiant `STATS_API_KEY_HEADER`.

## Namespaces NVS firmware

Endpoint backend :

```text
namespace : pulsemon_api
clés : host, port
```

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
- Un appui maintenu cinq secondes dans le coin supérieur gauche d’un écran actif force aussi l’ouverture de l’AP pendant dix minutes.
- Après connexion station réussie, l’AP automatique est désactivé ; une fenêtre manuelle active reste ouverte jusqu’à son timeout.
- Le serveur HTTP de configuration démarre uniquement après `WIFI_EVENT_AP_START` et s’arrête après `WIFI_EVENT_AP_STOP`.
- Le DNS captif suit le même cycle AP et se lie uniquement à `192.168.4.1`, jamais à `INADDR_ANY`.
- Les handlers du portail retournent `503 Service Unavailable` si l’AP n’est plus actif.
