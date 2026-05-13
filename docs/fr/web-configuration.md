# Configuration web ESP32

Le firmware ESP32-S3 expose une petite interface web locale sur son serveur HTTP de configuration. Ce portail sert à configurer l’appareil, pas à fournir un dashboard riche.

## Pages

- `GET /` sert la page de configuration PulseMon.
- `GET /wifi` sert la page de configuration Wi-Fi.

Les chemins `GET` inconnus peuvent revenir vers `/` pour le comportement de portail captif.

## API de configuration

L’API de configuration firmware contient :

- `GET /api/config` ;
- `POST /api/config` ;
- `POST /api/config/clear`.

`POST /api/config` accepte des champs `application/x-www-form-urlencoded` comme :

- `openweather_key` ;
- `clear_openweather_key` ;
- `gnews_key` ;
- `clear_gnews_key` ;
- `news_max_items` ;
- `news_slide_speed` ;
- `gmt_offset_min` ;
- `openweather_city_id` ;
- `language`.

## Namespaces NVS

La configuration principale PulseMon est stockée dans `pulsemon_cfg` :

| Clé | Rôle |
|---|---|
| `ow_key` | clé API OpenWeather |
| `gmt_min` | décalage GMT en minutes |
| `ow_city` | identifiant ville OpenWeather |
| `lang` | langue UI/météo |

La configuration news est stockée dans `news` :

| Clé | Rôle |
|---|---|
| `provider` | fournisseur actif, fixé à GNews |
| `gnews_key` | clé GNews |
| `enabled` | activation du module news |
| `refresh_min` | intervalle de rafraîchissement |
| `category` | catégorie GNews |
| `lang` | langue GNews |
| `country` | pays GNews |
| `max_items` | nombre maximal d’articles |
| `slide_speed` | vitesse de scroll circulaire LVGL |
| `max_age_days` | âge maximal d’un article |
| `last_ok_ts` | timestamp du dernier succès |
| `last_error` | code compact de dernière erreur |

## Gestion des secrets

`GET /api/config` ne doit pas retourner les clés API. Il doit seulement retourner des indicateurs de présence :

- `openweather_key_set` ;
- `gnews_key_set`.

Les opérations de reset doivent supprimer les clés concernées de la NVS.
