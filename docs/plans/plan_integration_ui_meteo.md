# Plan d'integration `ui_meteo`
code source de weather station /home/kaj/Develop/000-PlatformIO/ESP-IDF/Weather-Station/

Objectif: integrer `ui_meteo` dans PulseMon en gardant deux metiers separes: monitoring PC CPU/RAM/GPU d'un cote, meteo de l'autre. Aucun code genere `esp/src/ui/` ne doit etre modifie directement.

## Phase 1 - Analyse de l'existant PulseMon

- Architecture:
  - ESP-IDF via PlatformIO, environnement `LVGL-320-480`.
  - UI EEZ generee dans `esp/src/ui/`, non modifiable directement.
  - Runtime hors genere: `main.c`, `actions.c`, `ui_screen.c`, `vars.c`, `pulsemon_poller.c`, `ui_graphs.c`.
  - API PC consommee: `/api/v1/dashboard` et `/api/v1/gpu/dashboard`.
- Composants IHM:
  - Ecrans: `MAIN`, `GPU`, `FAN` genere mais desactive, `METEO` present.
  - Navigation actuelle: `MAIN -> GPU -> METEO`, retour `METEO -> GPU`.
  - Vars `ui_meteo_*` presentes avec valeurs neutres et setters hors genere.
- Flux principaux:
  - `pulsemon_poller` alimente uniquement le monitoring PC.
  - `ui_screen` gere l'ecran actif, ticks UI et correctifs runtime limites.
  - Toute mise a jour LVGL depuis une tache doit rester sous verrou display.
- Dependances:
  - LVGL 8.4, EEZ, ESP-IDF, HTTP client PulseMon.
  - Pas de service meteo, cache meteo, config meteo, ni i18n dans PulseMon.
- Points d'integration possibles:
  - `vars.c`: stockage texte `ui_meteo_*`.
  - `actions.c`: navigation vers/depuis `SCREEN_ID_METEO`.
  - `ui_screen.c`: horloge/date locale et activation ecran.
  - Futurs modules meteo separes: client, modeles, cache, service, icones.

## Phase 2 - Analyse de `ui_meteo` Weather-Station

- Fonctions presentes:
  - `get/set_var_ui_meteo_*`: bindings EEZ heure, date, temperature, condition, previsions.
  - `ui_screen_apply_time/date`: formatage heure/date vers vars UI.
  - `weather_service_start/request_update`: tache + timer de fetch meteo.
  - `weather_apply_ui`: temperature, condition, icone principale.
  - `weather_apply_forecast`: 6 jours de prevision + icones.
  - `hourly_strip_*`: details horaires, animation, cache horaire.
  - `ui_backend.h`: acces aux objets `ui_meteo_img`, `ui_meteo_fi*`.
- Dependances:
  - OpenWeatherMap, NVS/secrets, Wi-Fi, ESP HTTP TLS, cJSON/C++, cache, SPIFFS/icons.
  - Assets meteo: `icon_50.bin`, `icon_150.bin`, index icones, decodeur svg2bin, images/fonts EEZ.
  - i18n Weather-Station: `i18n`, `lanague`, `_()`, noms de jours traduits.
- Incompatibilites avec PulseMon:
  - IDs differents: `SCREEN_ID_UI_METEO` vs `SCREEN_ID_METEO`.
  - Details horaires absents dans PulseMon.
  - Service meteo et pipeline icones absents.
  - i18n non implemente dans PulseMon: ne pas porter `i18n`, `lanague`, ni `_()` maintenant.
  - Port direct `weather_service` + `hourly_strip` trop large pour une premiere integration.

## Phase 3 - Integration dediee de la page `ui_meteo`

