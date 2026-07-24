# Météo et actualités

L’ESP32-S3 récupère directement la météo et les actualités. Ces services n’utilisent pas le backend Linux.

## Configuration météo

Stockée dans le namespace NVS `pulsemon_cfg` :

- `ow_key` ;
- `gmt_min` ;
- `ow_city` ;
- `lang`.

Les langues météo supportées sont `fr`, `en`, `de`, `es` et `it`. Le décalage GMT par défaut est `+60 minutes` et la langue par défaut `fr`.

## Transport OpenWeather courant

Le firmware construit actuellement ces URLs :

```text
http://api.openweathermap.org/data/2.5/weather
http://api.openweathermap.org/data/3.0/onecall
http://api.openweathermap.org/data/2.5/forecast
```

La clé API est placée dans le paramètre `appid`. C’est un défaut connu car les requêtes utilisent HTTP en clair. Ce comportement reste documenté comme existant jusqu’à migration du client vers HTTPS avec validation de certificat.

Le rafraîchissement météo est planifié toutes les 30 minutes. Le service conserve le dernier snapshot météo valide en cas d’échec.

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
