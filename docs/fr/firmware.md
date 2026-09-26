# Firmware ESP32-S3

Le firmware utilise PlatformIO, ESP-IDF et LVGL 8.4 avec la définition de carte custom `jc3248w535c`.

## Environnements de build

- `pulsmon-esp32s3-display` : build release par défaut ;
- `pulsmon-esp32s3-display-dev` : build debug avec flags de diagnostics PulseMon.

Les captures d’écran automatiques sur SD sont désactivées par défaut en release comme en dev. `PULSEMON_SCREENSHOT_AUTOSTART=0` est défini dans les flags communs de build et `capture_config.h` conserve également cette valeur par défaut ; `main.c` ne démarre donc pas la tâche périodique `lcd_capture` tant que le support screenshot debug et le verrou d’auto-démarrage ne sont pas tous les deux activés explicitement.

## Démarrage

`esp/src/main.c` initialise :

1. écran et UI générée ;
2. support des icônes météo ;
3. gestionnaire Wi-Fi, NVS et callbacks de cycle AP ;
4. workers météo et actualités ainsi que le moniteur de présence Printer en lecture seule ;
5. connexion Wi-Fi ;
6. serveur HTTP de configuration et DNS captif uniquement lorsque l’AP de configuration démarre réellement ;
7. une séquence réseau autonome de premier démarrage après obtention d’une adresse IP station ;
8. le poller backend PC en dernier, sans faire de la disponibilité du backend une condition de démarrage.

Après `IP_EVENT_STA_GOT_IP`, une tâche dédiée `startup_net` exécute la séquence réseau hors du callback d’événement Wi-Fi. L’ordre normal est : démarrer SNTP sur `pool.ntp.org` et attendre au maximum 8 secondes une horloge système valide, demander le premier cycle Météo et attendre au maximum 12 secondes la fin de ce worker, demander le premier cycle GNews et attendre au maximum 10 secondes, demander le premier cycle Printer en arrière-plan, puis démarrer le poller backend PC. Météo et GNews continuent de partager `pulsemon_https_gate`, donc leurs opérations HTTPS/TLS restent sérialisées. La demande Printer reste volontairement non bloquante au démarrage : le worker inactif effectue d’abord son probe HTTP de présence et, uniquement si l’imprimante configurée répond, poursuit avec le cycle de statut MQTT transitoire. Le poller backend démarre en dernier. Le démarrage attend ensuite au maximum 9 secondes que le poller détermine l’état initial du backend : un backend confirmé en ligne libère directement le splash vers Main ; un backend indisponible ou encore indéterminé le libère vers Météo avec la navigation Main/GPU désactivée.

L’écran de démarrage attend au maximum 10 secondes la première connexion Wi-Fi. Si aucune adresse IP station n’est disponible à l’issue de ce délai, le firmware libère directement l’écran de démarrage vers Météo avec les écrans de monitoring PC désactivés tandis que `startup_net` continue d’attendre en arrière-plan ; la même séquence NTP/Météo/GNews/Printer/backend démarre dès qu’une adresse IP est obtenue ultérieurement. Lorsque le backend est ensuite confirmé en ligne, Main/GPU sont réactivés et PulseMon revient sur Main si Météo est toujours l’écran autonome de repli. Un timeout NTP n’est pas bloquant : SNTP reste actif pour une synchronisation ultérieure et le démarrage continue sans dépendre de l’horloge du PC. L’horloge système est obtenue en UTC via SNTP ; l’affichage de l’heure Météo applique localement l’offset `gmt_min` configuré.

## Politique mémoire et TLS

La politique mémoire ESP32-S3 validée est commune aux builds release et dev :

- la PSRAM est accessible au `malloc()` normal et les allocations supérieures à 4096 octets préfèrent la mémoire externe ;
- 32 Kio de mémoire interne sont réservés aux allocations nécessitant INTERNAL/DMA ;
- MbedTLS utilise l’allocateur par défaut avec buffers TLS dynamiques ;
- le body Météo de 32 Kio et le body GNews de 16 Kio sont alloués explicitement en PSRAM ;
- Météo et GNews sont sérialisés par `pulsemon_https_gate` sur l’opération HTTPS complète ;
- le draw buffer LVGL complet est en PSRAM tandis que les deux buffers de transfert DMA utilisent `hres * vres / 20`, soit environ 30 Kio au total en RGB565 320 × 480.