- A verifier avant integration:
  - Correspondance des objets generes PulseMon: `ui_meteo_clock`, `ui_meteo_img`, `ui_meteo_date`, `ui_meteo_temp`, `ui_meteo_condition`, `ui_meteo_fi1..fi6`, `ui_meteo_fd1..fd6`, `ui_meteo_ft1..ft6`.
  - Coherence types entre `esp/src/ui/vars.h` et `esp/src/ui/screens.c`: tous les `ui_meteo_*` texte doivent rester en `const char *`.
  - Assets EEZ requis par la page presents apres generation, sans edition manuelle dans `esp/src/ui/`.
  - Navigation `GPU <-> METEO` fonctionnelle sans reactiver `FAN`.
  - `pulsemon_poller` ne doit pas utiliser `/api/v1/dashboard` pour alimenter la meteo.
- A apporter:
  - Un backend UI minimal si besoin (`ui_meteo_backend.h/.c`) pour acceder aux objets icones hors genere.
  - Un formatage local simple heure/date, sans i18n.
  - Des valeurs neutres et etats d'erreur meteo deterministes (`--`, `offline`, date vide).
  - Plus tard seulement: modules meteo separes `meteo_client`, `meteo_models`, `meteo_cache`, `meteo_service`, `meteo_icons`.
  - Icones dynamiques uniquement quand le pipeline est choisi: index, `.bin`, decodeur, stockage flash.
- A integrer:
  - Etape UI statique: afficher la page avec les vars neutres.
  - Etape horloge/date: mettre a jour `ui_meteo_houre` et `ui_meteo_date` localement.
  - Etape donnees meteo: ecrire uniquement les vars `ui_meteo_temp`, `condition`, `fd*`, `ft*`.
  - Etape icones: raccorder `ui_meteo_img` puis `ui_meteo_fi1..fi6`.
  - Reporter `ui_meteo_details` et `hourly_strip` a une phase separee.
- Controles:
  - Build apres chaque sous-etape: `pio run -e LVGL-320-480`.
  - Verifier absence de WDT/reset, texte non tronque, pas de chevauchement.
  - Verifier que CPU/RAM/GPU continuent de se mettre a jour.
  - Verifier que `FAN` reste masque/desactive.

## Phase 4 - Integration fonctionnelle meteo independante

- Decoupage:
  - `meteo_client`: HTTP meteo uniquement, sans reutiliser le client API PulseMon PC.
  - `meteo_models`: structures current/forecast compactes.
  - `meteo_cache`: dernier snapshot meteo valide + stale/erreur.
  - `meteo_service`: tache/timer meteo, frequence lente, logs sobres.
  - `meteo_icons`: mapping condition -> asset, separe du parsing JSON.
- Integration minimale:
  - Source meteo explicite et configurable, sans toucher au contrat JSON PulseMon.
  - Pas de MQTT, pas de cloud PulseMon, pas de reconstruction metier PC.
  - Pas d'i18n: jours et libelles simples, stables, non traduits.
- Raccordement fonctionnel:
  - Le service meteo met a jour le cache, puis les vars UI sous verrou LVGL.
  - En cas d'echec, conserver le dernier snapshot meteo valide et afficher un etat stale/offline.
  - Ne jamais bloquer l'UI sur une requete HTTP.
- Tests intermediaires:
  - Build firmware.
  - Test meteo online/offline.
  - Test changement d'ecran pendant fetch.
  - Test backend PulseMon PC absent: `ui_meteo` doit rester autonome.
- Non-regression:
  - Ne pas modifier `/api/v1/dashboard`, `/api/v1/gpu/dashboard`, ni les modeles CPU/RAM/GPU.
  - Ne pas melanger `pulsemon_poller` et meteo.
  - Surveiller RAM/Flash avant d'ajouter icones et hourly details.

## Phase 5 - Test global

- Compiler PulseMon: `pio run -e LVGL-320-480`.
- Flasher uniquement apres build propre et demande explicite.
- Tester toute l'application:
  - boot, backend PulseMon, `MAIN`, `GPU`, `METEO`.
  - navigation tactile complete, pas de reset/WDT.
  - valeurs CPU/RAM/GPU toujours mises a jour.
  - `ui_meteo` ne degrade pas polling ni graphes.
- Verifier l'IHM globale:
  - textes visibles, non tronques, sans chevauchement.
  - icones presentes si activees.
  - etat correct si meteo offline et si backend PC offline.
  - logs nominaux peu verbeux.
