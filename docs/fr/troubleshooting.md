# Dépannage

## Backend indisponible

Vérifier :

- état du processus ou du service systemd ;
- adresse de bind et port ;
- route `GET /api/v1/health` ;
- permissions sur les chemins sysfs, hwmon et DRM ;
- configuration optionnelle `STATS_API_KEY`.

Ne pas activer `STATS_API_KEY` pour le flux ESP/UI standard courant : ces clients n’envoient pas encore le header.

## État backend après redémarrage

Le backend n’utilise aucune base persistante. Les snapshots et historiques courts sont reconstruits en mémoire après redémarrage.

## Échec backend pendant le chargement de configuration

Exécuter le précontrôle avec le même environnement que le service :

```bash
cd api
python3 -m app.config
```

L’erreur indique la variable invalide et son contrat accepté. Vérifier `pulsemon-api.conf` et les surcharges du processus. Les échecs courants sont un port hors `1..65535`, un intervalle inférieur à sa borne, une capacité d’historique nulle, un nombre non fini, une écriture booléenne invalide, un alpha EMA hors `(0, 1]`, un token de header HTTP invalide ou un BDF PCI mal formé.

Les valeurs de configuration ne sont pas reprises dans le message d’erreur afin de ne pas exposer une clé API.

## ESP32 affiche backend offline

L’hôte et le port backend sont configurés dans le portail local et stockés dans le namespace NVS `pulsemon_api`. Vérifier :

- les valeurs `backend_host` et `backend_port` de `GET /api/config` ;
- routage LAN, DNS et pare-feu ;
- adresse de bind et port effectif du backend ;
- les fallbacks compilés `PULSEMON_API_DEFAULT_HOST` et `PULSEMON_API_DEFAULT_PORT` si le namespace a été effacé ;
- `PULSEMON_HTTP_TIMEOUT_MS`.

La sauvegarde du portail recharge immédiatement la cible. L’effacement de la configuration PulseMon restaure le fallback compilé.

## Configuration Wi-Fi ESP32

Vérifier :

- namespace NVS `pulsemon_wifi` ;
- AP `PulseMon-Setup` si identifiants absents ou retries épuisés ;
- adresse du portail `http://192.168.4.1/` pendant la connexion à cet AP ;
- `GET /api/wifi/status` et `GET /api/wifi/scan` uniquement lorsque l’AP de configuration est actif.

Le portail et le DNS captif s’arrêtent après une connexion station réussie. Ils sont volontairement indisponibles via l’adresse LAN station normale. Si l’AP est actif mais que la redirection DNS échoue, ouvrir directement `http://192.168.4.1/`.

Avec des identifiants valides et fonctionnels, maintenir le coin supérieur gauche de n’importe quel écran actif pendant cinq secondes pour ouvrir `PulseMon-Setup`. La fenêtre manuelle dure cinq minutes et ne coupe pas la liaison station. Si l’AP n’apparaît pas, vérifier que l’appui est continu et commence dans le hotspot de 64 × 64 pixels du coin supérieur gauche.

## Météo indisponible

Vérifier :

- présence clé OpenWeather et ID ville ;
- langue et décalage GMT valides ;
- Wi-Fi et DNS ;
- accès HTTPS/DNS à `api.openweathermap.org` ;
- fichiers d’icônes SD si seules les icônes manquent.

La validation TLS OpenWeather utilise le bundle de certificats ESP-IDF ; une erreur TLS ou certificat provoque un échec de rafraîchissement et le dernier snapshot valide est conservé.

## Actualités indisponibles

Vérifier :

- heure SNTP valide ;
- présence de la clé GNews ;
- accès HTTPS/DNS à GNews ;
- intervalle de rafraîchissement et backoff ;
- règles d’âge et validation des articles.

## Échec mémoire HTTPS ou TLS sur ESP32

Pour diagnostiquer la mémoire firmware, utiliser l’environnement `pulsmon-esp32s3-display-dev`. Son instrumentation `PULSEMON_DEBUG` journalise les valeurs libres/minimales INTERNAL, DMA et SPIRAM ainsi que les high-water marks de stack. Le build release omet volontairement ces mesures de diagnostic.

Si le log série contient `esp-aes: Failed to allocate memory`, `pulsemon_diag: alloc_fail` ou `ESP_ERR_HTTP_FETCH_HEADER`, conserver la capture complète du boot jusqu’à l’échec. Vérifier aussi que `pulsemon_https_gate` sérialise toujours Météo et GNews, que la réserve interne de 32 Kio apparaît au boot et qu’aucun stack overflow/canary/watchpoint ne suit une modification mémoire. Ne pas contourner ce type d’échec en autorisant des sessions TLS Météo/GNews concurrentes.

## Ancien écran généré visible dans les recherches source

La sortie EEZ générée contient encore un ancien écran FAN inaccessible et des bindings de compatibilité. La navigation active ne résout que Main, GPU et Météo. Ne pas modifier les fichiers générés ni les bindings de compatibilité pour retirer manuellement cet écran ; effectuer le changement de design dans EEZ Studio puis régénérer.

La télémétrie du ventilateur GPU reste disponible via `/api/v1/gpu/dashboard` lorsque le pilote l’expose.

## Diagnostics backend

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode compare --duration-s 60 --sample-hz 10 --ema-alpha 0.25 --output diagnostics/raw_vs_display_gpu_pct.jsonl
```

```bash
cd api
.venv/bin/python -m app.diagnostics.raw_capture --mode raw --duration-s 60 --sample-hz 10 --output diagnostics/raw_metrics.jsonl
```
