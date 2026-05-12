# Taches d'integration `ui_meteo`

Source: `docs/plans/plan_integration_ui_meteo.md`.

Objectif: rendre `ui_meteo` fonctionnelle dans PulseMon sans melanger le metier meteo avec le monitoring PC CPU/RAM/GPU. Ne jamais modifier directement `esp/src/ui/`.

## T0 - Controle initial

- [ ] Verifier la branche `meteo`.
- [ ] Verifier l'etat git avant chaque serie de changements.
- [ ] Lire `agent.md`, `esp/agent.md`, `esp/platformio.ini` et les fichiers touches.
- [ ] Confirmer que `esp/src/ui/vars.h` et `esp/src/ui/screens.c` sont coherents pour les vars `ui_meteo_*`.
- [ ] Confirmer que `FAN` reste masque/desactive et hors navigation utilisateur.

Validation:
- `git status --short --branch`
- controle manuel des prototypes `get_var_ui_meteo_*` / usages dans `tick_screen_ui_meteo`.

## T1 - Baseline UI `ui_meteo`

- [ ] Verifier que `SCREEN_ID_METEO` existe et que `objects.ui_meteo` est cree.
- [ ] Verifier les objets requis: `ui_meteo_clock`, `ui_meteo_img`, `ui_meteo_date`, `ui_meteo_temp`, `ui_meteo_condition`, `ui_meteo_fi1..fi6`, `ui_meteo_fd1..fd6`, `ui_meteo_ft1..ft6`.
- [ ] Verifier que les valeurs neutres sont initialisees dans `main.c`.
- [ ] Verifier que la navigation `GPU <-> METEO` fonctionne sans passer par `FAN`.
- [ ] Corriger uniquement hors `esp/src/ui/` si un ajustement runtime est necessaire.

Validation:
- `pio run -e LVGL-320-480`
- test navigation `MAIN -> GPU -> METEO -> GPU`
- verifier visuellement que la page affiche des valeurs neutres sans chevauchement.

## T2 - Horloge/date locale

- [ ] Ajouter ou adapter un tick leger hors genere pour mettre a jour `ui_meteo_houre`.
- [ ] Ajouter ou adapter un formatage local simple pour `ui_meteo_date`.
- [ ] Ne pas introduire `i18n`, `lanague`, `_()` ni table de traduction.
- [ ] Eviter toute allocation dynamique dans le tick.
- [ ] Mettre a jour l'ecran sous verrou LVGL si la mise a jour vient d'une tache.

Validation:
- `pio run -e LVGL-320-480`
- verifier heure/date sur `METEO`
- verifier que `MAIN` et `GPU` restent fonctionnels.

## T3 - Decouplage du poller PulseMon

- [ ] Verifier que `pulsemon_poller` n'alimente pas la meteo avec `/api/v1/dashboard`.
- [ ] Si necessaire, separer explicitement le comportement `SCREEN_ID_METEO` du polling PC.
- [ ] Conserver le polling PC pour `MAIN`/`GPU`.
- [ ] Ne pas modifier le contrat JSON PulseMon.

Validation:
- `pio run -e LVGL-320-480`
- backend PC actif: CPU/RAM/GPU continuent de se mettre a jour.
- backend PC absent: `METEO` reste affichable avec ses valeurs propres/neutres.

## T4 - Backend UI meteo minimal

- [ ] Creer un wrapper hors genere si necessaire: `ui_meteo_backend.h/.c`.
- [ ] Exposer uniquement les acces utiles aux objets icones: `ui_meteo_img`, `ui_meteo_fi1..fi6`.
- [ ] Garder ce wrapper sans logique HTTP, sans parsing JSON et sans logique metier meteo.
- [ ] Proteger les acces objets nuls si l'ecran n'est pas cree.

Validation:
- `pio run -e LVGL-320-480`
- verifier aucun crash en entrant/sortant de `METEO`.

## T5 - Modules meteo independants

- [ ] Creer `meteo_models`: structures compactes current/forecast.
- [ ] Creer `meteo_cache`: dernier snapshot valide, age/stale, etat erreur.
- [ ] Creer `meteo_client`: requete HTTP meteo uniquement.
- [ ] Creer `meteo_service`: tache/timer, frequence lente, logs sobres.
- [ ] Garder les modules meteo independants de `pulsemon_poller` et des modeles CPU/RAM/GPU.
- [ ] Prevoir une configuration meteo explicite sans toucher aux endpoints PulseMon.

Validation:
- `pio run -e LVGL-320-480`
- test unitaire ou test manuel de parsing si le projet expose un chemin simple.
- verifier RAM/Flash apres ajout des modules.

## T6 - Raccordement texte meteo

- [ ] Mapper le snapshot meteo vers `ui_meteo_temp`.
- [ ] Mapper la condition vers `ui_meteo_condition`.
- [ ] Mapper 6 jours de prevision vers `ui_meteo_fd1..fd6`.
- [ ] Mapper les temperatures de prevision vers `ui_meteo_ft1..ft6`.
- [ ] En echec fetch, conserver le dernier snapshot valide et afficher un etat stale/offline.
- [ ] Ne jamais bloquer l'UI pendant une requete HTTP.

Validation:
- `pio run -e LVGL-320-480`
- meteo online: valeurs remplies.
- meteo offline: dernier snapshot ou etat neutre stable.
- changement d'ecran pendant fetch: pas de reset/WDT.

## T7 - Icones meteo

- [ ] Choisir le pipeline icones avant import: assets EEZ fixes ou pipeline Weather-Station `.bin`.
- [ ] Si pipeline `.bin`: apporter seulement les fichiers necessaires (`icon_50.bin`, `icon_150.bin`, index, decodeur, stockage flash).
- [ ] Creer `meteo_icons` pour mapper condition -> asset.
- [ ] Raccorder d'abord l'icone principale `ui_meteo_img`.
- [ ] Raccorder ensuite les icones prevision `ui_meteo_fi1..fi6`.
- [ ] Surveiller RAM/Flash et temps de decode.

Validation:
- `pio run -e LVGL-320-480`
- verifier icones visibles sur `METEO`
- verifier fallback si icone absente ou code inconnu.

## T8 - Details horaires hors premiere integration

- [ ] Ne pas integrer `ui_meteo_details` tant que la page principale n'est pas stable.
- [ ] Ne pas porter `hourly_strip` dans la premiere integration fonctionnelle.
- [ ] Ouvrir une phase/tache separee pour details horaires si besoin.

Validation:
- aucun appel `hourly_strip_*` dans PulseMon pendant cette integration.

## T9 - Test global

- [ ] Compiler le firmware.
- [ ] Flasher uniquement apres demande explicite.
- [ ] Tester boot complet.
- [ ] Tester `MAIN`, `GPU`, `METEO`.
- [ ] Tester backend PC actif puis absent.
- [ ] Tester meteo online puis offline.
- [ ] Verifier absence de WDT/reset.
- [ ] Verifier textes, icones, navigation et logs.
- [ ] Verifier que `FAN` reste non expose.

Validation:
- `pio run -e LVGL-320-480`
- flash/test materiel seulement sur demande.

## Contraintes permanentes

- [ ] Ne pas modifier directement `esp/src/ui/`.
- [ ] Ne pas introduire i18n maintenant.
- [ ] Ne pas introduire MQTT.
- [ ] Ne pas modifier le contrat JSON PulseMon CPU/RAM/GPU.
- [ ] Ne pas melanger meteo et monitoring PC.
- [ ] Limiter allocations dynamiques, logs bruyants et traitements longs dans les ticks UI.