Les stacks ajustées sont de 6144 octets pour le poller backend, 7168 octets pour `MeteoTask`, 8192 octets pour `NewsTask` et 3072 octets pour `MeteoClock`. La tâche LVGL utilise 5120 octets et le worker Printer transitoire 7168 octets ; ces deux valeurs proviennent des mesures DEV de high-water obtenues pendant une impression réelle avec récupération de miniature. Les diagnostics de heap, DMA et high-water de stack sont compilés uniquement dans l’environnement dev via `PULSEMON_DEBUG` ; le build release conserve les protections runtime mais n’émet pas ces mesures et n’enregistre pas le callback d’échec d’allocation. Le build dev mesure en plus heap/stack LVGL une fois par minute, le worker Printer après son premier statut réussi et avant la destruction de la tâche, ainsi que l’état du heap autour des mises à jour du cache de miniature Printer. Il rapporte aussi le coût bloquant des flush LVGL une fois par minute et, pour les changements d’écran animés, le temps exact jusqu’à `LV_EVENT_SCREEN_LOADED` avec le nombre de flush, leur moyenne et leur maximum pendant la transition.

## Écrans actifs et navigation

```text
Main --gauche--> GPU --gauche--> Météo --gauche--> Printer
Main <--droite-- GPU <--droite-- Météo <--droite-- Printer
```

Les transitions animées par swipe utilisent une durée commune de 160 ms. Le fondu de démarrage reste à 200 ms. Cette durée de navigation plus courte est volontaire : les mesures DEV avec le précédent réglage à 220 ms ont observé 70 transitions réelles autour de 250 ms en moyenne, le travail de flush écran ne représentant qu’une partie de cette durée. Lorsque le backend PC est confirmé hors ligne, Main et GPU sont temporairement retirés de la navigation ; seul le parcours Météo <-> Printer reste disponible. Ils sont restaurés uniquement après confirmation du retour backend.

La navigation vers Printer est inconditionnelle et indépendante de la disponibilité de l’imprimante. Un moniteur périodique léger vérifie l’imprimante configurée toutes les 60 secondes lorsque l’écran est inactif. Chaque probe HTTP réussi crée un worker MQTT temporaire, lit l’état courant puis détruit le client MQTT et le worker. L’entrée sur Printer bascule en mode live : le worker conserve sa session MQTT et rafraîchit le statut toutes les 5 secondes. La sortie de Printer termine immédiatement cette session live puis revient au monitoring périodique. Si une impression active a déjà produit un snapshot d’affichage valide, ce snapshot est conservé en PSRAM pendant que l’écran est inactif puis restauré immédiatement au prochain retour sur Printer avant la reprise du polling live. Le label EEZ généré `imp_gone` reste masqué tant qu’un snapshot d’impression active est disponible en cache ; sinon il reste visible jusqu’à la réception d’un statut `1002` valide. Aucun fichier sous `esp/src/ui/` n’est modifié par l’intégration runtime.

## Polling backend

`PULSEMON_DASHBOARD_POLL_MS` vaut par défaut `1000 ms`.

- Écran GPU : requête `/api/v1/gpu/dashboard`.
- Écrans Main, Météo et Printer : requête `/api/v1/dashboard`. La disponibilité de Printer reste indépendante de celle du backend.
- La disponibilité backend est suivie avec les états `UNKNOWN`, `ONLINE`, `SUSPECT` et `OFFLINE`. Un seul échec fait seulement passer `ONLINE`/`UNKNOWN` à `SUSPECT` ; l’écran courant et les dernières valeurs valides sont conservés.
- L’état `OFFLINE` n’est déclaré qu’après des échecs continus pendant `PULSEMON_BACKEND_OFFLINE_GRACE_MS` (5000 ms). Main/GPU sont alors désactivés et un écran Main/GPU actif bascule une seule fois vers Météo ; Printer n’est jamais déplacé.
- En état `OFFLINE`, le retour exige `PULSEMON_BACKEND_RECOVERY_SUCCESSES` (2) réponses dashboard valides consécutives. Un échec pendant cette récupération remet le compteur à zéro.
- Une fois le retour confirmé, Main/GPU sont réactivés. Si Météo est toujours l’écran de repli automatique, PulseMon revient sur Main ; si l’utilisateur consulte Printer, cet écran n’est pas interrompu.

