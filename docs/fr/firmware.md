# Firmware ESP32-S3

Le firmware utilise PlatformIO, ESP-IDF et LVGL 8.4 avec la définition de carte custom `jc3248w535c`.

## Environnements de build

- `pulsmon-esp32s3-display` : build release par défaut ;
- `pulsmon-esp32s3-display-dev` : build debug avec flags de diagnostics PulseMon.

## Démarrage

`esp/src/main.c` initialise :

1. écran et UI générée ;
2. support des icônes météo ;
3. gestionnaire Wi-Fi, NVS et callbacks de cycle AP ;
4. services météo et actualités ;
5. connexion Wi-Fi ;
6. serveur HTTP de configuration et DNS captif uniquement lorsque l’AP de configuration démarre réellement ;
7. poller backend après connexion station ;
8. service imprimante en lecture seule, démarré à la demande uniquement pendant l’affichage de l’écran Printer.

## Écrans actifs et navigation

```text
Main --gauche--> GPU --gauche--> Météo --gauche--> Printer
Main <--droite-- GPU <--droite-- Météo <--droite-- Printer
```

La navigation vers Printer est inconditionnelle et indépendante de la disponibilité de l’imprimante. L’entrée sur Printer démarre le service imprimante en lecture seule ; la sortie de Printer arrête sa session MQTT et sa tâche. Le label EEZ généré `imp_gone`, visible par défaut, reste affiché jusqu’à la réception d’un statut `1002` valide rendant l’imprimante disponible. Si la disponibilité est perdue pendant que Printer reste affiché, l’écran reste sur Printer et `imp_gone` est réaffiché. Aucun fichier sous `esp/src/ui/` n’est modifié par l’intégration runtime.

## Polling backend

`PULSEMON_DASHBOARD_POLL_MS` vaut par défaut `1000 ms`.

- Écran GPU : requête `/api/v1/gpu/dashboard`.
- Écrans Main, Météo et Printer : requête `/api/v1/dashboard`. La disponibilité de Printer reste indépendante de celle du backend.
- En cas d’échec backend : conserver les dernières valeurs et marquer le backend offline. Main/GPU basculent automatiquement vers Météo ; un écran Printer déjà actif n’est pas déplacé par la panne backend.
- Au retour du backend après bascule offline automatique : revenir sur Main uniquement si Météo est toujours l’écran actif ; une navigation manuelle ailleurs annule ce retour automatique.

L’hôte et le port backend sont chargés depuis le namespace NVS `pulsemon_api`. `pulsemon_api_config.h` fournit l’hôte/port de fallback compilés et les valeurs fixes de timeout/polling. Les changements du portail sont rechargés immédiatement. Le firmware n’utilise ni découverte backend ni header de clé API. Le portail n’est pas un service LAN permanent : HTTP et DNS captif démarrent avec l’AP de configuration et s’arrêtent avec lui. Un appui maintenu cinq secondes dans le coin supérieur gauche de Main, GPU ou Météo ouvre une fenêtre manuelle de dix minutes en conservant la connexion station. Le déclencheur est implémenté dans le runtime non généré et ne modifie aucune sortie EEZ.

## Télémétrie imprimante

`esp/src/printer_service.c` dialogue directement avec l’imprimante configurée sur le LAN privé. Il n’utilise ni bibliothèque intermédiaire, ni daemon, ni cloud, ni broker externe. L’implémentation courante est strictement en lecture seule. Aucun trafic LAN Printer n’est produit lorsque l’écran Printer n’est pas actif : la tâche et le client MQTT sont créés à l’entrée puis libérés à la sortie.

Séquence de connexion :

1. HTTP `GET /system/info?X-Token=<code-acces>` sur le port 80 pour récupérer le numéro de série de l’imprimante ;
2. connexion MQTT 3.1.1 directe au port 1883 avec l’utilisateur `elegoo` et le code d’accès imprimante comme mot de passe ;
3. enregistrement du client sous l’arborescence de topics du numéro de série ;
4. méthode `1001` pour l’identité et méthode `1002` pour l’état du job courant ;
5. `PING` applicatif toutes les 30 secondes tant que la session est active ;
6. rafraîchissement complet du statut toutes les 5 secondes.

Le service alimente `name_printer`, `printer_ip`, `print_file_name`, `print_time_start`, `print_time_end`, `print_time_elapsed`, `print_time_remaining` et `print_bar`. Les heures de début/fin sont calculées depuis l’horloge locale et les durées écoulée/restante fournies par l’imprimante. Une réponse de statut valide rend Printer disponible ; un échec bootstrap, MQTT ou statut le rend indisponible.

Pour la preview du job courant, la méthode `1002` reste la source du nom de fichier actif. Lorsque ce nom change, le firmware envoie la méthode MQTT en lecture seule `1045 GET_FILE_THUMBNAIL` avec `storage_media="local"` et `file_name=<filename>`. Une réponse valide fournit la preview du slicer dans `result.thumbnail` sous forme de PNG encodé en base64. Le firmware décode ce PNG puis en transfère la propriété au cache d’affichage. Le réassemblage des réponses MQTT utilise une allocation temporaire bornée à 256 Kio demandée explicitement en PSRAM en priorité, avec fallback sur le heap normal. Le PNG décodé suit la même politique PSRAM prioritaire. Ces allocations sont libérées lorsqu’elles ne sont plus nécessaires et la miniature en cache est effacée lorsque le service Printer s’arrête.

`esp/src/printer_thumbnail.c` relie l’image runtime au cadre EEZ généré `image_gode` sans modifier `esp/src/ui/`. Le support PNG LVGL est déjà actif via `LV_USE_PNG=1` ; le cache image runtime est réglé à une entrée afin que le PNG soit décodé une fois puis réutilisé pendant l’affichage. L’ancienne preview est effacée lors d’un changement de job ou à la fin du job. Un échec MQTT/thumbnail transitoire est retenté au maximum toutes les 30 secondes ; une réponse 1045 valide mais sans PNG exploitable n’est pas retentée tant que le nom du job actif ne change pas. Le code d’accès imprimante ne fait pas partie de la requête thumbnail et n’est jamais écrit dans les logs.

## Météo et actualités

Météo et GNews fonctionnent indépendamment du backend dès que le Wi-Fi et les clés nécessaires sont disponibles. Les deux utilisent HTTPS avec validation des certificats via le bundle ESP-IDF et conservent leur état local selon leur implémentation.

## Fichiers EEZ

- `esp/src/ui/` est la sortie générée compilée par le firmware. Ne jamais la modifier directement.
- `esp/eez/pulsmon/` est le projet EEZ Studio et l’état sauvegardé de l’application. Modifier uniquement via EEZ Studio.
- L’intégration runtime non générée reste hors de ces fichiers générés.

La sortie générée conserve un ancien écran FAN inaccessible et les bindings de compatibilité requis. Il n’est pas chargé par la navigation active et n’est adossé à aucune API FAN générique. Le retirer uniquement via EEZ Studio puis régénération.
