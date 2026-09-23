# Météo et actualités

L’ESP32-S3 récupère directement la météo et les actualités. Ces services n’utilisent pas le backend Linux.

## Configuration météo

Stockée dans le namespace NVS `pulsemon_cfg` :

- `ow_key` ;
- `gmt_min` ;
- `ow_city` ;
- `lang`.

Les langues météo supportées sont `fr`, `en`, `de`, `es` et `it`. Le décalage GMT par défaut est `+60 minutes` et la langue par défaut `fr`.

## Transport OpenWeather

Le firmware utilise les endpoints HTTPS suivants :

```text
https://api.openweathermap.org/data/2.5/weather
https://api.openweathermap.org/data/3.0/onecall
https://api.openweathermap.org/data/2.5/forecast
```

La clé API est placée dans le paramètre `appid` requis par OpenWeather. Les certificats TLS sont validés via le bundle de certificats ESP-IDF. Le firmware ne journalise ni l’URL de requête ni la clé.

Le rafraîchissement météo est planifié toutes les 30 minutes. Le service conserve le dernier snapshot météo valide en cas d’échec de requête, de validation TLS ou de parsing.

## Coordination HTTPS et mémoire

Météo et GNews partagent `pulsemon_https_gate`, un mutex firmware qui empêche deux sessions HTTPS/TLS externes simultanées. Météo conserve le verrou pendant toute sa mise à jour current + forecast, y compris le chemin de fallback des prévisions. GNews attend le même verrou avant sa requête.

Après l’obtention de l’IP Wi-Fi, les requêtes de démarrage sont échelonnées par timers one-shot : polling backend après 0,5 s, Météo après 1,5 s cumulé et GNews après 4,5 s cumulé. Le callback du timer ne fait que planifier le travail ; les requêtes réseau s’exécutent dans les tâches des services.

Météo alloue explicitement son body de 32 Kio en PSRAM et GNews fait de même pour son body de 16 Kio. MbedTLS utilise des buffers dynamiques avec la politique d’allocation normale, tandis que 32 Kio de mémoire interne sont réservés aux besoins INTERNAL/DMA. Cette organisation préserve la marge de mémoire interne/DMA pendant les handshakes TLS sans laisser Météo et GNews se la disputer en parallèle.

## Icônes météo

Les icônes binaires sont chargées depuis la carte SD via `/sdcard/icon_150.bin` et `/sdcard/icon_50.bin`.

## GNews

GNews utilise :

```text
https://gnews.io/api/v4/top-headlines
```

Authentification :

```text
X-Api-Key: <clé stockée en NVS>
```

Les certificats TLS sont validés via le bundle de certificats ESP-IDF.

Valeurs par défaut :

- activé ;
- rafraîchissement toutes les 30 minutes ;
- catégorie `general` ;
- langue `fr` ;
- pays `fr` ;
- maximum 5 items ;
- âge maximal 15 jours ;
- vitesse ticker 35.

Le service exige Wi-Fi, DNS, heure SNTP valide et clé GNews stockée. Il valide le titre, la date, l’âge, la langue optionnelle et l’UTF-8, puis conserve les titres valides dans un cache local.

## Priorité de la ligne d’information

1. alerte météo ;
2. actualité valide courante ;
3. actualité en cache ;
4. ligne vide.