L’hôte et le port backend sont chargés depuis le namespace NVS `pulsemon_api`. `pulsemon_api_config.h` fournit l’hôte/port de fallback compilés et les valeurs fixes de timeout/polling. Les changements du portail sont rechargés immédiatement. Le firmware n’utilise ni découverte backend ni header de clé API. Le portail n’est pas un service LAN permanent : HTTP et DNS captif démarrent avec l’AP de configuration et s’arrêtent avec lui. Un appui maintenu cinq secondes dans le coin supérieur gauche de Main, GPU ou Météo ouvre une fenêtre manuelle de cinq minutes en conservant la connexion station. Le déclencheur est implémenté dans le runtime non généré et ne modifie aucune sortie EEZ.

## Télémétrie imprimante

`esp/src/printer_service.c` dialogue directement avec l’imprimante configurée sur le LAN privé. Il n’utilise ni bibliothèque intermédiaire, ni daemon, ni cloud, ni broker externe. L’implémentation courante est strictement en lecture seule. Lorsque l’écran Printer est inactif, un timer de 60 secondes crée un worker temporaire uniquement lorsqu’un contrôle est dû. Le worker commence par la requête HTTP `/system/info` existante, utilisée comme probe de présence peu coûteux ; si ce probe échoue, aucun client MQTT n’est créé. S’il réussit, MQTT existe uniquement le temps de rafraîchir identité/statut puis le client et le worker sont détruits. La stack Printer de 7 Kio n’est donc pas maintenue résidente entre deux contrôles inactifs. Seuls le dernier snapshot d’affichage valide d’une impression active et sa miniature restent conservés localement entre les sessions.

L’hôte et le code d’accès imprimante sont chargés depuis le namespace NVS `printer` (`host`, `access_code`). `printer_config.h` ne contient plus que les constantes runtime/protocole fixes de Printer et aucun credential. Le portail local expose le host sauvegardé et uniquement un indicateur booléen de présence du code d’accès ; une modification ou un effacement des réglages Printer demande un rafraîchissement immédiat. Une session live ferme l’ancienne connexion, tandis qu’un service inactif lance un cycle court en arrière-plan avec les nouveaux réglages, sans reboot.

Séquence de connexion :

1. HTTP `GET /system/info?X-Token=<code-acces>` sur le port 80 sert de probe de présence et récupère le numéro de série de l’imprimante ;
2. uniquement après réussite de ce probe, connexion MQTT 3.1.1 directe au port 1883 avec l’utilisateur `elegoo` et le code d’accès imprimante comme mot de passe ;
3. enregistrement du client sous l’arborescence de topics du numéro de série ; le firmware attend jusqu’à 30 secondes le `register_response` avant de considérer cette session MQTT indisponible, tandis que le timeout MQTT normal reste à 5 secondes ;
4. méthode `1001` pour l’identité et méthode `1002` pour l’état du job courant ;
5. lorsque Printer est inactif, un seul cycle de statut est effectué puis MQTT est détruit immédiatement ;
6. lorsque Printer est actif, la session MQTT reste ouverte, un `PING` applicatif est envoyé toutes les 30 secondes et le statut complet est rafraîchi toutes les 5 secondes.

