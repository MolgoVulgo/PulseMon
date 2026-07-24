# Configuration web locale ESP32

Le firmware démarre un serveur HTTP ESP-IDF de configuration pendant le démarrage normal. Il sert aux paramètres Wi-Fi, météo et news ; ce n’est pas un dashboard de supervision.

## Pages et routes

- `GET /` — page de configuration météo/news ;
- `GET /wifi` — page de configuration Wi-Fi ;
- `GET /api/config` — état météo/news sans secrets ;
- `POST /api/config` — mise à jour météo/news ;
- `POST /api/config/clear` — effacement des namespaces météo et news ;
- `GET /api/wifi/status` — état station/AP ;
- `GET /api/wifi/scan` — scan des réseaux visibles ;
- `POST /api/wifi` — sauvegarde et application des identifiants station ;
- `POST /api/wifi/clear` — effacement des identifiants et activation de l’AP ;
- chemins GET inconnus — page principale pour le comportement portail captif.

## Champs de configuration

`POST /api/config` accepte les champs de formulaire :

- `openweather_key` et `clear_openweather_key` ;
- `gnews_key` et `clear_gnews_key` ;
- `gmt_offset_min` obligatoire ;
- `openweather_city_id` optionnel ;
- `language` obligatoire ;
- `news_max_items` optionnel ;
- `news_slide_speed` optionnel.

`GET /api/config` retourne des indicateurs de présence de clé, jamais les clés elles-mêmes.

## Comportement de l’AP Wi-Fi

L’AP de configuration est `PulseMon-Setup` avec mot de passe vide dans la configuration courante. Il est activé si les identifiants manquent ou si la connexion station échoue plusieurs fois, puis désactivé après connexion station réussie.

Le serveur HTTP et le DNS captif sont démarrés indépendamment de l’activation de l’AP. Le serveur ne possède pas d’authentification applicative dans l’implémentation courante. Cette limite n’est acceptable que dans le réseau local de confiance visé et doit rester explicite.
