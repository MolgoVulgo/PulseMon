# Configuration web locale ESP32

Le firmware expose un serveur HTTP ESP-IDF de configuration uniquement lorsque l’AP de configuration est actif. Il sert à l’endpoint backend et aux paramètres Wi-Fi, Printer, météo et news ; ce n’est ni un dashboard de supervision ni un service LAN permanent.

## Pages et routes

- `GET /` — page de configuration backend/Printer/météo/news ;
- `GET /wifi` — page de configuration Wi-Fi ;
- `GET /api/config` — endpoint backend et état Printer/météo/news sans secrets ;
- `POST /api/config` — mise à jour backend, Printer, météo et news ;
- `POST /api/config/clear` — effacement des namespaces backend, Printer, météo et news avec restauration du fallback endpoint ;
- `GET /api/wifi/status` — état station/AP, état de la fenêtre manuelle et temps restant en millisecondes ;
- `GET /api/wifi/scan` — scan des réseaux visibles ;
- `POST /api/wifi` — sauvegarde et application des identifiants station ;
- `POST /api/wifi/clear` — effacement des identifiants et activation de l’AP ;
- chemins GET inconnus — page principale pour le comportement portail captif.

## Champs de configuration

`POST /api/config` accepte les champs de formulaire :

- `backend_host` obligatoire, validé comme adresse IPv4 ou hostname DNS/mDNS ;
- `backend_port` obligatoire, entier `1..65535` ;
- `printer_host`, validé comme adresse IPv4 ou hostname DNS/mDNS ;
- `printer_access_code`, vide pour conserver le secret enregistré ;
- `clear_printer_config=1` pour effacer les deux champs Printer enregistrés ;
- `openweather_key` et `clear_openweather_key` ;
- `gnews_key` et `clear_gnews_key` ;
- `gmt_offset_min` obligatoire ;
- `openweather_city_id` optionnel ;
- `language` obligatoire ;
- `news_max_items` optionnel ;
- `news_slide_speed` optionnel.

`GET /api/config` retourne `backend_host`, `backend_port`, `printer_host` et des indicateurs de présence de secrets. Il ne retourne jamais les valeurs OpenWeather, GNews ou du code d’accès Printer. Les réglages Printer sont stockés dans le namespace NVS `printer` ; l’endpoint backend reste dans `pulsemon_api`. Une sauvegarde réussie recharge la cible backend et réveille un service Printer actif afin que les nouveaux réglages soient utilisés sans redémarrage.

## Comportement de l’AP Wi-Fi

L’AP de configuration est `PulseMon-Setup` avec mot de passe vide dans la configuration courante. Il est activé automatiquement si les identifiants manquent ou si la connexion station échoue plusieurs fois. Cet AP automatique est désactivé après connexion station réussie ; une fenêtre ouverte manuellement reste active jusqu’à son timeout.

`WIFI_EVENT_AP_START` démarre le serveur HTTP et le DNS captif. `WIFI_EVENT_AP_STOP` arrête d’abord le DNS captif puis le serveur HTTP. Le DNS captif se lie uniquement à `192.168.4.1` ; il n’écoute plus sur toutes les interfaces. Chaque handler HTTP vérifie aussi que l’AP de configuration est actif et retourne `503 Service Unavailable` dans le cas contraire.

L’AP de configuration ne possède pas d’authentification applicative et utilise actuellement un mot de passe Wi-Fi vide. Son exposition est donc volontairement limitée aux périodes où le mode configuration est nécessaire. En fonctionnement normal connecté en station, AP désactivé, le portail n’est pas disponible via l’adresse LAN station.

Un déclencheur local explicite est disponible sur chaque écran actif : maintenir le coin supérieur gauche pendant cinq secondes. Le firmware conserve la connexion station courante et ouvre `PulseMon-Setup` pendant cinq minutes. `/api/wifi/status` indique si cette fenêtre manuelle est active et son temps restant en millisecondes. Les deux pages du portail interrogent cet état sans cache, affichent le temps restant et désactivent le formulaire résiduel avec un message explicite de fermeture lorsque le portail disparaît. Le serveur HTTP et le DNS captif suivent le cycle AP qui en résulte. À l’expiration des cinq minutes, l’AP se ferme automatiquement si la station est connectée ; si la connexion station est indisponible, les règles de fallback normales maintiennent l’accès à la configuration. Le déclencheur n’efface aucun identifiant et ne modifie aucun réglage NVS.