Le service alimente `name_printer`, `printer_ip`, `print_file_name`, `print_time_start`, `print_time_end`, `print_time_elapsed`, `print_time_remaining`, `print_layer` et `print_bar`. Pour un job actif, `print_layer` conserve toujours la forme `courante/total` : la couche courante vient du champ `result.print_status.current_layer` de la méthode `1002`, tandis que le nombre total de couches est récupéré séparément par la méthode en lecture seule `1046 GET_FILE_DETAIL`, avec `storage_media="local"` et le `filename` brut ; le total est lu dans `result.layer`. La requête 1046 est mémorisée par nom de fichier actif et n’est retentée au maximum que toutes les 30 secondes tant que le total reste indisponible. Pendant l’attente du total, l’affichage conserve les deux parties sous la forme `courante/--`. L’ancien champ texte `layerProgress` reste accepté uniquement en fallback si `current_layer` lui-même est absent. Le `print_file_name` affiché correspond au nom du modèle d’origine : il est tronqué à la première extension source `.stl`, `.obj` ou `.3mf`, sans tenir compte de la casse, afin de masquer les informations ajoutées par le slicer ; si aucune extension source n’est trouvée, seul un suffixe final `.gcode` est retiré. Le nom brut renvoyé par l’imprimante reste conservé en interne pour la détection des changements de job et les méthodes `1045`/`1046`. `print_time_elapsed` avance localement chaque seconde lorsque l’écran Printer est actif, à partir de la dernière valeur `print_duration` reçue de l’imprimante ; chaque statut méthode 1002 sert donc de correction/resynchronisation et non de seule mise à jour de l’horloge. La restauration du cache Printer recalcule également l’elapsed depuis cette même ancre. Les heures de début/fin continuent d’être recalculées depuis l’horloge locale et les dernières durées écoulée/restante fournies par l’imprimante. Les réponses MQTT mettent toujours à jour le snapshot/cache interne, mais les variables LVGL ne sont écrites que lorsque l’écran Printer est actif ; à l’entrée sur Printer, un snapshot de job actif déjà en cache est restauré en premier. Quitter Printer n’efface pas ce cache. Un statut sans impression ou une modification de la configuration Printer invalide le job en cache. Le probe HTTP authentifié est l’autorité de présence d’un cycle lorsque l’écran est inactif : un probe réussi passe Printer disponible avant le démarrage de MQTT. Si la session MQTT courte ou la requête de statut échoue ensuite en arrière-plan, la disponibilité reste active et le worker est détruit ; le cycle suivant à 60 secondes retente normalement. Un échec du probe HTTP rend Printer indisponible. En mode Printer live, un échec de session MQTT ou de statut peut toujours marquer la télémétrie live indisponible jusqu’au prochain reconnect/probe, sans effacer un snapshot d’impression active déjà mémorisé.

Pour la preview du job courant, la méthode `1002` reste la source du nom brut du fichier actif. Un cycle de statut en arrière-plan peut détecter un changement de job et effacer une ancienne preview en cache, mais il ne récupère pas une nouvelle miniature lorsque Printer est inactif. En mode Printer live, le firmware envoie la méthode MQTT en lecture seule `1045 GET_FILE_THUMBNAIL` avec `storage_media="local"` et le `file_name=<filename>` brut inchangé. Une réponse valide fournit la preview du slicer dans `result.thumbnail` sous forme de PNG encodé en base64. Le firmware décode ce PNG directement en PSRAM puis en transfère la propriété au cache d’affichage. Le réassemblage des réponses MQTT utilise une allocation temporaire bornée à 256 Kio demandée explicitement en PSRAM en priorité, avec fallback sur le heap normal. Le PNG en cache est conservé en quittant Printer tant que le job ne change pas. Il est effacé lorsque le job actif change ou se termine, ou lors du rechargement de la configuration Printer.

`esp/src/printer_thumbnail.c` relie l’image runtime au cadre EEZ généré `image_gode` sans modifier `esp/src/ui/`. Le support PNG LVGL est actif via `LV_USE_PNG=1` et `LV_IMG_CACHE_DEF_SIZE=1` active une entrée de cache d’image décodée afin que le PNG puisse être réutilisé pendant l’affichage au lieu d’être redécodé à chaque redraw. Les grosses allocations d’image adossées à `malloc()` continuent de suivre la politique firmware de préférence PSRAM. L’ancienne preview est effacée lors d’un changement de job ou à la fin du job. Un échec MQTT/thumbnail transitoire est retenté au maximum toutes les 30 secondes ; une réponse 1045 valide mais sans PNG exploitable n’est pas retentée tant que le nom du job actif ne change pas. Le code d’accès imprimante ne fait pas partie de la requête thumbnail et n’est jamais écrit dans les logs.

## Météo et actualités

Météo et GNews fonctionnent indépendamment du backend dès que le Wi-Fi et les clés nécessaires sont disponibles. L’heure système est synchronisée directement par l’ESP32-S3 via SNTP avant leur séquence normale de premier démarrage ; le backend PC n’est plus une source d’horloge. Les deux utilisent HTTPS avec validation des certificats via le bundle ESP-IDF et conservent leur état local selon leur implémentation.

## Fichiers EEZ

- `esp/src/ui/` est la sortie générée compilée par le firmware. Ne jamais la modifier directement.
- `esp/eez/pulsmon/` est le projet EEZ Studio et l’état sauvegardé de l’application. Modifier uniquement via EEZ Studio.
- L’intégration runtime non générée reste hors de ces fichiers générés.

La sortie générée conserve un ancien écran FAN inaccessible et les bindings de compatibilité requis. Il n’est pas chargé par la navigation active et n’est adossé à aucune API FAN générique. Le retirer uniquement via EEZ Studio puis régénération.
